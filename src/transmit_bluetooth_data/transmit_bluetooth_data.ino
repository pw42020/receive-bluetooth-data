/*
   BLUETOOTH SERVER

   A connect hander associated with the server starts a background task that
   performs notification every couple of seconds.
*/
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

BLEServer* pServer = NULL;
BLECharacteristic* pLeft = NULL;
BLECharacteristic* pRight = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define LEG_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define REC_LEG_SERVICE "a26972ba-affb-420f-8b53-0db2d9124395"
#define REC_LEG_CHAR "7b0eb53c-a873-466f-bc1c-1ff732f08957"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer* pServer) { deviceConnected = false; }
};

void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("StrideSync");

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
  pRight = pService->createCharacteristic(
      RIGHT_UUID, BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pLeft->addDescriptor(new BLE2902());
  pRight->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(
      0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // notify changed value
  if (deviceConnected) {
    char buf_to_send[30];
    sprintf(buf_to_send, "%d,%d,%d,%d,%d,%d", value + 1, value + 2, value + 3,
            value + 4, value + 5, value + 6);
    pLeft->setValue(buf_to_send);
    pLeft->notify();

    pRight->setValue(buf_to_send);
    pRight->notify();
    delay(5);  // bluetooth stack will go into congestion, if too many packets
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
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}