#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>

#include "RF24Manager.h"
#include "Configuration.h"


#if defined(ESP32)
  #define CE_PIN  GPIO_NUM_22
  #define CSN_PIN GPIO_NUM_21
#elif defined(ESP8266)
  #define CE_PIN  D4
  #define CSN_PIN D8
#elif defined(SEEED_XIAO_M0)
  #define CE_PIN  D2
  #define CSN_PIN D3
#endif

#ifdef RADIO_RF24
  #define MAX_RETRIES_BEFORE_DONE 10
#else
  #define MAX_RETRIES_BEFORE_DONE 1
#endif

CRF24Manager::CRF24Manager(IVEDMessageProvider *vedProvider)
:vedProvider(vedProvider), jobDone(false), transmittedCount(0), tsLastTransmit(0) {  
  radio = new RF24(CE_PIN, CSN_PIN);
  
  if (!radio->begin()) {
    Log.errorln(F("Failed to initialize RF24 radio"));
    error = true;
    return;
  }

  #ifdef RADIO_RF24
  uint8_t addr[6];
  memcpy(addr, RF24_ADDRESS, 6);

  const uint8_t maxMessageSize = 32;
  Log.infoln("Max message size: %u", maxMessageSize);
  
  radio->setAddressWidth(5);
  radio->setDataRate(RF24_DATA_RATE);
  radio->setPALevel(RF24_PA_LEVEL);
  radio->setChannel(RF24_CHANNEL);
  radio->setPayloadSize(maxMessageSize);
  radio->setRetries(15, 15);
  radio->setAutoAck(false);
  radio->openWritingPipe(addr);
  radio->stopListening();
  
  Log.infoln("Radio initialized");
  if (Log.getLevel() >= LOG_LEVEL_NOTICE) {
    Log.noticeln(F(" RF Channel: %i"), radio->getChannel());
    Log.noticeln(F(" RF DataRate: %i"), radio->getDataRate());
    Log.noticeln(F(" RF PALevel: %i"), radio->getPALevel());
    Log.noticeln(F(" RF PayloadSize: %i"), radio->getPayloadSize());

    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      char buffer[870] = {'\0'};
      radio->sprintfPrettyDetails(buffer);
      Log.verboseln(buffer);
    }
  }
  #else
    jobDone = true;
  #endif

  error = false;
  tMillis = millis();
  retries = 0;
}

CRF24Manager::~CRF24Manager() { 
  powerDown();
  delay(5);
  delete radio;
  Log.noticeln(F("CRF24Manager destroyed"));
}

void CRF24Manager::loop() {
  if (isJobDone()) {
    // Nothing to do
    return;
  }

  if (millis() - tsLastTransmit < 100) {
    // Allow slower 8266s to catch up
    return;
  }

  // Take measurement
  CBaseMessage *msg = vedProvider->pollMessage();
  if (msg != NULL) { 
    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      Log.verboseln(F("Msg: %s"), msg->getString().c_str());
    }
    if (radio->write(msg->getMessageBuffer(), msg->getMessageLength(), true)) {
      tMillis = millis();
      tsLastTransmit = millis();
      Log.noticeln(F("Transmitted message %i/%i: %s"), transmittedCount, MSGS_TO_TRANSMIT_BEFORE_DONE, msg->getString().c_str());
      if (++transmittedCount > MSGS_TO_TRANSMIT_BEFORE_DONE) {
        jobDone = true;
      }
    } else {
      if (++retries > MAX_RETRIES_BEFORE_DONE) {
        // Lost cause
        Log.warningln(F("Failed to transmit after %i retries"), retries);
        jobDone = true;
      } else {
        // Retry
        uint16_t backoffDelaySec = 100 * retries * retries;  // Exp back off with each attempt
        Log.noticeln(F("RF24 transmit error, will try again for attempt %i after %i seconds"), retries, backoffDelaySec);
        delay(backoffDelaySec);
        intLEDBlink(50);
      }
    }
    delete msg;
  } else if (millis() - tMillis > 5000) {
    tMillis = millis();
    Log.warningln("No VE.Direct message for over 5sec");
  }
}

void CRF24Manager::powerDown() {
  jobDone = true;
  #ifdef RADIO_RF24
    radio->powerDown();
  #endif
}

void CRF24Manager::powerUp() {
  jobDone = false;
  tMillis = millis();
  retries = 0;
  transmittedCount = 0;
  #ifdef RADIO_RF24
    radio->powerUp();
  #endif
}