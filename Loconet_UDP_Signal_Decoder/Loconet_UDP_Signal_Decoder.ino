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
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LocoNet.h>
#include <Adafruit_NeoPixel.h>

// Ethernet
  // The MAC address of your Ethernet board (or Ethernet Shield) is located on the back of the circuit board.
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0x02 };  // Arduino Ethernet
  EthernetClient client;
  
  // Change this to your own network
  byte ip[] = { 192,168,1,10 };
  
  // Multicast
  IPAddress multicast(224,0,0,1);
  unsigned int port = 1235;

  // An EthernetUDP instance to let us send and receive packets over UDP
  EthernetUDP Udp;

// Loconet
  static   LnBuf        LnTxBuffer;
  static   lnMsg        *LnPacket;
  
  uint16_t signalStartAddress[] = { 1, 17 };
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
  
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.beginMulticast(multicast ,port);
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer);

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
/*          Check for any received LocoNet packets                       */
/*************************************************************************/ 
void LoconetRX() {
  
  // Parse and display incoming packets
  int packetSize = Udp.parsePacket();

    if(packetSize) {    
      char packetBuffer[packetSize]; //buffer to hold incoming packet,
      
      Serial.print("Received packet of size ");
      Serial.print(packetSize);
      Serial.print(" From: ");
      IPAddress remote = Udp.remoteIP();
      for (int i =0; i < 4; i++) {
        Serial.print(remote[i], DEC);
        if (i < 3)
        {
          Serial.print(".");
        }
      }
      Serial.print(":");
      Serial.print(Udp.remotePort());
      Serial.print(" - ");
  
      // read the packet into packetBufffer
      Udp.read(packetBuffer,packetSize);
  
      // move it to LnTXBuffer
      for (int i=0; i < packetSize; i++) {
        // Add it to the buffer
        addByteLnBuf( &LnTxBuffer, packetBuffer[i]  & 0xFF);
      }

    }
    // Check to see if we have received a complete packet yet
    LnPacket = recvLnMsg( &LnTxBuffer );
    
    if( LnPacket ) {

    // First print out the packet in HEX
      Serial.print("Loconet: ");
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
}

/*************************************************************************/
/*          Program Loop                                                 */
/*************************************************************************/ 
void loop() {

  // Check for any received LocoNet packets
  LoconetRX();
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
