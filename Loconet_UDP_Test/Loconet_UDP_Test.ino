#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LocoNet.h>

LocoNetClass sensors;

// The MAC address of your Ethernet board (or Ethernet Shield) is located on the back of the circuit board.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0x02 };  // Arduino Ethernet
EthernetClient client;

// Change this to your own network
byte ip[] = { 192,168,60,33 };
 
// Multicast
IPAddress multicast(224,0,0,1);
unsigned int port = 1235;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

int base_sensor_address = 501;

static   LnBuf        LnTxBuffer ;
static   lnMsg        *LnPacket;

byte IN1;
byte IN2=0;
 
void setup() {
  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.beginMulticast(multicast ,port);
 
  Serial.begin(57600);
  Serial.println("Loconet UDP");
  
  // Initialize a LocoNet packet buffer to buffer bytes from the PC 
  initLnBuf(&LnTxBuffer) ;
  
}
 
void loop() {
 
  rxLoconet();
  delay(10);
}

void rxLoconet() {
   // Parse and display incoming packets
  
  int packetSize = Udp.parsePacket();
  if(packetSize) {
    
    char packetBuffer[packetSize];
    Udp.read(packetBuffer, packetSize);
    
    for (int i; i < packetSize; i++) {
      // Add it to the buffer
      addByteLnBuf( &LnTxBuffer, packetBuffer[i] & 0xFF );
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
}

void txLoconet4Byte (byte type, byte firstbyte, byte secondbyte) 
{
    byte checksum = 0xFF - (0xB2^ firstbyte ^ secondbyte);
 
      // Add bytes to the buffer
    addByteLnBuf( &LnTxBuffer, type ) ;
    addByteLnBuf( &LnTxBuffer, firstbyte ) ;
    addByteLnBuf( &LnTxBuffer, secondbyte ) ;
    addByteLnBuf( &LnTxBuffer, checksum ) ;
  
      // Check to see if we have a complete packet...
    LnPacket = recvLnMsg( &LnTxBuffer ) ;
    if( LnPacket )
    {
     
     // Get the length of the packet
     uint8_t LnPacketSize = getLnMsgSize( LnPacket ); 
     uint8_t locoTX[LnPacketSize];
      
      for(int i = 0; i < LnPacketSize; i++) {
        locoTX[i] = LnPacket->data[i];
        Serial.println(locoTX[i], HEX);
      }
      
      Udp.beginPacket(multicast, port);
      Udp.write(locoTX, LnPacketSize);
      Udp.endPacket();

     }
}

void sendSensor(int address, boolean level ) {
  
  address = (address / 2) - 1;
  
  IN1 = lowByte( address & 0x7F );
  IN2 = highByte( address ) << 1;
  
  bitWrite(IN2, 0, (bitRead(lowByte( address), 7)) );
  bitWrite (IN2,4, level);
  bitWrite (IN2,5, bitRead(address,0));
  bitClear (IN2,6); //  input source 0=DS54, 1=switch
  
  txLoconet4Byte (OPC_INPUT_REP, IN1, IN2);
  
  print_binary(IN1, 8);
  Serial.println();
  print_binary(IN2, 8);
  Serial.println();
  Serial.println("---------");
  
}

void sendTestSensor() {
  sendSensor(1024, 1);
}

void sendALLsensors()
{
  byte addr;
   for (int i = 0; i < 1; i++)
     {
        addr = 16; //base_sensor_address + i;
        IN1 = addr;
        IN1 = IN1 >> 1;
        bitClear (IN1,7);
        bitClear (IN2,6); //  input source 0=DS54, 1=switch
        bitWrite (IN2,5, bitRead(addr,0));
        bitWrite (IN2,4, 1);
        txLoconet4Byte (OPC_INPUT_REP, IN1, IN2);  
       }
  
  }

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Sensor messages
void notifySensor( uint16_t Address, uint8_t State )
{
  Serial.print("Sensor: ");
  Serial.print(Address, DEC);
  Serial.print(" - ");
  Serial.println( State ? "Active" : "Inactive" );
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Request messages
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  Serial.print("Switch Request: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch Report messages
void notifySwitchReport( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  Serial.print("Switch Report: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

  // This call-back function is called from LocoNet.processSwitchSensorMessage
  // for all Switch State messages
void notifySwitchState( uint16_t Address, uint8_t Output, uint8_t Direction )
{
  Serial.print("Switch State: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");
}

void print_binary(int v, int num_places)
{
    int mask=0, n;

    for (n=1; n<=num_places; n++)
    {
        mask = (mask << 1) | 0x0001;
    }
    v = v & mask;  // truncate v to specified number of places

    while(num_places)
    {

        if (v & (0x0001 << num_places-1))
        {
             Serial.print("1");
        }
        else
        {
             Serial.print("0");
        }

        --num_places;
        if(((num_places%4) == 0) && (num_places != 0))
        {
            Serial.print("_");
        }
    }
}  
