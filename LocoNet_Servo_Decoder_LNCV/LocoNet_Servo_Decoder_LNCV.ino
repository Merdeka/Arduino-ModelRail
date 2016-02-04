#include <EEPROM.h>
#include <Wire.h>
#include <LocoNet.h>
#include <Adafruit_PWMServoDriver.h>

#define DEBUG

#define       NrServos  16  // Number of Servos Connected
const uint8_t baseAddr = 2; // EEProm byte 0 and 1 are used for the decoder address so start storing servo settings from byte 2

#define ARTNR 5001          // Item Number (Art.-Nr.): 50010
uint16_t decoderAddress;
uint16_t lncv[(NrServos*3)+1];
boolean programmingMode;

typedef struct servoValues {
  uint16_t address;
  uint16_t servoMin;
  uint16_t servoMax; 
};
 
servoValues settings[NrServos];

Adafruit_PWMServoDriver servo = Adafruit_PWMServoDriver(0x40);

// Loconet
#define LOCONET_TX_PIN 7
static lnMsg *LnPacket;
LocoNetCVClass lnCV;

/******************************************************************************
 * Setup
 ******************************************************************************/
void setup() {

  // Read Settings from EEProm
  readEEProm();

  // First initialize the LocoNet interface
  LocoNet.init(LOCONET_TX_PIN);
  
  Serial.begin(57600);

  Serial.print("\nLoconet Servo Decoder #");
  Serial.println(decoderAddress, DEC);

  for(int i=0; i<NrServos; i++){
    Serial.print("Servo ");
    Serial.print(i+1);
    Serial.print(" Address: ");
    Serial.print(settings[i].address);
    Serial.print(" Min: ");
    Serial.print(settings[i].servoMin);
    Serial.print(" Max: ");
    Serial.println(settings[i].servoMax);
  }
   
  servo.begin();
  
  servo.setPWMFreq(60);  // Analog servos run at ~60 Hz updates  
}

/******************************************************************************
 * Loop
 ******************************************************************************/
void loop() {

  LnPacket = LocoNet.receive() ;

  if (LnPacket) {
    uint8_t packetConsumed(LocoNet.processSwitchSensorMessage(LnPacket));
    if (packetConsumed == 0) {
      packetConsumed = lnCV.processLNCVMessage(LnPacket);
    }
  }
  
  // If this packet was not a Switch or Sensor Message then print a new line 
  //if(!LocoNet.processSwitchSensorMessage(LnPacket)) {
    
  //}
  
}

/******************************************************************************
 * Arduino Reset Function
 ******************************************************************************/
void(* resetFunc) (void) = 0;  // declare reset fuction at address 0

/******************************************************************************
 * Read Settings from EEProm
 ******************************************************************************/
void readEEProm() {

  // read the decoder address from EEProm byte 0 and 1
  uint16_t value = 0x00;
  value = value | (EEPROM.read(0) << 8);
  value = value |  EEPROM.read(1);
  decoderAddress = value;
  
  // read servoValue array to eeprom
  // servoValue array takes 3x2 bytes
  // addresses are baseAddr + address every 6 bytes
  for (uint8_t i=0; i < NrServos; i++){
    eeprom_read_block((void*)&settings[i], (void*) (i*6)+baseAddr, sizeof(settings[i])); // void * data, void * dest = i*6 + baseAddr, size_t size
  }
}

/******************************************************************************
 * Write Settings to EEProm
 ******************************************************************************/
void writeEEProm() {

  // write the decoder address to EEProm byte 0 and 1
  EEPROM.write(0, (decoderAddress >> 8) & 0xFF );
  EEPROM.write(1,  decoderAddress & 0xFF);
  
  // write servoValue array to eeprom
  // servoValue array takes 3x2 bytes
  // addresses are baseAddr + address every 6 bytes
  for (uint8_t i=0; i < NrServos; i++){
    eeprom_write_block((const void*)&settings[i], (void*) (i*6)+baseAddr, sizeof(settings[i])); // const void * data, void * dest = i*6 + baseAddr, size_t size
  }
}

/******************************************************************************
 * Pulse Length in seconds routine
 * setServoPulse(0, 0.001) is a ~1 millisecond pulse width. its not precise!
 ******************************************************************************/
void setServoPulse(uint8_t n, double pulse) {
  double pulselength;
  
  pulselength = 1000000;   // 1,000,000 us per second
  pulselength /= 60;   // 60 Hz
  //Serial.print(pulselength); Serial.println(" us per period"); 
  pulselength /= 4096;  // 12 bits of resolution
  //Serial.print(pulselength); Serial.println(" us per bit"); 
  pulse *= 1000;
  pulse /= pulselength;
  //Serial.println(pulse);
  servo.setPWM(n, 0, pulse);
}

/******************************************************************************
 * Routine to Set Servo
 ******************************************************************************/
void setServo(uint8_t servoNum, uint8_t Direction) {
  
  if( Direction == 1 ) {
    for (uint16_t pulselen = settings[servoNum].servoMin; pulselen < settings[servoNum].servoMax; pulselen++) {
      servo.setPWM(servoNum, 0, pulselen);
    }
  } else {
    for (uint16_t pulselen = settings[servoNum].servoMax; pulselen > settings[servoNum].servoMin; pulselen--) {
      servo.setPWM(servoNum, 0, pulselen);
    }
  }
}

/******************************************************************************
 * This call-back function is called from LocoNet.processSwitchSensorMessage
 * for all Switch Request messages
 ******************************************************************************/
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  Serial.print("Switch Request: ");
  Serial.print(Address, DEC);
  Serial.print(':');
  Serial.print(Direction ? "Closed" : "Thrown");
  Serial.print(" - ");
  Serial.println(Output ? "On" : "Off");

  // check if the Address is meant for us
  for (uint8_t i=0; i<NrServos; i++) {
    if (Address == settings[i].address) {
      setServo(i, Direction ? 0 : 1);
    }
  }
}


  /**
   * Notifies the code on the reception of a read request.
   * Note that without application knowledge (i.e., art.-nr., module address
   * and "Programming Mode" state), it is not possible to distinguish
   * a read request from a programming start request message.
   */
int8_t notifyLNCVread(uint16_t ArtNr, uint16_t lncvAddress, uint16_t, uint16_t & lncvValue) {
  Serial.print("Enter notifyLNCVread(");
  Serial.print(ArtNr, HEX);
  Serial.print(", ");
  Serial.print(lncvAddress, HEX);
  Serial.print(", ");
  Serial.print(", ");
  Serial.print(lncvValue, HEX);
  Serial.print(")");
  // Step 1: Can this be addressed to me?
  // All ReadRequests contain the ARTNR. For starting programming, we do not accept the broadcast address.
  if (programmingMode) {
    if (ArtNr == ARTNR) {
      if (lncvAddress < (NrServos*3)+1) {
        lncvValue = lncv[lncvAddress];
        Serial.print(" LNCV Value: ");
        Serial.print(lncvValue);
        Serial.print("\n");
        return LNCV_LACK_OK;
      } else {
        // Invalid LNCV address, request a NAXK
        return LNCV_LACK_ERROR_UNSUPPORTED;
      }
    } else {
      Serial.print("ArtNr invalid.\n");
      return -1;
    }
  } else {
    Serial.print("Ignoring Request.\n");
    return -1;
  }
}

int8_t notifyLNCVprogrammingStart(uint16_t & ArtNr, uint16_t & ModuleAddress) {
  // Enter programming mode. If we already are in programming mode,
  // we simply send a response and nothing else happens.
  Serial.print("notifyLNCVProgrammingStart ");
  if (ArtNr == ARTNR) {
    Serial.print("artnrOK ");
    if (ModuleAddress == decoderAddress) {
      Serial.print("moduleUNI ENTERING PROGRAMMING MODE\n");
      programmingMode = true;
      SettingstoLNCV();
      return LNCV_LACK_OK;
    } else if (ModuleAddress == 0xFFFF) {
      Serial.print("moduleBC ENTERING PROGRAMMING MODE\n");
      ModuleAddress = decoderAddress;
      SettingstoLNCV();
      return LNCV_LACK_OK;
    }
  }
  Serial.print("Ignoring Request.\n");
  return -1;
}

  /**
   * Notifies the code on the reception of a write request
   */
int8_t notifyLNCVwrite(uint16_t ArtNr, uint16_t lncvAddress, uint16_t lncvValue) {
  Serial.print("notifyLNCVwrite, ");
  if (!programmingMode) {
    Serial.print("not in Programming Mode.\n");
    return -1;
  }

  if (ArtNr == ARTNR) {
    Serial.print("Artnr OK, ");

    if(lncvAddress == 0) { // Address Change
      lncv[0] = lncvValue;
      Serial.print("New Module Address: ");
      Serial.println(lncvValue);
      return LNCV_LACK_OK;
    }
    else if ( lncvAddress > 0 && lncvAddress < (NrServos*3)+1 ) {
      lncv[lncvAddress] = lncvValue;
      Serial.print("LNCV ");
      Serial.print(lncvAddress, DEC);
      Serial.print(" -> ");
      Serial.println(lncvValue);
      return LNCV_LACK_OK;
    }
    else {
      Serial.println("LACK ERROR");
      return LNCV_LACK_ERROR_UNSUPPORTED;
    }

  }
  else {
    Serial.print("Artnr Invalid.\n");
    return -1;
  }
}

  /**
   * Notifies the code on the reception of a request to end programming mode
   */
void notifyLNCVprogrammingStop(uint16_t ArtNr, uint16_t ModuleAddress) {
  Serial.print("notifyLNCVprogrammingStop ");
  if (programmingMode) {
    if (ArtNr == ARTNR && ModuleAddress == lncv[0]) {
      programmingMode = false;
      Serial.print("End Programing Mode.\n");
      Serial.print("Module Address is now: ");
      Serial.println(lncv[0], DEC);

      int Adr = 1;
      int Min = 2;
      int Max = 3;
      
      LNCVtoSettings();
      
      writeEEProm();
      Serial.println("EEProm Updated");

      // wait 250msec and reboot
      delay(250);
      resetFunc();
    }
    else {
      if (ArtNr != ARTNR) {
        Serial.print("Wrong Artnr.\n");
        return;
      }
      if (ModuleAddress != lncv[0]) {
        Serial.print("Wrong Module Address: ");
        Serial.print(ModuleAddress, DEC);
        Serial.print(" : ");
        Serial.println(lncv[0], DEC);
        return;
      }
    }
  }
  else {
    Serial.print("Ignoring Request.\n");
  }
}

void LNCVtoSettings() {

      decoderAddress = lncv[0];
  
      int Adr = 1;
      int Min = 2;
      int Max = 3;
      
      for(int i=0; i<NrServos ;i++) {
        settings[i].address = lncv[Adr];
        settings[i].servoMin = lncv[Min];
        settings[i].servoMax = lncv[Max];
        
        Adr = Adr+3;
        Min = Min+3;
        Max = Max+3;
      }
}

void SettingstoLNCV() {

      lncv[0] = decoderAddress;
        
      int Adr = 1;
      int Min = 2;
      int Max = 3;
      
      for(int i=0; i<NrServos ;i++) {
        lncv[Adr] = settings[i].address;
        lncv[Min] = settings[i].servoMin;
        lncv[Max] = settings[i].servoMax;
        
        Adr = Adr+3;
        Min = Min+3;
        Max = Max+3;
      }
}


