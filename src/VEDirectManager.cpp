#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>

#if defined(ESP8266)
  #include <SoftwareSerial.h> // ESP8266 uses software UART because of USB conflict with its single full hardware UART
#endif

#if defined(ESP32)
  #define VE_RX GPIO_NUM_16
  #define VE_TX GPIO_NUM_17
#elif defined(ESP8266)
  #define VE_RX D3
  #define VE_TX D4
#elif defined(SEEED_XIAO_M0)
  #define VE_RX D7
  #define VE_TX D6
#else
  #error Unsupported platform
#endif

#include "VEDirectManager.h"
#include "Configuration.h"

static constexpr char checksumTagName[] = "CHECKSUM";

// Protocol https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.33.pdf

CVEDirectManager::CVEDirectManager(ISensorProvider* sensor)
:sensor(sensor), mState(IDLE), mChecksum(0), mTextPointer(0) {  
  jobDone = false;

  #if defined(ESP32)
    Serial2.begin(19200, SERIAL_8N1, VE_RX, VE_TX);
    VEDirectStream = &Serial2;
  #elif defined(ESP8266)
    pinMode(VE_RX, INPUT);
    pinMode(VE_TX, OUTPUT);
    SoftwareSerial *ves = new SoftwareSerial(VE_RX, VE_TX);
    ves->begin(19200, SWSERIAL_8N1);
    VEDirectStream = ves;
  #elif defined(SEEED_XIAO_M0)
    Serial1.begin(19200, SERIAL_8N1); // Defaults to RX=D7; TX=D8;
    VEDirectStream = &Serial1;
  #endif

  

  tMillis = 0;
}

CVEDirectManager::~CVEDirectManager() { 
  powerDown();
  delay(5);
  Log.noticeln(F("CRF24Manager destroyed"));
}

void CVEDirectManager::loop() {
  if (isJobDone()) {
    // Nothing to do
    return;
  }
  
  if (millis() - tMillis > 1000) {
    tMillis = millis();  

    Log.infoln("Reading VEDirect data");
    // Take measurement
    char rc;
    while (VEDirectStream->available() > 0) {
      rc = VEDirectStream->read();
      rxData(rc);
    }

    Log.infoln("Read %u values", mVEData.size());
    std::map<String, String>::iterator it = mVEData.begin();
    while (it != mVEData.end()) {
      Log.verboseln("  [%s]='%s'",it->first.c_str(), it->second.c_str());
      ++it;
    }
  }
}

void CVEDirectManager::powerDown() {
  jobDone = true;
}

void CVEDirectManager::powerUp() {
  jobDone = false;
  tMillis = 0;
}

void CVEDirectManager::rxData(uint8_t inbyte) {
  
  if ( (inbyte == ':') && (mState != CHECKSUM) ) {
    mState = RECORD_HEX;
  }
  if (mState != RECORD_HEX) {
    mChecksum += inbyte;
  }
  inbyte = toupper(inbyte);
 
  switch(mState) {
    case IDLE:
      /* wait for \n of the start of an record */
      switch(inbyte) {
        case '\n':
          mState = RECORD_BEGIN;
          break;
        case '\r': /* Skip */
        default:
          break;
      }
      break;
    case RECORD_BEGIN:
      mTextPointer = mName;
      *mTextPointer++ = inbyte;
      mState = RECORD_NAME;
      break;
    case RECORD_NAME:
      // The record name is being received, terminated by a \t
      switch(inbyte) {
      case '\t':
        // the Checksum record indicates a EOR
        if ( mTextPointer < (mName + sizeof(mName)) ) {
          *mTextPointer = 0; /* Zero terminate */
          if (strcmp(mName, checksumTagName) == 0) {
            mState = CHECKSUM;
            break;
          }
        }
        mTextPointer = mValue; /* Reset value pointer */
        mState = RECORD_VALUE;
        break;
      default:
        // add byte to name, but do no overflow
        if ( mTextPointer < (mName + sizeof(mName)) )
          *mTextPointer++ = inbyte;
        break;
      }
      break;
    case RECORD_VALUE:
      // The record value is being received.  The \r indicates a new record.
      switch(inbyte) {
      case '\n':
        // forward record, only if it could be stored completely
        if ( mTextPointer < (mValue + sizeof(mValue)) ) {
          *mTextPointer = 0; // make zero ended
          mVEData[String(mName)] = String(mValue);
        }
        mState = RECORD_BEGIN;
        break;
      case '\r': /* Skip */
        break;
      default:
        // add byte to value, but do no overflow
        if ( mTextPointer < (mValue + sizeof(mValue)) )
          *mTextPointer++ = inbyte;
        break;
      }
      break;
    case CHECKSUM:
    {
      if (mChecksum != 0) {
        Log.warningln("Invalid checksum %x, ignoring frame", mChecksum);
        mVEData.clear();
      }
      mChecksum = 0;
      mState = IDLE;
      frameEndEvent(mChecksum == 0);
      break;
    }
    case RECORD_HEX:
      if (hexRxEvent(inbyte)) {
        mChecksum = 0;
        mState = IDLE;
      }
      break;
  }
}

bool CVEDirectManager::hexRxEvent(uint8_t inbyte) {
  Log.infoln("Unsupported HEX data %x", inbyte);
  return true;
}

void CVEDirectManager::frameEndEvent(bool valid) {
  
}