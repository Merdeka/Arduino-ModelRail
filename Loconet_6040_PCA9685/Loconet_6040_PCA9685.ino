/************************************************************************************************************
 *
 *  Copyright (C) 2015 Timo Sariwating
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, If not, see <http://www.gnu.org/licenses/>.
 *
 ************************************************************************************************************

DESCRIPTION:
This is a Marklin 6040 Keyboard converted to Loconet.
- The original i2c PCB is removed and replaced with an Arduino's and a 16bit i2c MCP23016 port expander.
- The PCA9685 handles all the LED's. The Arduino the Keypresses and LocoNet
- The top Marklin PCB wth 32 buttons and LEDs is reused.
- There are 2 vesions of the top PCB. The older version has 2 shift registers and does not need the MCP23016.
- The 4 pin switch on the back is reused for address selection.

- LNCV programming is on the todo list.

Loconet:
- TX pin D7
- RX pin D8
You MUST connect the RX input to the AVR ICP pin which on an Arduino UNO is digital pin 8.
The TX output can be any Arduino pin, but the LocoNet library defaults to digital pin 6 for TX

i2c:
- SDA pin A4
- SCL pin A5

Address Switch: (On Master)
- switch 1 pin D5
- switch 2 pin D4
- switch 3 pin D3
- switch 4 pin D2

LEDs:
- top    row - PCA9685 01-08
- bottom row - PCA9685 09-16
/************************************************************************************************************/
#include <EEPROM.h>
#include <Keypad_MC16.h>
#include <Keypad.h>
#include <Wire.h>
#include <LocoNet.h>
#include <Adafruit_PWMServoDriver.h>

// Debug Mode
boolean debugMode = true;

// Loconet
#define LOCONET_TX_PIN 7
static lnMsg *LnPacket;
static LnBuf  LnTxBuffer ;

// Keypad
#define I2CADDR 0x20
const byte ROWS = 4; //4 rows
const byte COLS = 8; //8 columns

//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  { 0x01 , 0x02 , 0x03 , 0x04 , 0x05 , 0x06 , 0x07 , 0x08 },
  { 0x09 , 0x0A , 0x0B , 0x0C , 0x0D , 0x0E , 0x0F , 0x10 },
  { 0x11 , 0x12 , 0x13 , 0x14 , 0x15 , 0x16 , 0x17 , 0x18 },
  { 0x19 , 0x1A , 0x1B , 0x1C , 0x1D , 0x1E , 0x1F , 0x20 },
};

byte buttonRowPins[ROWS] = {8, 9, 10, 11};             //connect to the row pinouts of the keypad
byte buttonColPins[COLS] = {7, 6, 5, 4, 3, 2, 1, 0};   //connect to the column pinouts of the keypad

int ADDRESSES[16];

boolean buttonState[16];
boolean waitForLack;
uint16_t LackAddress;

//initialize an instance of class NewKeypad
Keypad_MC16 MarklinKeypad( makeKeymap(hexaKeys), buttonRowPins, buttonColPins, ROWS, COLS, I2CADDR); 

// Adress Switch on the Back
int adrr_switch[4] = { 5, 4, 3, 2 };
byte address = 0;   // Byte to store Address (Only the 4 LSB are used)

// you can also call it with a different address you want
Adafruit_PWMServoDriver led = Adafruit_PWMServoDriver(0x43);

/*****************************************************************************/
/*          Setup                                                            */
/*****************************************************************************/
void setup() {
  
  // start the serial port
  if (debugMode) 
    Serial.begin(57600);
  
  // Read Address Pins
  for (int i = 0; i < 4; i++) {
   pinMode(adrr_switch[i], INPUT_PULLUP);
   boolean reading = !digitalRead(adrr_switch[i]);
   bitWrite(address, i, reading);
  }
  address++;

  if(EEPROM.read(0) == address) {
    Serial.println("Address matches EEPROM");
  } else {
    Serial.println("Address Changed");
    EEPROM.write(0, address);
  }
  
  // Allocate the Addresses to the Buttons
  for(int i = 0; i < 16; i++) {
    ADDRESSES[i] = (((address - 1) * 16) + (i + 1));

    // Load button states from EEPROM
    byte state = EEPROM.read(i+16);
    switch(state) {
      case 0x00:
      buttonState[i] = 1;
        break;
      case 0xFF:
      buttonState[i] = 0;
        break;
    }
  }
  
  // Initialize Keypad
  MarklinKeypad.begin( );
  MarklinKeypad.addEventListener(keypadEvent);
  MarklinKeypad.setHoldTime(500);
  MarklinKeypad.setDebounceTime(100);

  // Initialize the LED Driver
  led.begin();
  led.setPWMFreq(500);
  
  // Initialize the LocoNet interface
  LocoNet.init(LOCONET_TX_PIN); // The TX output in LocoNet library defaults to digital pin 6 for TX
  
  if (debugMode) {
    Serial.print("6040 Loconet. Address: ");
     Serial.println(address);
     Serial.println("Buttons:");
     for (int i = 0; i < 8; i++) {
       if(ADDRESSES[i] < 10) Serial.print("0");
       Serial.print(ADDRESSES[i]);
       Serial.print(" ");
     }
     Serial.println();
     for (int i = 8; i < 16; i++) {
       if(ADDRESSES[i] < 10) Serial.print("0");
       Serial.print(ADDRESSES[i]);
       Serial.print(" ");
     }
     Serial.println();
  }

  // Blink Leds One Time then blink address LED 3 times and reset Leds
    // Test All LEDS
    for (int i=0;i<16;i++){
      setLed(i, 1);
    }
    delay(750);
    for (int i=0;i<16;i++){
      setLed(i, 0);
    }
    delay(250);

    // Bink Led Matching Address
    for (int i=0;i<3;i++){
      setLed(address-1, 1);
      delay(250);
      setLed(address-1, 0);
      delay(250);
    }
  
    // Reset LEDS
    for (int i=0;i<16;i++){
      setLed(i, 0);
    }
  
  initLnBuf(&LnTxBuffer);

  getUpdate();
}

/*****************************************************************************/
/*          Program Loop                                                     */
/*****************************************************************************/
void loop() {
  
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket )
  {
    // Request status on GLOBAL power ON request 0x83
    if ( LnPacket -> data[0] == OPC_GPON ) {
      //getUpdate();
    }
    
    // First print out the packet in HEX
    Serial.print("RX: ");
    uint8_t msgLen = getLnMsgSize(LnPacket); 
    for (uint8_t x = 0; x < msgLen; x++)
    {
      uint8_t val = LnPacket->data[x];
        // Print a leading 0 if less than 16 to make 2 HEX digits
      if(val < 16)
        Serial.print('0');
        
      Serial.print(val, HEX);
      Serial.print(' ');
    }
    
    
    // If this packet was not a Switch or Sensor Message then print a new line 
    if(!LocoNet.processSwitchSensorMessage(LnPacket))
      Serial.println();
  }
  
  // Check Keypad for Button Press
  char button = MarklinKeypad.getKey();

  updateLeds();
  
}


/*****************************************************************************/
/*          Callback functions to Check Keypad                               */
/*****************************************************************************/
void keypadEvent(KeypadEvent button) {
  
  int data = (int) button;
  int buttonAddress = 0;
  boolean DIRECTION = 0;
  
  // Red Top Row
  if (data >=  1 && data <=  8) {
    buttonAddress = ADDRESSES[data - 1];
    DIRECTION = 0;
  }
  // Green Top Row
  if (data >=  9 && data <= 16) {
    buttonAddress = ADDRESSES[data - 9];
    DIRECTION = 1;
  }
  // Red Bottom Row
  if (data >= 17 && data <= 24) {
    buttonAddress = ADDRESSES[data - 9];
    DIRECTION = 0;
  }
  // Green Botton Row
  if (data >= 25 && data <= 32) {
    buttonAddress = ADDRESSES[data - 17];
    DIRECTION = 1;
  }
  
  switch (MarklinKeypad.getState()) {
      case PRESSED:
        if (debugMode) {
          Serial.print("Button ");
          Serial.print(data);
          Serial.println(" PRESSED");
        }
        LocoNet.requestSwitch(buttonAddress, 1, DIRECTION);
        processKeypad(button);
        break;
      
      case HOLD:
        if (debugMode) {
          Serial.print("Button ");
          Serial.print(data);
          Serial.println(" HOLD");
        }
        break;
      
      case RELEASED:
        if (debugMode) {
          Serial.print("Button ");
          Serial.print(data);
          Serial.println(" RELEASED");
        }
        LocoNet.requestSwitch(buttonAddress, 0, DIRECTION);
        processKeypad(button);
        break;
    }
    
}

/*****************************************************************************/
/*          Subroutine to Process Keypad Data                                */
/*****************************************************************************/
void processKeypad(char button) {
  
   int data = (int) button;
   int e;
   byte state;

  if (data >=  1 && data <=  8) { buttonState[(data -  1)] = 1; e = data -  1; state = 0xff; }
  if (data >=  9 && data <= 16) { buttonState[(data -  9)] = 0; e = data -  9; state = 0x00; }
  
  if (data >= 17 && data <= 24) { buttonState[(data -  9)] = 1; e = data -  9; state = 0xff; }
  if (data >= 25 && data <= 32) { buttonState[(data - 17)] = 0; e = data - 17; state = 0x00; }

  e = e+16;
  EEPROM.write(e, state);
}

/*****************************************************************************/
/*          Subroutine to Request Status Update                              */
/*****************************************************************************/
void getUpdate() {
  
  for(int i=0;i<16;i++) {
    LocoNet.requestSwitch(ADDRESSES[i], 0, buttonState[i]);
  }
}

/*****************************************************************************/
/*          Subroutine to check and set LEDS                                 */
/*****************************************************************************/
void updateLeds() {
  
  for (int i = 0; i < 16; i++) {
    setLed(i, buttonState[i]);
  }
}

/*****************************************************************************/
/*          Subroutine to switch LED                                         */
/*****************************************************************************/
void setLed(int LED, bool state) {
  if (state == true) {
    led.setPWM(LED, 4096, 0);
  } else {
    led.setPWM(LED, 0, 4096);
  }
}

/*****************************************************************************/
/*          Called to update Leds                                            */
/*****************************************************************************/
void processUpdate( uint16_t Address, uint8_t Direction ) {
    for(int i = 0; i < 16; i++) {
      if( Address == ADDRESSES[i] ) {
        
        switch (Direction) {
          case 0x00: // thrown(red)
            buttonState[i] = 1;
            break;
          case 0x20: // closed(green)
            buttonState[i] = 0;
            break;
          default:
            Serial.println("Unknown");
            break;
          }
      }
    }
}

/*****************************************************************************/
/* This call-back function is called from LocoNet.processSwitchSensorMessage */
/* for all Sensor messages                                                   */
/*****************************************************************************/
void notifySensor( uint16_t Address, uint8_t State ) {
  
    Serial.print("Sensor: ");
    Serial.print(Address, DEC);
    Serial.print(" - ");
    Serial.println( State ? "Active" : "Inactive" );
}

/*****************************************************************************/
/* This call-back function is called from LocoNet.processSwitchSensorMessage */
/* for all Switch Request messages                                           */
/*****************************************************************************/
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {

    Serial.print("Switch Request: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");

    processUpdate(Address, Direction);
}

/*****************************************************************************/
/* This call-back function is called from LocoNet.processSwitchSensorMessage */
/* for all Switch Report messages                                            */
/*****************************************************************************/
void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {

    Serial.print("Switch Report: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction ? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");
}

/*****************************************************************************/
/* This call-back function is called from LocoNet.processSwitchSensorMessage */
/* for all Switch State messages                                             */
/*****************************************************************************/
void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {

    Serial.print("Switch State: ");
    Serial.print(Address, DEC);
    Serial.print(':');
    Serial.print(Direction, HEX); //? "Closed" : "Thrown");
    Serial.print(" - ");
    Serial.println(Output ? "On" : "Off");

    waitForLack = 1;
    LackAddress = Address;
}

