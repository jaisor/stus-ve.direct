#include <Arduino.h>
#include <vector>

#include <ArduinoLog.h>
#include <ArduinoLowPower.h>
#include <SPI.h>

#include "Configuration.h"
#include "Device.h"
#include "VEDMessageProvider.h"
#include "RF24Manager.h"
#include "VEDirectManager.h"

CRF24Manager *rf24Manager;
CVEDirectManager *vedManager;
CDevice *device;
unsigned long tsMillisBooted;

#define LED_SETUP INTERNAL_LED_PIN

void setup() {
  randomSeed(analogRead(0));
  
  pinMode(INTERNAL_LED_PIN, OUTPUT);
  pinMode(LED_SETUP, OUTPUT);
  
  digitalWrite(LED_SETUP, LOW);

  #ifndef DISABLE_LOGGING
  Serial.begin(19200); while (!Serial); delay(100);
  Log.begin(LOG_LEVEL, &Serial);
  Log.infoln(F("Initializing..."));
  #endif

  device = new CDevice();
  vedManager = new CVEDirectManager(device);
  rf24Manager = new CRF24Manager(vedManager);
  tsMillisBooted = millis();

  if (rf24Manager->isError() || vedManager->isError()) {
    Log.errorln(F("rf24Manager->isError()=%i; vedManager->isError()=%i"), rf24Manager->isError(), vedManager->isError());
    while(true) {
      intLEDBlink(250);
      delay(250);
    }
  }

  delay(300);
  Log.infoln(F("Initialized"));
  digitalWrite(LED_SETUP, HIGH);
  delay(100);
  digitalWrite(LED_SETUP, LOW);
  delay(100);
  digitalWrite(LED_SETUP, HIGH);
}

void loop() {
  
  intLEDOn();
  device->loop();
  vedManager->loop();
  rf24Manager->loop();

  // Conditions for deep sleep:
  // - Min time elapsed since smooth boot
  // - Any working managers report job done
  if (DEEP_SLEEP_INTERVAL_SEC > 0 
    && millis() - tsMillisBooted > DEEP_SLEEP_MIN_AWAKE_MS
    && rf24Manager->isJobDone()) {

    Log.noticeln(F("Initiating deep sleep for %u sec"), DEEP_SLEEP_INTERVAL_SEC);
    intLEDOff();
    #if defined(ESP32)
      ESP.deepSleep((uint64_t)DEEP_SLEEP_INTERVAL_SEC * 1e6);
    #elif defined(ESP8266)
      ESP.deepSleep((uint64_t)DEEP_SLEEP_INTERVAL_SEC * 1e6); 
    #elif defined(SEEED_XIAO_M0)
      rf24Manager->powerDown();
      LowPower.deepSleep(DEEP_SLEEP_INTERVAL_SEC * 1000);
      delay(100);
      // This deep sleep resumes where it left off on waking
      rf24Manager->powerUp();
      tsMillisBooted = millis();
    #else
      Log.warningln(F("Scratch that, deep sleep is not supported on this platform, delaying instead"));
      delayMicroseconds((uint64_t)DEEP_SLEEP_INTERVAL_SEC * 1e6);
    #endif
  } else if (DEEP_SLEEP_INTERVAL_SEC == 0 
    && millis() - tsMillisBooted > DEEP_SLEEP_MIN_AWAKE_MS
    && rf24Manager->isJobDone()) {
      
    intLEDOff();
    rf24Manager->powerDown();
    vedManager->powerDown();
    Log.infoln(F("Deep sleep disabled, chilling for 5 sec"));
    delay(5000);
    rf24Manager->powerUp();
    vedManager->powerUp();
    tsMillisBooted = millis();
  }

  if (rf24Manager->isRebootNeeded() 
    || (DEEP_SLEEP_INTERVAL_SEC > 0 && (millis() - tsMillisBooted) > DEEP_SLEEP_INTERVAL_SEC * 1000)) {

    Log.noticeln(F("Device is not sleeping right, resetting to save battery"));
    #ifdef ESP32
      ESP.restart();
    #elif ESP8266
      ESP.reset();
    #elif SEEED_XIAO_M0
      NVIC_SystemReset();
    #endif
  }

  yield();
}


