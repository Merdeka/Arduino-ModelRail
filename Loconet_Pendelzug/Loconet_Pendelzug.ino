/************************************************************************************************************
 *
 *  Copyright (C) 2017 Timo Sariwating
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
This is a Loconet Pendelzugsteuerung
- It uses an Arduino Pro Mini 5v 16MHz

Loconet:
- TX pin D7
- RX pin D8
You MUST connect the RX input to the AVR ICP pin which on an Arduino UNO is digital pin 8.
The TX output can be any Arduino pin, but the LocoNet library defaults to digital pin 6 for TX

Libraries:
Uses the Loconet librarie. It needs to be installed in order to compile.
Loconet: https://github.com/mrrwa/LocoNet

 * State 1 = Stop in Station A
 * State 2 = Running between A and B
 * State 3 = Break for Station B
 * State 4 = Stop in Station B
 * State 5 = Running between B and A
 * State 6 = Break for Station A

/************************************************************************************************************/
#include <LocoNet.h>

// Loconet
#define LOCONET_TX_PIN 7
static lnMsg *LnPacket;
uint8_t UB_SRC_MASTER = 0x00;

// Variables will change:
uint8_t mySlot;
uint8_t runState      = 0;
boolean runPendel     = false;
boolean sendCommand   = false;
unsigned long waitTimer, updateTimer;

// Change these wait timer to your preference
uint16_t MIN_WAIT =  5;
uint16_t MAX_WAIT = 20;

// Change This to the Locomotive Address
uint16_t Address      = 28;

// Define your feedback Addresses here
#define Run            13     // Change This to feedback Address for the Run Button
#define Stop           14     // Change This to feedback Address for the Stop Button
#define SensorBrakeA   12     // Change This to feedback Address for Station A Breaking
#define SensorStopA    11     // Change This to feedback Address for Station A Stop
#define SensorBrakeB   15     // Change This to feedback Address for Station B Breaking
#define SensorStopB    16     // Change This to feedback Address for Station B Stop

/******************************************************************************/
/*          Setup                                                             */
/******************************************************************************/
void setup() {
  
  // First initialize the LocoNet interface
  LocoNet.init(LOCONET_TX_PIN);

  // Configure the serial port for 115200 baud
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Loconet Pendelzugsteuerung"));

  LocoNet.send( OPC_LOCO_ADR, (uint8_t) ( Address >> 7 ), (uint8_t) ( Address & 0x7F ) );
  
} //Setup


/******************************************************************************/
/*          Program Loop                                                      */
/******************************************************************************/
void loop() {

  if(runPendel == true) {

    //-------------------------------------------------------------------------------------------
    // Timed Code
    //-------------------------------------------------------------------------------------------
    // Every 1 Second
    if ((millis()-updateTimer)>1000) { // 1 Second

      if (waitTimer != 0) {
        Serial.print("Wait Time - ");
        Serial.print(waitTimer, DEC);
        Serial.println(" Seconds");
      }

      if(waitTimer == 0) {
        if(runState == 1) {
          runState = 2; // Starting from Station A
          sendCommand = true;
        } else if (runState == 4) {
          runState = 5; // Starting from Station B
          sendCommand = true;
        }       
      } else {
        waitTimer--;
      }
      updateTimer = millis();
    }
    
    if(sendCommand == true) {
      
      switch (runState) {
        case 0: // Train not in Station A or B
          Serial.println(F("Train not in Station A or B"));
          sendCommand = false;
          break;
        case 1: // Stop in Station A
          LocoNet.send( OPC_LOCO_DIRF, mySlot, 0x00);     // Locomotive Forward Direction & Lights Off
          LocoNet.send( OPC_LOCO_SPD, mySlot, 0 );        // Locomotive Speed 0
          Serial.println(F("Stop in Station A"));
          waitTimer = getNewWaitTimer();
          sendCommand = false;
          break;
        case 2: // Running between A and B
          LocoNet.send( OPC_LOCO_DIRF, mySlot, 0x10);     // Locomotive Forware Direction & Lights On
          LocoNet.send( OPC_LOCO_SPD, mySlot, 50 );       // Locomotive Speed 50
          Serial.println(F("Running from Station A to B"));
          sendCommand = false;
          break;
        case 3: // Break for Station B
          LocoNet.send( OPC_LOCO_SPD, mySlot, 15 );
          Serial.println(F("Breaking for Station B"));
          sendCommand = false;
          break;
        case 4: // Stop in Station B
          LocoNet.send( OPC_LOCO_DIRF, mySlot, 0x20);     // Locomotive Reverse Direction  & Lights Off
          LocoNet.send( OPC_LOCO_SPD, mySlot, 0 );        // Locomotive Speed 0
          Serial.println(F("Stop in Station B"));
          waitTimer = getNewWaitTimer();
          sendCommand = false;
          break;
        case 5: // Running between B and A
          LocoNet.send( OPC_LOCO_DIRF, mySlot, 0x30);     // Locomotive Reverse Direction & Lights On
          LocoNet.send( OPC_LOCO_SPD, mySlot, 50 );       // Locomotive Speed 50
          Serial.println(F("Running from Station B to A"));
          sendCommand = false;
          break;
        case 6: // Break for Station A
          LocoNet.send( OPC_LOCO_SPD, mySlot, 15 );
          Serial.println(F("Breaking for Station A"));
          sendCommand = false;
          break;
      }
      
    } else {
      // Pendel not Running
    }
  }
  
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if ( LnPacket ) {

    switch (LnPacket -> data[0]) {
      case OPC_GPON: // Request status on GLOBAL power ON request 0x83
        Serial.println(F("[LOCONET]: OPC_GPON"));
        digitalWrite(LED_BUILTIN, HIGH);
        runState = 0;
        sendIBRequest();
        runPendel = true;
        sendCommand = true;
        break;
      case OPC_GPOFF:
        Serial.println(F("[LOCONET]: OPC_GPOFF"));
        digitalWrite(LED_BUILTIN, LOW);
        runPendel = false;
        break;
      case OPC_PEER_XFER:
        if( LnPacket -> data[1] == 0x0F && LnPacket -> data[2] == UB_SRC_MASTER && LnPacket -> data[3] == 0x49 && LnPacket -> data[4] == 0x4B ) {
          int saddr     = 0;
          byte pxct     = LnPacket -> data[6];
          byte d6       = LnPacket -> data[12];
          byte d7       = LnPacket -> data[13];
          uint16_t addr = LnPacket -> data[7]*16;

          /* move in the high bit: */
          d6 = d6 | ((pxct&0x20)?0x80:0x00);
          d7 = d7 | ((pxct&0x40)?0x80:0x00);

          // S88 Port 01-08
          for( saddr = 0; saddr < 8; saddr++ ) {
            notifySensor( addr+(7-saddr)+1, d6 & (0x01 << saddr) );
          }

          // S88 Port 09-16
          addr  = LnPacket -> data[7]*16 + 8;
          for( saddr = 0; saddr < 8; saddr++ ) {
            notifySensor( addr+(7-saddr)+1, d7 & (0x01 << saddr) );
          }
        }
      default:
        // If this packet was not a Switch or Sensor Message then print a new line
        if (!LocoNet.processSwitchSensorMessage(LnPacket)) {
      
          // Check if Received Address in Slot is Same as our Address
          if( LnPacket->sd.command == OPC_SL_RD_DATA && Address == (uint16_t) (( LnPacket->sd.adr2 << 7 ) + LnPacket->sd.adr )) {
             mySlot = LnPacket->sd.slot;
             Serial.print(F("Loco Address: "));
             Serial.print(Address);
             Serial.print(F(" | Loconet Slot: "));
             Serial.println(mySlot);
          }
          
        }
    }
    
  }
} //Loop

/******************************************************************************/
/* This call-back function is called from LocoNet.processSwitchSensorMessage  */
/* for all Sensor messages                                                    */
/******************************************************************************/ 
void notifySensor( uint16_t Address, uint8_t State ) {

  switch (Address) {
    case Run:
      if(State != 0) {
        runPendel = true;
        Serial.println(F("Run Pendelzug"));
      }
      break;
    case Stop:
      if(State != 0) {
        runPendel = false;
        Serial.println(F("Stop Pendelzug"));
        break;
      }
    case SensorStopA:
      if(State != 0) {
        runState = 1;
        sendCommand = true;    
      }
      break;
    case SensorBrakeB:
      if(State != 0  && runState == 2) {
        runState = 3;
        sendCommand = true;
      }
      break;
    case SensorStopB:
      if(State != 0) {
        runState = 4;
        sendCommand = true;
      }
      break;
    case SensorBrakeA:
      if(State != 0 && runState == 5) {
        runState = 6;
        sendCommand = true;
      }
      break;
    default:
      break;
  }

/*
  Serial.print("Sensor: ");
  Serial.print(Address, DEC);
  Serial.print(" - ");
  Serial.println( State ? "Active" : "Inactive" );
*/
  
} //notifySensor

/*************************************************************************/
/*         Send Intellibox Sensor Status Request                         */
/*************************************************************************/
void sendIBRequest() {

  // Rocrail http://bazaar.launchpad.net/~rocrail-project/rocrail/Rocrail/view/head:/rocdigs/impl/loconet.c
  // Line 477

  /*
    SRC = 1 (KPU)
    DSTL/H = "I"/"B" = Intellibox (SPU) (i.e., 0x49 and 0x42)
    ReqId = 19 0x13 (same as you write)
    PXCT1 = 0 (unless you have an s88 module # higher than 128!)
    D1 = s88 module # (minus 1), i.e. 0 for the 1st s88 module
    D2..D7 = 0
  */
  
  lnMsg SendPacket;
  
  SendPacket.data[ 0 ] = OPC_IMM_PACKET;
  SendPacket.data[ 1 ] = 0xF;
  SendPacket.data[ 2 ] = 0x1;     /* SRC   */
  SendPacket.data[ 3 ] = 0x49;    /* DSTL  */
  SendPacket.data[ 4 ] = 0x42;    /* DSTH  */
  SendPacket.data[ 5 ] = 0x13;    /* ReqID */
  SendPacket.data[ 6 ] = 0x0;     /* PXCT1 */
  SendPacket.data[ 7 ] = 0x0;     /* D1    */  // Change This to Your own feedback if needed
  SendPacket.data[ 8 ] = 0x0;     /* D2    */
  SendPacket.data[ 9 ] = 0x0;     /* D3    */
  SendPacket.data[ 10 ] = 0x0;    /* D4    */
  SendPacket.data[ 11 ] = 0x0;    /* D5    */
  SendPacket.data[ 12 ] = 0x0;    /* D6    */
  SendPacket.data[ 13 ] = 0x0;    /* D7    */
  
  LocoNet.send( &SendPacket ) ;
} //sendIBRequest

/*************************************************************************/
/*        Get New Wait Timer                                             */
/*************************************************************************/
uint16_t getNewWaitTimer(){
  return random(MIN_WAIT, MAX_WAIT);
} //getNewWaitTimer

