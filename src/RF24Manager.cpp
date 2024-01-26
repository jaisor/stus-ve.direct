#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <ArduinoLog.h>

#include "RF24Manager.h"
#include "Configuration.h"
#include "RF24Message.h"
#include "RF24Message_VED_MPPT.h"
#include "RF24Message_VED_INV.h"


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

CRF24Manager::CRF24Manager(ISensorProvider* sensor, IVEDMessageProvider *vedProvider)
:sensor(sensor), vedProvider(vedProvider) {  
  jobDone = false;
  radio = new RF24(CE_PIN, CSN_PIN);
  
  if (!radio->begin()) {
    Log.errorln(F("Failed to initialize RF24 radio"));
    return;
  }

  #ifdef RADIO_RF24

  uint8_t addr[6];
  memcpy(addr, RF24_ADDRESS, 6);

  uint8_t maxMessageSize = sizeof(r24_message_uvthp_t);
  if (sizeof(r24_message_ved_inv_t) > maxMessageSize) { maxMessageSize = sizeof(r24_message_ved_inv_t); }
  if (sizeof(r24_message_ved_mppt_t) > maxMessageSize) { maxMessageSize = sizeof(r24_message_ved_mppt_t); }
  Log.noticeln("sizeof(r24_message_uvthp_t): %u", sizeof(r24_message_uvthp_t));
  Log.noticeln("sizeof(r24_message_ved_inv_t): %u", sizeof(r24_message_ved_inv_t));
  Log.noticeln("sizeof(r24_message_ved_mppt_t): %u", sizeof(r24_message_ved_mppt_t));
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

  // Take measurement
  CBaseMessage *msg = vedProvider->checkForMessage();
  if (msg != NULL) { 
    if (Log.getLevel() >= LOG_LEVEL_VERBOSE) {
      Log.verboseln(F("Msg: %s"), msg->getString().c_str());
    }
    if (radio->write(msg->getMessageBuffer(), msg->getMessageLength(), true)) {
      Log.noticeln(F("Transmitted message length %i"), msg->getMessageLength());
      jobDone = true;
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
  radio->powerUp();
}