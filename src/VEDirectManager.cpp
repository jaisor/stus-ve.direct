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

// TODO: Eventually use https://github.com/giacinti/VeDirectFrameHandler/blob/master/example/ReadVEDirectFramehandler.ino
// Protocol https://www.victronenergy.com/upload/documents/VE.Direct-Protocol-3.33.pdf

CVEDirectManager::CVEDirectManager(ISensorProvider* sensor)
:sensor(sensor) {  
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
  
  //if (millis() - tMillis > 1000) {
    {
    tMillis = millis();  

    //Log.infoln("Reading VEDirect data");
    // Take measurement
    char rc;
    while (VEDirectStream->available() > 0) {
      rc = VEDirectStream->read();
      //VEDirectHandler.rxData(rc);
      Serial.write(rc);
    }

    //for (int i = 0; i < VEDirectHandler.veEnd; i++ ) {
    //  Log.infoln("VED '%s'=%s", VEDirectHandler.veName[i], VEDirectHandler.veValue[i]);
    //}
  }
}

void CVEDirectManager::powerDown() {
  jobDone = true;
}

void CVEDirectManager::powerUp() {
  jobDone = false;
  tMillis = 0;
}

void CVEDirectManager::LogHelper(const char* module, const char* error) {
  Log.errorln("VED error in %s : %s", module, error);
}