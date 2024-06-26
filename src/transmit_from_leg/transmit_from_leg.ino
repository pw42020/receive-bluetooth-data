/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#include "BLEDevice.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

#define NUM_FLOATS 6

#define SERVERNAME "StrideSync"

#define SERVICE_UUID "a26972ba-affb-420f-8b53-0db2d9124395"
#define LEFT_UUID "7b0eb53c-a873-466f-bc1c-1ff732f08957"
#define RIGHT_UUID "f542a2e9-d11f-4e48-999c-242ef20b5876"

// The remote service we wish to connect to.
static BLEUUID serviceUUID(SERVICE_UUID);
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID(RIGHT_UUID);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// BNO055 data

Adafruit_BNO055 bnoquad = Adafruit_BNO055(55,0x29);
Adafruit_BNO055 bnoshin = Adafruit_BNO055(56,0x28);

void setup_bno055() {
  bool run = true;
  if(!bnoquad.begin()) //init bnoquad
  {
    Serial.print("Ooops, no BNO055.1 for quad detected ... Check your wiring or I2C ADDR!");
    run = false;
  }
  if(!bnoshin.begin()) //init bnoshin
  {
    Serial.print("Ooops, no BNO055.2 for shin detected ... Check your wiring or I2C ADDR!");
    run = false;
  }
  if (!run) {
    while(1) {
      imu::Quaternion shin_quat = bnoshin.getQuat();

      Serial.println("shin orientation: ");
      Serial.print(shin_quat.w());
      Serial.print(", ");
      Serial.print(shin_quat.x());
      Serial.print(", ");
      Serial.print(shin_quat.y());
      Serial.print(", ");
      Serial.println(shin_quat.z());

      delay(50);
    }
  } else {
    Serial.println("GOOD!!!!");
  }
  delay(1000);
  bnoquad.setExtCrystalUse(true);
  bnoshin.setExtCrystalUse(true);
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.write(pData, length);
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
   if (advertisedDevice.getName() != "") {
      Serial.print("BLE Advertised Device found: Name: ");
      Serial.print(advertisedDevice.getName().c_str());
      Serial.print(", Address: ");
      Serial.println(advertisedDevice.getAddress().toString().c_str());
      Serial.print("Advertised device has service UUIDs: ");
      Serial.println(advertisedDevice.haveServiceUUID());
    }
    if (advertisedDevice.getName() == SERVERNAME) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      doScan = true;
      Serial.println("Device found. Connecting!");
    }
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);

  // while (!Serial);
  
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  setup_bno055();

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.

void float2Bytes(byte bytes_temp[4],float float_variable){ 
  memcpy(bytes_temp, (unsigned char*) (&float_variable), 4);
}

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);

    // get quaternion data, convert to Euler
    // @note: Do this to get around Euler issues with BNO055, forums say this should work fine
    imu::Quaternion quad_quat = bnoquad.getQuat();
    imu::Quaternion shin_quat = bnoshin.getQuat();

    Serial.println("quad orientation: ");
    Serial.print(quad_quat.w());
    Serial.print(", ");
    Serial.print(quad_quat.x());
    Serial.print(", ");
    Serial.print(quad_quat.y());
    Serial.print(", ");
    Serial.println(quad_quat.z());

    imu::Vector<3> quad_ori = quad_quat.toEuler();
    imu::Vector<3> shin_ori = shin_quat.toEuler();

    // quad_ori.normalize();
    // shin_ori.normalize();

    Serial.println("Shin orientation: ");
    Serial.print(shin_ori.x()*RAD_TO_DEG);
    Serial.print(", ");
    Serial.print(shin_ori.y()*RAD_TO_DEG);
    Serial.print(", ");
    Serial.println(shin_ori.z()*RAD_TO_DEG);

    Serial.println("Quad orientation: ");
    Serial.print(quad_ori.x()*RAD_TO_DEG);
    Serial.print(", ");
    Serial.print(quad_ori.y()*RAD_TO_DEG);
    Serial.print(", ");
    Serial.println(quad_ori.z()*RAD_TO_DEG);

    // initialize list of bytes that will be transmitted
    byte orientation_bytes[sizeof(float)*NUM_FLOATS] = {};

    float floats_to_send[NUM_FLOATS] = {shin_ori.x(), shin_ori.y(), shin_ori.z(), quad_ori.x(), quad_ori.y(), quad_ori.z()};

    // adding each value to buffer
    for (int8_t ii; ii < NUM_FLOATS; ii++) {
      byte* orientation_float = &orientation_bytes[sizeof(float)*ii];
      float2Bytes(orientation_float, floats_to_send[ii]);
    }

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(orientation_bytes, sizeof(float)*NUM_FLOATS);
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(33); // Sample rate of 15Hz
} // End of loop
