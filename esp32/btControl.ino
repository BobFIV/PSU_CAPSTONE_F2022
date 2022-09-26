//Wrapper that hides the complexities of bluetooth
#include "BluetoothSerial.h"

//Ensure BT settings are enabled for device
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

//Instance of the class that handles bt communication
BluetoothSerial bt;

//GPIO pins for our light control
int rPin = 23;
int yPin = 22;
int gPin = 21;

void setup() {
  
  Serial.begin(115200);

  // Set LED as output
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(yPin, OUTPUT);

  digitalWrite(rPin, LOW);
  digitalWrite(yPin, LOW);
  digitalWrite(gPin, LOW);
  
  //Bluetooth device name
  bt.begin("LightPost1");

  Serial.println("Device started!");
}

String message = "";

void loop() {

  //If the BT server has given us a message
  if(bt.available()) {

    //Read incoming byte
    char incomingChar = bt.read();

    //if it is the end of a word, we can clear it. Else, build the string
    if(incomingChar != '\n') {
      message += String(incomingChar);
    } else {
      message = "";
    }

    Serial.write(incomingChar);
  }

  if(message == "red_on") {

    digitalWrite(rPin, HIGH);
    digitalWrite(yPin, LOW);
    digitalWrite(gPin, LOW);
  
  } 
  else if (message == "green_on") {
    
    digitalWrite(rPin, LOW);
    digitalWrite(yPin, LOW);
    digitalWrite(gPin, HIGH);
  
  }
  else if (message == "yellow_on") {

    digitalWrite(rPin, LOW);
    digitalWrite(yPin, HIGH);
    digitalWrite(gPin, LOW);
    
  } else if (message == "off") {
    
    digitalWrite(rPin, LOW);
    digitalWrite(yPin, LOW);
    digitalWrite(gPin, LOW);
    
  }
  delay(20);
}
