/*
   BLUETOOTH SERVER

   A connect hander associated with the server starts a background task that
   performs notification every couple of seconds.
*/
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

BLEServer* pServer = NULL;
BLECharacteristic* pLeft = NULL;
BLECharacteristic* pRight = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

Adafruit_BNO055 bnoquad = Adafruit_BNO055(55,0x28);
Adafruit_BNO055 bnoshin = Adafruit_BNO055(56,0x29);

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "a26972ba-affb-420f-8b53-0db2d9124395"
#define LEFT_UUID "7b0eb53c-a873-466f-bc1c-1ff732f08957"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup_bno055() {
  if(!bnoquad.begin()) //init bnoquad
  {
    Serial.print("Ooops, no BNO055.1 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  if(!bnoshin.begin()) //init bnoshin
  {
    Serial.print("Ooops, no BNO055.2 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  delay(1000);
  bnoquad.setExtCrystalUse(true);
  bnoshin.setExtCrystalUse(true);
}

void setup() {
  Serial.begin(115200);

  // setup_bno055();

  // Create the BLE Device
  BLEDevice::init("STRIDESYNCLEGCPU");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pLeft = pService->createCharacteristic(
      LEFT_UUID, BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pLeft->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(
      0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // notify changed value
  if (deviceConnected) {
    // sensors_event_t event; //new sensor event
    // bnoquad.getEvent(&event); //read bnoquad event
    // sensors_vec_t quad_ori;
    // memcpy(&quad_ori, &event.orientation, sizeof(sensors_vec_t));
    // bnoshin.getEvent(&event); //read bnoshin event
    // sensors_vec_t shin_ori;
    // memcpy(&shin_ori, &event.orientation, sizeof(sensors_vec_t));
    // Serial.println("Shin orientation: ");
    // Serial.print(shin_ori.x);
    // Serial.print(", ");
    // Serial.print(shin_ori.y);
    // Serial.print(", ");
    // Serial.println(shin_ori.z);
    char buf_to_send[80];
    // sprintf(buf_to_send, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", shin_ori.x, shin_ori.y, shin_ori.z, quad_ori.x, quad_ori.y, quad_ori.z);
    sprintf(buf_to_send, "180,180,180,180,180,180");
    pLeft->setValue(buf_to_send);
    pLeft->notify();

    delay(67);  // bluetooth stack will go into congestion, if too many packets
               // are sent, in 6 hours test i was able to go as low as 3ms
    value++;
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("Connected");
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}