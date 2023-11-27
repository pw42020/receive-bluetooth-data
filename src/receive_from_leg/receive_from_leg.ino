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

BLEClient*  pClient;


char * fileToWrite = "/session0.csv";

/** All information regarding SD card write and read **/

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    // Serial.print("Notify callback for characteristic ");
    // Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    // Serial.print(" of data length ");
    // Serial.println(length);
    // Serial.print("data: ");
    // Serial.write(pData, length);
    // if (fileToWrite == NULL)
    char buf[length + 2] = {};
    memcpy(buf, pData, length);
    strcat(buf, "\n"); // adding new line character to 
    appendFile(SD, fileToWrite, buf);
    // Serial.print("Wrote ");
    // Serial.print(buf);
    // Serial.print(" to ");
    // Serial.println(fileToWrite);
    // Serial.println();
    delay(67); // delay for 67ms to mimic 15Hz sample rate which we will be receiving

    // check once again to see if done with run
    lastButtonState = buttonState;
    buttonState = digitalRead(buttonPin);

    // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (buttonState == HIGH && lastButtonState == LOW) {
      // turn LED on:
      receive = !receive;
      digitalWrite(ledPin, receive);
    }
}

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

/** returns pClient so user can disconnect later */
// BLEClient* connectToServer() {
//     Serial.print("Forming a connection to ");
//     Serial.println(myDevice->getAddress().toString().c_str());
    
//     pClient  = BLEDevice::createClient();
//     Serial.println(" - Created client");

//     pClient->setClientCallbacks(new MyClientCallback());

//     // Connect to the remove BLE Server.
//     pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
//     Serial.println(" - Connected to server");
//     pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
//     // Obtain a reference to the service we are after in the remote BLE server.
//     Serial.println("Available services:");

//     BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
//     if (pRemoteService == nullptr) {
//       Serial.print("Failed to find our service UUID: ");
//       Serial.println(serviceUUID.toString().c_str());
//       pClient->disconnect();
//       return NULL;
//     }
//     Serial.println(" - Found our service");


//     // Obtain a reference to the characteristic in the service of the remote BLE server.
//     pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
//     if (pRemoteCharacteristic == nullptr) {
//       Serial.print("Failed to find our characteristic UUID: ");
//       Serial.println(charUUID.toString().c_str());
//       pClient->disconnect();
//       return NULL;
//     }
//     Serial.println(" - Found our characteristic");

//     // Read the value of the characteristic.
//     if(pRemoteCharacteristic->canRead()) {
//       std::string value = pRemoteCharacteristic->readValue();
//       Serial.print("The characteristic value was: ");
//       Serial.println(value.c_str());
//     }

//     if(pRemoteCharacteristic->canNotify())
//       pRemoteCharacteristic->registerForNotify(notifyCallback);

//     connected = true;
//     return pClient;
// }

void openNewFileToWrite() {
  // opening new file to write all characteristics to
  File root = SD.open("/");

  File file = root.openNextFile();
  File next_file = file.openNextFile();
  while (next_file) {
    file = next_file;
    next_file = file.openNextFile();
  }

  if (file == NULL) {
    fileToWrite = "/session0.csv";
  } else {
    // creating file of newest number (i.e. 1->2, 12->13, etc)
    std::string filename = file.name();

    fileToWrite = "/";

    // remove '/' and '.csv'
    sprintf(fileToWrite, "/session%d.csv", atoi(filename.substr(8, filename.size() - 4).c_str()) + 1);
    writeFile(SD, fileToWrite, "");
  }
  Serial.print("Created file named ");
  Serial.println(fileToWrite);
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
      
    if (advertisedDevice.getName() != "") {
      Serial.print("BLE Advertised Device found: Name: ");
      Serial.print(advertisedDevice.getName().c_str());
      Serial.print(", Address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      Serial.print("Advertised device has service UUIDs: ");
      Serial.println(advertisedDevice.haveServiceUUID());
    }
    if (advertisedDevice.getName() == leftLegBLESERVERNAME) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      doScan = true;
      Serial.println("Device found. Connecting!");
    }
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void setupMicroSD() {
  digitalWrite(RX, LOW);
  Serial.println("Hello Stride Sync MicroSD initialization");
  if(!SD.begin(RX)){
      Serial.println("Card Mount Failed");
      return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  listDir(SD, "/", 0);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  deleteFile(SD, "/hello.txt");
  writeFile(SD, fileToWrite, "");
  
  return;
}


void setup() {
  Serial.begin(115200);

  // initializing MicroSD card
  setupMicroSD();
  

  // setting up BLE Application
  Serial.println("Starting Arduino BLE Server application...");
  BLEDevice::init("StrideSync");

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
    digitalWrite(ledPin, receive);
  }

  if (deviceConnected) {
      if (receive) {
        if (strcmp((const char*)pLeft->getData(), "START") && strcmp((const char*)pLeft->getData(), "START")){
          // now do checking to make sure that neither are empty strings
          if (!strcmp((const char*)pLeft->getData(), "") || !strcmp((const char*)pLeft->getData(), ""))
            return;
          // if strings are not uninitialized and not null, add them to the csv
          char buf[100];
          sprintf(buf, "%s,%s\n", (const char*)pLeft->getData(), (const char*)pRight->getData());
          appendFile(SD, fileToWrite, buf);
          Serial.println(fileToWrite);
          Serial.printf("Appending to file %s", buf);
        }
      } else {
        // transmitting file to computer when available
        // waiting for computer to become available
        while (strcmp((const char*)pTransmit->getData(), "GOOD")) {
          Serial.println("WAITING UNTIL GOOD");
          delay(1);
        }
        File file = SD.open(fileToWrite);

        Serial.println("reading /session0.csv");
        String buf;
        char terminator = '\n';
        while(file.available()) {
          buf = file.readStringUntil(terminator);
          Serial.print("Sent ");
          Serial.println(buf);
          pTransmit->setValue(buf.c_str());
          pTransmit->notify();

          // waiting for reader to say they've received all the data
          while(strcmp((const char *)pTransmit->getData(), "GOOD")) {
            // Serial.println("WAITING UNTIL GOOD");
            delay(1);
          }

          delay(5);  // bluetooth stack will go into congestion, if too many packets
                    // are sent, in 6 hours test i was able to go as low as 3ms
        }
        Serial.print("Attempting to delete ");
        Serial.println(file.name());
        // delete file upon completion of transmission
        deleteFile(SD, file.name());
        pTransmit->setValue("DONE");
        pTransmit->notify();
        
      }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      delay(500);  // give the bluetooth stack the chance to get things read
      Serial.println("stopped transmitting, waiting to start reading data from BLE");
      pLeft->setValue("START");
      pLeft->notify();
      pRight->setValue("START");
      pServer->startAdvertising();  // start advertising
      pLeft->notify();
      oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  
  delay(67); // Delay half a second between loops.
} // End of loop
