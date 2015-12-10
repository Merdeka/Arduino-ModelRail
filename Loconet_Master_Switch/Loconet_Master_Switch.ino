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
This is a Loconet OPC_GPON and OPC_GPOFF Switch
- It uses an Arduino Pro Mini 5v 16MHz

Loconet:
- TX pin D7
- RX pin D8
You MUST connect the RX input to the AVR ICP pin which on an Arduino UNO is digital pin 8.
The TX output can be any Arduino pin, but the LocoNet library defaults to digital pin 6 for TX

Buttons:
- Red Button      D3
- Green Button    D2

The Red button is NC
The Green Butten is NO

LEDs:
- Red LED         D6
- Green LED       D5
/************************************************************************************************************/
#include <LocoNet.h>

#define REDBUTTON 3
#define GREENBUTTON 2

#define REDLED 6
#define GREENLED 5

// Loconet
#define LOCONET_TX_PIN 7
static lnMsg *LnPacket    ;
static LnBuf  LnTxBuffer  ;

// Variables will change:
int buttonStateRed;
int buttonStateGreen; 
int lastButtonStateRed = LOW;
int lastButtonStateGreen = HIGH;

boolean OPCSTATE = 0;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTimeRed = 0;    // the last time the output pin was toggled
long lastDebounceTimeGreen = 0;  // the last time the output pin was toggled
long debounceDelay = 50;         // the debounce time; increase if the output flickers


/*************************************************************************/
/*          Setup                                                        */
/*************************************************************************/ 
void setup() {
  
  // Set the in and outputs
  pinMode(REDBUTTON,INPUT_PULLUP);
  pinMode(GREENBUTTON,INPUT_PULLUP);
  
  pinMode(REDLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  
  // Initialize the LocoNet interface
  LocoNet.init(LOCONET_TX_PIN); // The TX output in LocoNet library defaults to digital pin 6 for TX
  
}

/*************************************************************************/
/*          Send OPC_GP                                                  */
/*************************************************************************/ 
void sendOPC_GP(byte on) {
        lnMsg SendPacket;
        if (on) {
            SendPacket.data[ 0 ] = OPC_GPON;  
        } else {
            SendPacket.data[ 0 ] = OPC_GPOFF;  
        }
        LocoNet.send( &SendPacket ) ;
}

/*************************************************************************/
/*          Program Loop                                                 */
/*************************************************************************/ 
void loop() {
  
  // Read the Buttons
  readButtons();
  
  // Set the LEDs
  setLed();
  
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket )
  {
    unsigned char opcode = (int)LnPacket->sz.command;

        if (opcode == OPC_GPON)  {        // GLOBAL power ON request 0x83     
          OPCSTATE = 1;
        } else if (opcode == OPC_GPOFF) { // GLOBAL power OFF req 0x82
          OPCSTATE = 0;
        } else {
            // ignore the message...
        }
  }
  
}

/*************************************************************************/
/*          Read the Red and Green Buttons                               */
/*************************************************************************/ 
void readButtons() {
  
  // read the state of the switch into a local variable:
  int readingRed = digitalRead(REDBUTTON);
  int readingGreen = digitalRead(GREENBUTTON);

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (readingRed != lastButtonStateRed) {
    // reset the debouncing timer
    lastDebounceTimeRed = millis();
    
    if (buttonStateRed == HIGH) {
      sendOPC_GP(0);
      OPCSTATE = 0;
    }
  } 
  
  if (readingGreen != lastButtonStateGreen) {
    // reset the debouncing timer
    lastDebounceTimeGreen = millis();
    
    if (buttonStateGreen == LOW) {
      sendOPC_GP(1);
      OPCSTATE = 1;
    }
  } 
  
  if ((millis() - lastDebounceTimeRed) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (readingRed != buttonStateRed) {
      buttonStateRed = readingRed;
    }
  }
  
  if ((millis() - lastDebounceTimeGreen) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (readingGreen != buttonStateGreen) {
      buttonStateGreen = readingGreen;
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonStateRed = readingRed;
  lastButtonStateGreen = readingGreen;
  
}


/*************************************************************************/
/*          Set the Read and Green Leds                                  */
/*************************************************************************/ 
void setLed() {
  switch (OPCSTATE) {
    case 0:
      digitalWrite(REDLED, HIGH);
      digitalWrite(GREENLED, LOW);
      break;
    case 1:
      digitalWrite(REDLED, LOW);
      digitalWrite(GREENLED, HIGH);
      break;
  }
}
