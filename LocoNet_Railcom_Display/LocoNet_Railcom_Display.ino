#include <LocoNet.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// LocoNet Packet Monitor
// Demonstrates the use of the:
//
//   LocoNet.processSwitchSensorMessage(LnPacket)  
//
// function and examples of each of the notifyXXXXXXX user call-back functions

LiquidCrystal_I2C lcd(0x20, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

lnMsg        *LnPacket;

void setup() {
  // First initialize the LocoNet interface
  LocoNet.init();

  // Configure the serial port for 57600 baud
  Serial.begin(57600);
  Serial.println("LocoNet Monitor");

  lcd.begin(16,2);               // initialize the lcd
  lcdPrint("Loconet", "Railcom Monitor");
}

void loop() {  
  // Check for any received LocoNet packets
  LnPacket = LocoNet.receive() ;
  if( LnPacket ) {

    unsigned char opcode = (int)LnPacket->sz.command;

    if (opcode == OPC_GPON)  {        // GLOBAL power ON request 0x83     
      lcdPrint("GLOBAL power ON", "");
    } else if (opcode == OPC_GPOFF) { // GLOBAL power OFF req 0x82
      lcdPrint("GLOBAL power OFF", "");
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
    if(!LocoNet.processSwitchSensorMessage(LnPacket)) {
      Serial.println();
    }
  }
}

void lcdPrint(String line1, String line2) {
  lcd.clear();
  lcd.home ();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Sensor messages
void notifySensor( uint16_t Address, uint8_t State ) {
  Serial.print("Sensor: ");
  Serial.print(Address, DEC);
  Serial.print(" - ");
  Serial.println( State ? "Active" : "Inactive" );
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Request messages
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.print("Switch Request: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Report messages
void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.print("Switch Report: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch State messages
void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.print("Switch State: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for OPC_MULTI_SENSE 0xD0
void notifyMultiSenseTransponder( uint16_t Address, uint8_t Zone, uint16_t LocoAddress, uint8_t Direction ) {
  Serial.print("Railcom Sensor ");
  Serial.print(Address);
  Serial.print(" reports ");
  Serial.print(Direction? "present" : "absent");
  Serial.print(" of decoder address ");
  Serial.print(LocoAddress, DEC);
  Serial.print(" in zone ");
  Serial.println(Zone, HEX);

  String line1 = "Decoder ";
  line1 = line1 + LocoAddress;
  String line2 = Direction ? "present" : "absent";
  line2 = line2 + " zone " + Address;

  lcdPrint(line1, line2);
}
