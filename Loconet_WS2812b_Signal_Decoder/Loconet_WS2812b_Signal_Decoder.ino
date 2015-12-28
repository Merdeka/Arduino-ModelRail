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
  This is a LocoNet Signal Decoder
  - It uses Ethernet UDP Multicast using a W5100 Ethernet Card.

  Demonstrates the use of the:
  - LocoNet.processSwitchSensorMessage(LnPacket) function to switch signals
  - Using WS2812b as simple signals witch Adafruit NeoPixel library

  In this example 4 WS2812b are combined into two DB Ausfahrsignals. It uses two addresses per signal always
  starting from the odd/uneven address.
  Addr1 Aaddr2| Aspect | Signalbegriff
  ------------------------------------
  R    - R | 00 | HP0 -> RED/RED
  G    - R | 01 | HP1 -> GREEN
  R    - G | 02 | HP2 -> GREEN/YELLOW
  G    - G | 03 | SH1 -> RED/WHITE

/************************************************************************************************************/
#include <LocoNet.h>
#include <Adafruit_NeoPixel.h>

// Loconet
  #define LOCONET_TX_PIN 7
  static lnMsg *LnPacket    ;
  static LnBuf  LnTxBuffer  ;
  
  uint16_t signalStartAddress[] = { 1, 3 }; // Only Even Numbers as Start Address!!
  uint8_t numberOfSignals = sizeof(signalStartAddress) / 2;
  
  byte signalState[sizeof(signalStartAddress) / 2];

// WS2812b
  // Which pin on the Arduino is connected to the NeoPixels?
  #define PIN            6

  // How many NeoPixels are attached to the Arduino?
  #define NUMPIXELS      4

  // When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
  Adafruit_NeoPixel Signals = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

/*************************************************************************/
/*          Setup                                                        */
/*************************************************************************/ 
void setup() {

  pinMode(13, OUTPUT);

  Signals.begin(); // This initializes the NeoPixel library.
  Signals.setBrightness(32);

  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  Serial.println("LocoNet UDP Multicast Signal Decoder");

  Serial.print(numberOfSignals);
  Serial.print(" Signals: ");

  for (int i=0; i<numberOfSignals; i++) {
    Serial.print(signalStartAddress[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Initialize the LocoNet interface
  LocoNet.init(LOCONET_TX_PIN); // The TX output in LocoNet library defaults to digital pin 6 for TX

}

/*************************************************************************/
/*          Update Signal and send to appropriate signal tech            */
/*************************************************************************/ 
void updateSignal(uint16_t Signal, uint8_t signalState) {

      Serial.print("Signal ");
      Serial.print(Signal + 1);
      Serial.print(": Address ");
      Serial.print(signalStartAddress[Signal]);
      Serial.print("-");
      Serial.print(signalStartAddress[Signal] + 1);
      Serial.print(" State ");

      switch (signalState) {
      case 0x00:
        Serial.println("0->RED");
        break;
      case 0x01:
        Serial.println("1->GREEN");
        break;
      case 0x02:
        Serial.println("2->YELLOW");
        break;
      case 0x03:
        Serial.println("3->WHITE");
        break;
    }
    updateWS282bSignal(signalStartAddress[Signal], signalState);
}


/*************************************************************************/
/*          Set WS2812b Signal to Aspect                                 */
/*************************************************************************/ 
void updateWS282bSignal(uint16_t Address, uint8_t State) {

  for(int i=0;i<numberOfSignals;i++) {
    
    if(Address == signalStartAddress[i]) {
      Serial.print("Signal: ");
      Serial.print(Address);
      Serial.print(":");
      Serial.print(i);
      Serial.print(" -> ");
      Serial.println(State);

      int c = i*2;
  
       switch (State) {
        case 0: //RED
          Signals.setPixelColor(c    , Signals.Color(255,   0,   0));
          Signals.setPixelColor(c + 1, Signals.Color(255,   0,   0));
          break;
        case 1: //GREEN
          Signals.setPixelColor(c    , Signals.Color(  0, 150,   0));
          Signals.setPixelColor(c + 1, Signals.Color(  0,   0,   0));
          break;
        case 2: //YELLOW
          Signals.setPixelColor(c    , Signals.Color(  0, 150,   0));
          Signals.setPixelColor(c + 1, Signals.Color(255, 190,   0));
          break;
        case 3: //WHITE
          Signals.setPixelColor(c    , Signals.Color(255,   0,   0));
          Signals.setPixelColor(c + 1, Signals.Color(255, 255, 255));
          break;
      }
      Signals.show(); // Send update to WS2812b
    }
  }
}

/*************************************************************************/
/*          Program Loop                                                 */
/*************************************************************************/ 
void loop() {

  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket )
  {
    unsigned char opcode = (int)LnPacket->sz.command;

        if (opcode == OPC_GPON)  {        // GLOBAL power ON request 0x83     
          digitalWrite(13, HIGH);
        } else if (opcode == OPC_GPOFF) { // GLOBAL power OFF req 0x82
          digitalWrite(13, LOW);
        } else {
          if(!LocoNet.processSwitchSensorMessage(LnPacket));
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

    for (int i=0; i<numberOfSignals; i++) {
      
      if (Address == signalStartAddress[i] || Address == signalStartAddress[i]+1) {
        
        switch (Direction) {
          case 0x00: // Thrown
            if ( (Address & 0x01) == 0) { // Even Address
              bitSet(signalState[i], 1);
              updateSignal(i, signalState[i]);
            } else {                // Odd Address
              bitSet(signalState[i], 0);
            }
            break;
          case 0x20: // Closed
            if ( (Address & 0x01) == 0) { // Even Address
              bitClear(signalState[i], 1);
              updateSignal(i, signalState[i]);
            } else {                // Odd Address
              bitClear(signalState[i], 0);
            }
            break;
        }
      }
    }

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
/* for all Switch Report messages                                            */
/*****************************************************************************/
void notifySwitchOutputsReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {

    Serial.print("Switch Outputs Report: ");
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
}
