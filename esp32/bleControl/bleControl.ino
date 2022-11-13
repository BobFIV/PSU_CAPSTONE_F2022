#include "Arduino.h"
#include "Stream.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


#define DEVICE_LETTER "C"
#define DEVICE_NAME "Intersection" DEVICE_LETTER

//GPIO pins for our light control
// Note: These are not physical pin numbers, these are GPIO pin numbers. Example: r1Pin is set to GPIO23, which is physical pin #37
int r1Pin = 23;
int y1Pin = 22;
int g1Pin = 21;
int r2Pin = 19;
int y2Pin = 18;
int g2Pin = 5;

class BLESerial: public Stream
{
    public:

        BLESerial(void);
        ~BLESerial(void);

        bool begin(char* localName="IoT-Bus UART Service");
        int available(void);
        int peek(void);
        bool connected(void);
        int read(void);
        size_t write(uint8_t c);
        size_t write(const uint8_t *buffer, size_t size);
        void flush();
        void end(void);

    private:
        String local_name;
        BLEServer *pServer = NULL;
        BLEService *pService;
        BLECharacteristic * pTxCharacteristic;
        bool deviceConnected = false;
        uint8_t txValue = 0;
        
        std::string receiveBuffer;

        friend class BLESerialServerCallbacks;
        friend class BLESerialCharacteristicCallbacks;

};

class BLESerialServerCallbacks: public BLEServerCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onConnect(BLEServer* pServer) {
        // do anything needed on connection
        Serial.println("Device connected!");
        delay(1000); // wait for connection to complete or messages can be lost
    };

    void onDisconnect(BLEServer* pServer) {
        pServer->startAdvertising(); // restart advertising
        Serial.println("Started advertising due to disconnect!");
    }
};

class BLESerialCharacteristicCallbacks: public BLECharacteristicCallbacks {
    friend class BLESerial; 
    BLESerial* bleSerial;
    
    void onWrite(BLECharacteristic *pCharacteristic) {
 
      bleSerial->receiveBuffer = bleSerial->receiveBuffer + pCharacteristic->getValue();
    }

};

// Constructor

BLESerial::BLESerial()
{
  // create instance  
  receiveBuffer = "";
}

// Destructor

BLESerial::~BLESerial(void)
{
    // clean up
}

// Begin bluetooth serial

bool BLESerial::begin(char* localName)
{
    // Create the BLE Device
    BLEDevice::init(localName);

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    if (pServer == nullptr)
        return false;
    
    BLESerialServerCallbacks* bleSerialServerCallbacks =  new BLESerialServerCallbacks(); 
    bleSerialServerCallbacks->bleSerial = this;      
    pServer->setCallbacks(bleSerialServerCallbacks);

    // Create the BLE Service
    pService = pServer->createService(SERVICE_UUID);
    if (pService == nullptr)
        return false;

    // Create a BLE Characteristic
    pTxCharacteristic = pService->createCharacteristic(
                                            CHARACTERISTIC_UUID_TX,
                                            BLECharacteristic::PROPERTY_NOTIFY
                                        );
    if (pTxCharacteristic == nullptr)
        return false;                    
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                                                CHARACTERISTIC_UUID_RX,
                                                BLECharacteristic::PROPERTY_WRITE
                                            );
    if (pRxCharacteristic == nullptr)
        return false; 

    BLESerialCharacteristicCallbacks* bleSerialCharacteristicCallbacks =  new BLESerialCharacteristicCallbacks(); 
    bleSerialCharacteristicCallbacks->bleSerial = this;  
    pRxCharacteristic->setCallbacks(bleSerialCharacteristicCallbacks);

    // Start the service
    pService->start();
    Serial.println("starting service");

    // Start advertising
    pServer->getAdvertising()->addServiceUUID(pService->getUUID()); 
    pServer->getAdvertising()->start();
    Serial.println("Waiting a client connection to notify...");
    return true;
}

int BLESerial::available(void)
{
    // reply with data available
    return receiveBuffer.length();
}

int BLESerial::peek(void)
{
    // return first character available
    // but don't remove it from the buffer
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        return c;
    }
    else
        return -1;
}

bool BLESerial::connected(void)
{
    // true if connected
    if (pServer->getConnectedCount() > 0)
        return true;
    else 
        return false;        
}

int BLESerial::read(void)
{
    // read a character
    if ((receiveBuffer.length() > 0)){
        uint8_t c = receiveBuffer[0];
        receiveBuffer.erase(0,1); // remove it from the buffer
        return c;
    }
    else
        return -1;
}

size_t BLESerial::write(uint8_t c)
{
    // write a character
    uint8_t _c = c;
    pTxCharacteristic->setValue(&_c, 1);
    pTxCharacteristic->notify();
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    return 1;
}

size_t BLESerial::write(const uint8_t *buffer, size_t size)
{
    // write a buffer
    for(int i=0; i < size; i++){
        write(buffer[i]);
  }
  return size;
}

void BLESerial::flush()
{
    // remove buffered data
    receiveBuffer.clear();
}

void BLESerial::end()
{
    // close connection
}

//Instance of the class that handles bt communication
BLESerial bt;

void setup() {
  
  Serial.begin(115200);

  // Set LED as output
  pinMode(r1Pin, OUTPUT);
  pinMode(g1Pin, OUTPUT);
  pinMode(y1Pin, OUTPUT);
  pinMode(r2Pin, OUTPUT);
  pinMode(g2Pin, OUTPUT);
  pinMode(y2Pin, OUTPUT);

  digitalWrite(r1Pin, HIGH);
  digitalWrite(y1Pin, LOW);
  digitalWrite(g1Pin, LOW);
  digitalWrite(r2Pin, HIGH);
  digitalWrite(y2Pin, LOW);
  digitalWrite(g2Pin, LOW);
  
  //Bluetooth device name
  bt.begin(DEVICE_NAME);

  //Serial.println("Device started!");
}

String message = "";

void loop() {

  //If the BT server has given us a message
  if(bt.available()) {

    //Read incoming byte
    char incomingChar = bt.read();

    //if it is the end of a word, we can clear it. Else, build the string
    if(incomingChar != ';') {
      message += String(incomingChar);
    } else {
      message = "";
    }

    //Serial.write(incomingChar);
  }

  if(message == "red1") {
    digitalWrite(r1Pin, HIGH);
    digitalWrite(y1Pin, LOW);
    digitalWrite(g1Pin, LOW);
  } 
  else if (message == "green1") {   
    digitalWrite(r1Pin, LOW);
    digitalWrite(y1Pin, LOW);
    digitalWrite(g1Pin, HIGH);
  
  }
  else if (message == "yellow1") {
    digitalWrite(r1Pin, LOW);
    digitalWrite(y1Pin, HIGH);
    digitalWrite(g1Pin, LOW);
    
  } else if (message == "off1") {
    digitalWrite(r1Pin, LOW);
    digitalWrite(y1Pin, LOW);
    digitalWrite(g1Pin, LOW);
  }
  else if(message == "red2") {
    digitalWrite(r2Pin, HIGH);
    digitalWrite(y2Pin, LOW);
    digitalWrite(g2Pin, LOW);
  } 
  else if (message == "green2") {   
    digitalWrite(r2Pin, LOW);
    digitalWrite(y2Pin, LOW);
    digitalWrite(g2Pin, HIGH);
  }
  else if (message == "yellow2") {
    digitalWrite(r2Pin, LOW);
    digitalWrite(y2Pin, HIGH);
    digitalWrite(g2Pin, LOW);
    
  } else if (message == "off2") {
    digitalWrite(r2Pin, LOW);
    digitalWrite(y2Pin, LOW);
    digitalWrite(g2Pin, LOW);
  }
}
