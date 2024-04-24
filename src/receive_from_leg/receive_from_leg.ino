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
int devices_connected = 0;

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

#define RED_LED A3
#define GREEN_LED A2
#define BUTTON A0
#define BUTTON_STAY A1
#define DELAY_BETWEEN 67

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static BLEUUID serviceUUID(SERVICE_UUID);
// int iteration = 0;
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
char * fileToWrite = "/session0.txt";

int64_t time_ms = 0;
int64_t last_time_display = -1000;
int64_t last_time_ms = -DELAY_BETWEEN;
int64_t init_time_ms = -1;


int MAX_FLOATS = 2*NUM_FLOATS*SECONDS_IN_MINUTE*MAX_NUM_MINUTES*SAMPLES_PER_SECOND;

BLEClient*  pClient;

/** All information regarding SD card write and read **/

void writeFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        // Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        // Serial.println("File written");
    } else {
        // Serial.println("Write failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    // Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        // Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        // Serial.println("Message appended");
    } else {
        // Serial.println("Append failed");
    }
    file.close();
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { 
    BLEDevice::startAdvertising();
    devices_connected += 1; };

  void onDisconnect(BLEServer* pServer) { devices_connected -= 1; }
};

void setup_oled() 
{
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    // Serial.println(F("SSD1306 allocation failed"));
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

  // while (!Serial); // only turn on while debugging

  pinMode(RX, OUTPUT);
  if(!SD.begin(RX)){
      // Serial.println("Card Mount Failed");
      return;
  }

  writeFile(SD, fileToWrite, "");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  // Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // setting up BLE Application
  // Serial.println("Starting Arduino BLE Server application...");
  BLEDevice::init("StrideSync");

  // setup OLED
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
  pinMode(BUTTON, INPUT);
  // setting one side of the switch to high at all times
  pinMode(BUTTON_STAY, OUTPUT);
  digitalWrite(BUTTON_STAY, HIGH);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  BLEDevice::startAdvertising();
  // Serial.println("Started advertising.");
} // End of setup.

// This is the Arduino main loop function.
void loop() {
  lastButtonState = buttonState;
  buttonState = digitalRead(BUTTON);
  // Serial.println(buttonState);

  /**
  Method for the loop
  1. Check if switch on or off
  2. If switch on, wait for connection to legs and transmit
  3. If switch off, say "StrideSync off" OR wait for transmit to computer
  */

  // show you are stopping the run
  if (buttonState == LOW && lastButtonState == HIGH) {
    // turn LED on:
    receive = !receive;
    digitalWrite(RED_LED, HIGH);
    delay(1000);
    digitalWrite(RED_LED, LOW);
  }

  if (devices_connected >= 2) {
    /// 1. Check if switch on or off
  if (buttonState == HIGH) {
    /// 2. If switch on, wait for connection to legs and transmit
    if (strcmp((const char*)pLeft->getData(), "START") && strcmp((const char*)pLeft->getData(), "START")){
      // now do checking to make sure that neither are empty strings
      if (!strcmp((const char*)pLeft->getData(), "") || !strcmp((const char*)pLeft->getData(), "")){
        if (started)
          refreshDisplay(">=1 knee\nsleeve not\ntransmit");
        else
          return;
      }
      if (!started) {
        init_time_ms = millis();
        digitalWrite(GREEN_LED, HIGH);
        delay(1000);
        digitalWrite(GREEN_LED, LOW);
      }
      started = true;
      char oled_buf[100];
      sprintf(oled_buf, "Run start\nTime: %.1f", (float)time_ms/1000);
      if (time_ms - last_time_display > 500) {
        last_time_display = time_ms;
        refreshDisplay(oled_buf);
      }
      time_ms = millis() - init_time_ms;
      if (time_ms - last_time_ms >= DELAY_BETWEEN) {
        Serial.print("Time between time_ms and last_time_ms: ");
        Serial.println(time_ms - last_time_ms);
        last_time_ms = time_ms;
        // Serial.println(iteration);
        // iteration++;
        // if strings are not uninitialized and not null, add them to the csv
        float mini_float_buffer[2*NUM_FLOATS];
        memcpy(mini_float_buffer, pLeft->getData(), NUM_FLOATS*sizeof(float));
        memcpy(mini_float_buffer + NUM_FLOATS, pRight->getData(), NUM_FLOATS*sizeof(float));
        Serial.println("Done A");

        char string_buffer[100]; 
        Serial.println("Done B");
        sprintf(string_buffer, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
          mini_float_buffer[0],
          mini_float_buffer[1],
          mini_float_buffer[2],
          mini_float_buffer[3],
          mini_float_buffer[4],
          mini_float_buffer[5],
          mini_float_buffer[6],
          mini_float_buffer[7],
          mini_float_buffer[8],
          mini_float_buffer[9],
          mini_float_buffer[10],
          mini_float_buffer[11]
          );
        Serial.println("Done C");
        appendFile(SD, fileToWrite, string_buffer);
        Serial.println("Done D");
      }
      else
        return;
    }
  } else {
    /// 3. If switch off, say "StrideSync off" OR wait for transmit to computer
    if (started) {
      // transmitting file to computer when available
    // waiting for computer to become available
    while (strcmp((const char*)pTransmit->getData(), "GOOD")) {
      // Serial.println("WAITING UNTIL GOOD");
      refreshDisplay("Run stop\nTransfer\nto PC");
      delay(500);
      if (buttonState == HIGH && lastButtonState == LOW) {
        // turn LED on:
        receive = !receive;
        digitalWrite(RED_LED, receive);
      }
    }
    File file = SD.open(fileToWrite);
    // NOTE: CHANGE BACK TO i++
    // Serial.println("reading /session0.txt");
    String buf;
    char terminator = '\n';
    bool button_high = false;
    while (file.available()) {
      String string_buf_first_line = file.readStringUntil(terminator);
      String string_buf_second_line = "";
      if (file.available()) {
        string_buf_second_line = file.readStringUntil(terminator);
      }
      char *ptr = NULL;

      char buf[string_buf_first_line.length() + 1] = {};

      string_buf_first_line.toCharArray(buf, string_buf_first_line.length());

      // convert bytes to float so we can read them
      float float_array[4*NUM_FLOATS] = {};
      byte index = 0;
      ptr = strtok(buf, ",");
      while(ptr != NULL)
      {
          float_array[index] = atof(ptr);
          index++;
          ptr = strtok(NULL, ",");
      }

      string_buf_second_line.toCharArray(buf, string_buf_second_line.length());

      // convert bytes to float so we can read them
      index = 0;
      ptr = strtok(buf, ",");
      while(ptr != NULL)
      {
          float_array[2*NUM_FLOATS + index] = atof(ptr);
          index++;
          ptr = strtok(NULL, ",");
      }
      // Serial.print("Sent ");
      // Serial.println(string_buf);
      // Serial.print(float_array[0]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[1]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[2]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[3]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[4]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[5]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[6]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[7]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[8]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[9]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.print(float_array[10]*RAD_TO_DEG);
      // Serial.print(", ");
      // Serial.println(float_array[11]*RAD_TO_DEG);

      // Serial.print("Sent ");
      // Serial.println(buf);
      pTransmit->setValue((unsigned char *)float_array, 4*sizeof(float)*NUM_FLOATS);
      pTransmit->notify();

      digitalWrite(GREEN_LED, button_high);
      button_high = !button_high;

      // waiting for reader to say they've received all the data
      while(strcmp((const char *)pTransmit->getData(), "GOOD")) {
        // Serial.println("WAITING UNTIL GOOD");
        delay(5);
      }

      delay(5);  // bluetooth stack will go into congestion, if too many packets
                // are sent, in 6 hours test i was able to go as low as 3ms
    }
    pTransmit->setValue("DONE");
    pTransmit->notify();
    refreshDisplay("Done.");
    digitalWrite(GREEN_LED, HIGH);
    delay(3000);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    receive = !receive;
    refreshDisplay("Run not\nstarted.");
    pLeft->setValue("START");
    pLeft->notify();
    pRight->setValue("START");
    pRight->notify();
    started = false;
    time_ms = 0;
    pTransmit->setValue("");
    deleteFile(SD, fileToWrite);
    writeFile(SD, fileToWrite, "");
    } else {
      refreshDisplay("Turn on to\nstart run");
    }
  }
  } else {
    refreshDisplay("Waiting \nfor \nconnection");
  }
  delay(1); // Delay
  // Serial.write(13);
} // End of loop