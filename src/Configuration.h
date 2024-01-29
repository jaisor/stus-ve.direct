#pragma once

#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

//#define DISABLE_LOGGING
#ifndef DISABLE_LOGGING
  #define LOG_LEVEL LOG_LEVEL_INFO
#endif

#if defined(ESP32)
  #define DEVICE_NAME "STUSESP32"
#elif defined(ESP8266)
  #define DEVICE_NAME "STUSESP8266"
#elif defined(SEEED_XIAO_M0)
  #define DEVICE_NAME "STUSXIAO"
#else
  #define DEVICE_NAME "STUS0CLUE"
#endif

#define RADIO_RF24
#ifdef RADIO_RF24
  #define RF24_CHANNEL 76
  #define RF24_DATA_RATE RF24_250KBPS
  #define RF24_PA_LEVEL RF24_PA_HIGH
  #define RF24_ADDRESS "3STUS" // MPPT charger
  // #define RF24_ADDRESS "4STUS" // Phoenix inverter
#endif

//#define BATTERY_SENSOR  // ADC A0 using 0-3.3v voltage divider
#ifdef BATTERY_SENSOR

  #if ESP32
    #define BATTERY_SENSOR_ADC_PIN  A0
  #elif ESP8266
    #define BATTERY_SENSOR_ADC_PIN  A0
  #elif SEEED_XIAO_M0
    #define BATTERY_SENSOR_ADC_PIN  D1
  #else
    #define BATTERY_SENSOR_ADC_PIN  0
  #endif

#endif

#define TEMP_SENSOR_DS18B20
//#define TEMP_SENSOR_BME280
//#define TEMP_SENSOR_DHT
#ifdef TEMP_SENSOR_DHT
  #define TEMP_SENSOR_DHT_TYPE   DHT22
#endif
#ifdef TEMP_SENSOR_BME280
  #define BME_SEALEVELPRESSURE_HPA (1013.25)
  #define BME_I2C_ID 0x76
#endif
#ifdef ESP32
  #define DEEP_SLEEP_DISABLE_PIN GPIO_NUM_1
  #define TEMP_SENSOR_PIN 0
#elif ESP8266
  #define DEEP_SLEEP_DISABLE_PIN GPIO_NUM_1
  #define TEMP_SENSOR_PIN D3
#elif SEEED_XIAO_M0
  #define TEMP_SENSOR_PIN D4
#endif

#define DEEP_SLEEP_INTERVAL_SEC 0 // 300 // 5 min default, 0 - disabled
#define DEEP_SLEEP_MIN_AWAKE_MS 500 // Minimum time to remain awake after smooth boot before sleeping again
#define BATTERY_VOLTS_DIVIDER 217.55

#define INTERNAL_LED_PIN LED_BUILTIN

uint32_t CONFIG_getDeviceId();
unsigned long CONFIG_getUpTime();

void intLEDOn();
void intLEDOff();
void intLEDBlink(uint16_t ms);