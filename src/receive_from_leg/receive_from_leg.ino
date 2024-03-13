/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "BLEDevice.h"
#include <BLE2902.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <Adafruit_SSD1306.h>

// server information
BLEServer* pServer = NULL;
BLECharacteristic* pTransmit = NULL;
BLECharacteristic* pLeft = NULL;
BLECharacteristic* pRight = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// information for receiving data
#define leftLegBLESERVERNAME "STRIDESYNCLEGCPU_LEFT"
#define rightLegBLESERVERNAME "STRIDESYNCLEGCPU_RIGHT"
// The remote service we wish to connect to.
#define SERVICE_UUID "a26972ba-affb-420f-8b53-0db2d9124395"
#define LEFT_LEG_UUID "7b0eb53c-a873-466f-bc1c-1ff732f08957"
#define RIGHT_LEG_UUID "f542a2e9-d11f-4e48-999c-242ef20b5876"

// information for transmitting data
#define TRANSMIT_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define TRANSMIT_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define NUM_FLOATS 6
#define SECONDS_IN_MINUTE 60
#define SAMPLES_PER_SECOND 15
#define MAX_NUM_MINUTES 2

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int buttonPin = 0;
const int ledPin = A2;    // the number of the LED pin

static BLEUUID serviceUUID(SERVICE_UUID);
// The characteristic of the remote service we are interested in.

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

int buttonState = 1;
int lastButtonState = 1;
int receive = 1;

bool started = false;

int64_t time_ms = 0;

int MAX_FLOATS = 2*NUM_FLOATS*SECONDS_IN_MINUTE*MAX_NUM_MINUTES*SAMPLES_PER_SECOND;
float float_buffer[2*NUM_FLOATS*SECONDS_IN_MINUTE*MAX_NUM_MINUTES*SAMPLES_PER_SECOND];
int64_t num_samples_received = 0;

BLEClient*  pClient;

/** All information regarding SD card write and read **/
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { 
    BLEDevice::startAdvertising();
    deviceConnected = true; };

  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

void setup_oled() 
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

void refreshDisplay(char *buffer)
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(buffer);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // setting up BLE Application
  Serial.println("Starting Arduino BLE Server application...");
  BLEDevice::init("StrideSync");

  setup_oled();
  delay(2000);         // wait for initializing
  display.clearDisplay(); // clear display

  display.setTextSize(2);          // text size
  display.setTextColor(WHITE);     // text color
  display.setCursor(0, 10);        // position to display
  refreshDisplay("Run not\nstarted.");
  display.display();               // show on OLED
  // Start advertising
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(TRANSMIT_SERVICE_UUID);
  BLEService* pReceiveService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTransmit = pService->createCharacteristic(
      TRANSMIT_UUID, BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE);

  // create characteristics for receiving data
  pLeft = pReceiveService->createCharacteristic(
    LEFT_LEG_UUID, BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE);

  pRight = pReceiveService->createCharacteristic(
    RIGHT_LEG_UUID, BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE);

  pTransmit->addDescriptor(new BLE2902());
  pLeft->addDescriptor(new BLE2902());
  pRight->addDescriptor(new BLE2902());


  // setup all transmission information to computer
  // Start the service
  pService->start();
  pReceiveService->start();
  

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->addServiceUUID(TRANSMIT_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(
      0x0);  // set value to 0x00 to not advertise this parameter
  
  // set up pushbutton to turn on or off transmission
  pinMode(buttonPin, INPUT_PULLUP);
  BLEDevice::startAdvertising();
  Serial.println("Started advertising.");
} // End of setup.

// This is the Arduino main loop function.
void loop() {
  lastButtonState = buttonState;
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH && lastButtonState == LOW) {
    // turn LED on:
    receive = !receive;
    started = false;
    digitalWrite(ledPin, receive);
  }

  if (deviceConnected) {
      if (receive) {
        Serial.println("receiving");
        float list_of_zeros[6] = {0, 0, 0, 0, 0, 0};
        if (strcmp((const char*)pLeft->getData(), "START") && strcmp((const char*)pLeft->getData(), "START")){
          // now do checking to make sure that neither are empty strings
          // NOTE: CHANGE BACK TO || NOT &&
          if (!strcmp((const char*)pLeft->getData(), "") || !strcmp((const char*)pLeft->getData(), "")){
            if (started)
              refreshDisplay(">=1 knee\nsleeve not\ntransmit");
            else
              return;
          }
          if (SECONDS_IN_MINUTE*MAX_NUM_MINUTES*SAMPLES_PER_SECOND <= num_samples_received) {
            refreshDisplay("max samples\nreceived");
            return;
          }
          started = true;
          char oled_buf[100];
          sprintf(oled_buf, "Run start\nTime: %.2f", (float)time_ms/1000);
          refreshDisplay(oled_buf);
          time_ms += 67;
          // if strings are not uninitialized and not null, add them to the csv
          float *mini_float_buffer = (float*)float_buffer + 2*NUM_FLOATS*num_samples_received;
          char buf[100];
          sprintf(buf, "mini_float_buffer: %p, mini_float_buffer + NUM_FLOATS*sizeof(float): %p", (const char*)mini_float_buffer, (const char*)mini_float_buffer + NUM_FLOATS*sizeof(float));
          Serial.println(buf);
          memcpy(mini_float_buffer, pLeft->getData(), NUM_FLOATS*sizeof(float));
          memcpy(mini_float_buffer + NUM_FLOATS, pRight->getData(), NUM_FLOATS*sizeof(float));

          // Serial.print("Floats received from right: ");
          // Serial.print(last_six_floats[0]);
          // Serial.print(", ");
          // Serial.print(last_six_floats[1]);
          // Serial.print(", ");
          // Serial.print(last_six_floats[2]);
          // Serial.print(", ");
          // Serial.print(last_six_floats[3]);
          // Serial.print(", ");
          // Serial.print(last_six_floats[4]);
          // Serial.print(", ");
          // Serial.println(last_six_floats[5]);

          num_samples_received += 1;
        }
      } else {
        // transmitting file to computer when available
        // waiting for computer to become available
        while (strcmp((const char*)pTransmit->getData(), "GOOD")) {
          Serial.println("WAITING UNTIL GOOD");
          refreshDisplay("Run stop\nTransfer\nto PC");
          delay(500);
          if (buttonState == HIGH && lastButtonState == LOW) {
            // turn LED on:
            receive = !receive;
            digitalWrite(ledPin, receive);
          }
        }
        // NOTE: CHANGE BACK TO i++
        for (int64_t i = 0; i < num_samples_received; i++) {
          float * mini_float_buffer = (float*)float_buffer + 2*NUM_FLOATS*i;

          char buf[100];
          sprintf(buf, "mini_float_buffer: %p, mini_float_buffer + NUM_FLOATS*sizeof(float): %p", (const char*)mini_float_buffer, (const char*)mini_float_buffer + NUM_FLOATS*sizeof(float));
          Serial.println(buf);

          Serial.print("Floats transmitting: ");
          Serial.print(mini_float_buffer[0]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[1]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[2]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[3]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[4]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[5]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[0 + 6]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[1 + 6]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[2 + 6]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[3 + 6]);
          Serial.print(", ");
          Serial.print(mini_float_buffer[4 + 6]);
          Serial.print(", ");
          Serial.println(mini_float_buffer[5 + 6]);

          pTransmit->setValue((unsigned char*)mini_float_buffer, 2*NUM_FLOATS*sizeof(float));
          pTransmit->notify();
          Serial.println("Transmitting");
          Serial.print("num_samples_received: ");
          Serial.print(num_samples_received);
          Serial.print(", i: ");
          Serial.println(i);
          sprintf(buf, "mini_float_buffer: %p\n", (const char*)mini_float_buffer);
          Serial.println(buf);
          refreshDisplay("Transfer\n-ing to\nPC...");

          while(strcmp((const char *)pTransmit->getData(), "GOOD")) {
            // Serial.println("WAITING UNTIL GOOD");
            delay(5);
          }
        }
        pTransmit->setValue("DONE");
        pTransmit->notify();
        refreshDisplay("Done.");
        delay(3000);
        receive = !receive;
        refreshDisplay("Run not\nstarted.");
        pLeft->setValue("START");
        pLeft->notify();
        pRight->setValue("START");
        pRight->notify();
      }
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  
  delay(67); // Delay half a second between loops.
  Serial.write(13);
} // End of loop