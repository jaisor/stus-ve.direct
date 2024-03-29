#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>
#include <set>

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

#include <RF24Message_VED_MPPT.h>
#include <RF24Message_VED_INV.h>
#include <RF24Message_VED_BATT.h>
#include <RF24Message_VED_BATT_SUP.h>
#include <RF24Message.h>
#include "VEDirectManager.h"

static constexpr char checksumTagName[] = "CHECKSUM";

const std::set<uint16_t> PIDS_MPPT = {0XA057, 0XA055}; //
const std::set<uint16_t> PIDS_INV = {0xA2FA}; //
const std::set<uint16_t> PIDS_BATT = {0XA389}; //
const std::set<uint16_t> PIDS_WITH_SUPPLEMENTALS = {0XA389}; //

// Protocol https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.33.pdf

CVEDirectManager::CVEDirectManager(ISensorProvider* sensor)
:tMillis(0), tMillisError(millis()), jobDone(false), mState(IDLE), mChecksum(0), mTextPointer(0), sensor(sensor), lastPid(0), randomDelay(0) {  

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
    
    char rc;
    while (VEDirectStream->available() > 0) {
      rc = VEDirectStream->read();
      rxData(rc);
    }

    Log.verboseln("Read %u VE.Direct values", mVEData.size());
    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      std::map<String, String>::iterator it = mVEData.begin();
      while (it != mVEData.end()) {
        Log.verboseln("  [%s]='%s'",it->first.c_str(), it->second.c_str());
        ++it;
      }
    }
    
    if (messages.size() != 0) {
      // S'all good
      tMillisError = millis();
    } else if (millis() - tMillisError > 10000) {
      // Prepare error message
      Log.warningln(F("Preparing error message about VEDirect communication failure"));
      tMillisError = millis();
      const r24_message_uvthp_t _msg = {
        MSG_UVTHP_ID,
        CONFIG_getUpTime(),
        sensor->getBatteryVoltage(NULL),
        sensor->getTemperature(NULL),
        sensor->getHumidity(NULL),
        sensor->getBaroPressure(NULL),
        VEDirectCommFail
      };
      addMessage(new CRF24Message(0, _msg));
    }
  }
}

void CVEDirectManager::powerDown() {
  jobDone = true;
  while(!messages.empty()) {
    CBaseMessage *msg = messages.front();
    messages.pop();
    delete msg;
  }
  mVEData.clear();
}

void CVEDirectManager::powerUp() {
  jobDone = false;
  tMillis = 0;
  tMillisError = millis();
  mVEData.clear();
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
          if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
            //Log.verboseln("~~ [%s] = '%s'", mName, mValue);
          }
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
    case CHECKSUM: {
      Log.traceln("mVEData size=%i, checksum=%i", mVEData.size(), mChecksum);
      if (mChecksum != 0) {
        Log.traceln("Ignoring frame with invalid checksum %x", mChecksum);
      } else if (mVEData.size() == 0) {
        Log.traceln(F("Ignoring empty frame"));
      } else {
        frameEndEvent();
      }
      mChecksum = 0;
      mState = IDLE;
      mVEData.clear();
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

void CVEDirectManager::frameEndEvent() {

  std::map<String, String>::const_iterator pid = mVEData.find(String("PID"));
  if (pid == mVEData.end() 
      && (lastPid == 0 || PIDS_WITH_SUPPLEMENTALS.find(lastPid) == PIDS_WITH_SUPPLEMENTALS.end())) {

    Log.warningln("Ignoring frame without a PID (lastPid=%x)", lastPid);
    if (Log.getLevel() >= LOG_LEVEL_NOTICE) {
      std::map<String, String>::iterator it = mVEData.begin();
      while (it != mVEData.end()) {
        Log.noticeln(F("  [%s]='%s'"),it->first.c_str(), it->second.c_str());
        ++it;
      }
    }
    return;
  }

  bool tempCurrent = false;
  float temp = sensor->getTemperature(&tempCurrent);
  if (!sensor->isSensorReady() || !tempCurrent) {
    Log.verboseln(F("Skipping frame while temperature sensor not ready or stale"));
    return;
  }

  tMillisError = millis();
  uint16_t pidInt = 0;
  if (pid != mVEData.end()) {
    pidInt = std::stoul(pid->second.c_str(), nullptr, 16);
    lastPid = pidInt;

    Log.traceln(F("Preparing event for PID '%s'(%x) with %i values and sensor temp %DC"), pid->second.c_str(), pidInt, mVEData.size(), temp);
    if (PIDS_MPPT.find(pidInt) != PIDS_MPPT.end()) {
      Log.traceln(F("PID is MPPT charger"));
      const r24_message_ved_mppt_t _msg {
        MSG_VED_MPPT_ID,
        //
        static_cast<float>(atoi(mVEData[String("V")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("I")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("VPV")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("PPV")].c_str())),
        //
        static_cast<uint8_t>(atoi(mVEData[String("CS")].c_str())),
        static_cast<uint8_t>(atoi(mVEData[String("MPPT")].c_str())),
        static_cast<uint8_t>(strtol(mVEData[String("OR")].c_str(), NULL, 16)),
        static_cast<uint8_t>(atoi(mVEData[String("ERR")].c_str())),
        //
        static_cast<uint16_t>(atoi(mVEData[String("H20")].c_str()) * 10),
        static_cast<uint16_t>(atoi(mVEData[String("H21")].c_str())),
        //
        temp
      };
      addMessage(new CRF24Message_VED_MPPT(0, _msg));
    } else if (PIDS_INV.find(pidInt) != PIDS_INV.end()) {
      Log.traceln("PID is AC inverter");
      const r24_message_ved_inv_t _msg {
        MSG_VED_INV_ID,
        //
        static_cast<float>(atoi(mVEData[String("V")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("AC_OUT_I")].c_str()) / 10.0),
        static_cast<float>(atoi(mVEData[String("AC_OUT_V")].c_str()) / 100.0),
        static_cast<float>(atoi(mVEData[String("AC_OUT_S")].c_str())),
        //
        static_cast<uint8_t>(atoi(mVEData[String("CS")].c_str())),
        static_cast<int8_t>(atoi(mVEData[String("MODE")].c_str())),
        static_cast<uint8_t>(strtol(mVEData[String("OR")].c_str(), NULL, 16)),
        static_cast<uint8_t>(atoi(mVEData[String("AR")].c_str())),
        static_cast<uint8_t>(atoi(mVEData[String("WARN")].c_str())),
        //
        temp
      };
      addMessage(new CRF24Message_VED_INV(0, _msg));
    } else if (PIDS_BATT.find(pidInt) != PIDS_BATT.end()) {
      Log.traceln("PID is BATT monitor");
      const r24_message_ved_batt_t _msg {
        MSG_VED_BATT_ID,
        //
        static_cast<float>(atoi(mVEData[String("V")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("VS")].c_str()) / 1000.0),
        static_cast<float>(atoi(mVEData[String("I")].c_str()) / 1000.0),
        static_cast<int16_t>(atoi(mVEData[String("P")].c_str())),
        //
        static_cast<float>(atoi(mVEData[String("CE")].c_str()) / 1000.0),
        static_cast<uint16_t>(atoi(mVEData[String("SOC")].c_str())),
        static_cast<uint16_t>(atoi(mVEData[String("TTG")].c_str())),
        //
        static_cast<uint8_t>(atoi(mVEData[String("AR")].c_str())),
      };
      addMessage(new CRF24Message_VED_BATT(0, _msg));
    } else if (pid != mVEData.end()) {
      Log.warningln("Received frame with unsupported PID: %s", pid->second.c_str());
    }
  } else if (lastPid && PIDS_WITH_SUPPLEMENTALS.find(lastPid) != PIDS_WITH_SUPPLEMENTALS.end()) {
    Log.noticeln(F("Preparing supplemental event for PID %x with %i values and sensor temp %DC"), lastPid, mVEData.size(), temp);
    const r24_message_ved_batt_sup_t _msg {
      MSG_VED_BATT_SUP_ID, // TODO: Support other devices that might have supplemental messages
      //
      static_cast<float>(atoi(mVEData[String("H2")].c_str()) / 1000.0),
      static_cast<uint16_t>(atoi(mVEData[String("H4")].c_str())),
      //
      static_cast<float>(atoi(mVEData[String("H7")].c_str()) / 1000.0),
      static_cast<float>(atoi(mVEData[String("H15")].c_str()) / 1000.0),
      //
      static_cast<float>(atoi(mVEData[String("H18")].c_str()) * 10.0),
      static_cast<float>(atoi(mVEData[String("H17")].c_str()) * 10.0),
      //
      temp
    };
    addMessage(new CRF24Message_VED_BATT_SUP(0, _msg));
  }
}

CBaseMessage* CVEDirectManager::pollMessage() { 
  if (messages.size() == 0) {
    return NULL;
  }
  CBaseMessage* msg = messages.front();
  messages.pop();
  return msg;
}

void CVEDirectManager::addMessage(CBaseMessage *msg) {
  if (messages.size() > MSGS_TO_TRANSMIT_BEFORE_DONE + 1 && messages.front()->getId() == msg->getId()) {
    Log.noticeln("Deleting excess message with ID %x", msg->getId());
    CBaseMessage* msg = messages.front();
    messages.pop();
    delete msg;
  }
  Log.noticeln("Adding message with ID %i to queue of size: %i", msg->getId(), messages.size());
  messages.push(msg);
  
}