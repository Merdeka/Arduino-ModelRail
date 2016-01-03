/************************************************************************************************************
 *
 *  Copyright (C) 2015-2016 Timo Sariwating
 *  Edit by Septillion (Timo Engelgeer) January 2, 2016
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

Libraries:
Uses the Bounce2 an Loconet libraries. They need to be installed in order to compile.
Bounce2: https://github.com/thomasfredericks/Bounce2
Loconet: https://github.com/mrrwa/LocoNet

/************************************************************************************************************/
#include <LocoNet.h>
#include <Bounce2.h>

//Buttons
const byte RedButtonPin   = 3;    //of NC type
const byte GreenButtonPin = 2;  //of NO type

//LEDs
const byte RedLed   = 6;
const byte GreenLed = 5;

// Loconet
#define LOCONET_TX_PIN 7
static lnMsg *LnPacket;
static LnBuf  LnTxBuffer;

// Variables will change:
boolean OPCSTATE = 0;   //For state control center (on/off)

Bounce buttonRed, buttonGreen; //The button objects of Bounce2 library


/*************************************************************************/
/*          Setup                                                        */
/*************************************************************************/ 
void setup() {
  
  // Setup the buttons
  buttonRed.attach(RedButtonPin, INPUT_PULLUP);
  buttonGreen.attach(GreenButtonPin, INPUT_PULLUP);
  
  // Set up the outputs
  pinMode(RedLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);
  
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
  
  // Check LocoNet for a power state update
  checkLocoNet();
  
  // Set the LEDs
  setLed();  
}

/*************************************************************************/
/*          Read the Red and Green Buttons                               */
/*************************************************************************/ 
void readButtons() {
  //Read the actual buttons
  buttonRed.update();
  buttonGreen.update();
  
  //check for press of the red button (rose, because of the NC type)
  if(buttonRed.rose()){
    sendOPC_GP(0);  //Send new state to controle center
    OPCSTATE = 0;   //and save the new state
  }
  //Check is the green button became pressed (fell, because of NO type)
  else if(buttonGreen.fell()){
    sendOPC_GP(1);  //Send new state to controle center
    OPCSTATE = 1;   //and save the new state
  }
}

/*************************************************************************/
/*          Read the Red and Green Buttons                               */
/*************************************************************************/ 
void checkLocoNet() {
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket )
  {
    if (LnPacket->sz.command == OPC_GPON)  {        // GLOBAL power ON request 0x83     
      OPCSTATE = 1;
    } else if (LnPacket->sz.command == OPC_GPOFF) { // GLOBAL power OFF req 0x82
      OPCSTATE = 0;
    }
  }
}


/*************************************************************************/
/*          Set the Red and Green Leds                                  */
/*************************************************************************/ 
void setLed() {
  digitalWrite(RedLed, !OPCSTATE);
  digitalWrite(GreenLed, OPCSTATE);
}
