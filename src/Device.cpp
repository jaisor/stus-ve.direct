#include <Arduino.h>
#include <functional>
#include <ArduinoLog.h>

#include "Device.h"

#include <Wire.h>

CDevice::CDevice() {

  tMillisUp = millis();
  tMillisTemp = millis();
  sensorReady = false;

  tLastReading = 0;
#ifdef TEMP_SENSOR_DS18B20
  pinMode(TEMP_SENSOR_PIN, INPUT);
  oneWire = new OneWire(TEMP_SENSOR_PIN);
  DeviceAddress da;
  _ds18b20 = new DS18B20(oneWire);
  _ds18b20->setConfig(DS18B20_CRC);
  _ds18b20->begin();

  _ds18b20->getAddress(da);
  Log.notice(F("DS18B20 sensor at address: "));
  for (uint8_t i = 0; i < 8; i++) {
    if (da[i] < 16) Log.notice("o");
    Log.notice("%x", da[i]);
  }
  Log.noticeln(F(""));
  
  _ds18b20->setResolution(12);
  _ds18b20->requestTemperatures();

  sensorReady = true;
  tMillisTemp = 0;
#endif
#ifdef TEMP_SENSOR_BME280
  _bme = new Adafruit_BME280();
  if (!_bme->begin(BME_I2C_ID)) {
    Log.errorln(F("BME280 sensor initialization failed with ID %x"), BME_I2C_ID);
    sensorReady = false;
  } else {
    sensorReady = true;
    tMillisTemp = 0;
  }
#endif
#ifdef TEMP_SENSOR_DHT
  _dht = new DHT_Unified(TEMP_SENSOR_PIN, TEMP_SENSOR_DHT_TYPE);
  _dht->begin();
  sensor_t sensor;
  _dht->temperature().getSensor(&sensor);
  Log.noticeln(F("DHT temperature sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)"),
    sensor.name, sensor.version, sensor.sensor_id, 
    sensor.min_value, sensor.max_value, sensor.resolution);
  _dht->humidity().getSensor(&sensor);
  Log.noticeln(F("DHT humidity sensor name(%s) v(%u) id(%u) range(%F - %F) res(%F)"),
    sensor.name, sensor.version, sensor.sensor_id, 
    sensor.min_value, sensor.max_value, sensor.resolution);
  minDelayMs = sensor.min_delay / 1000;
  Log.noticeln(F("DHT sensor min delay %i"), minDelayMs);
#endif
#ifdef BATTERY_SENSOR
  #if SEEED_XIAO_M0
    analogReadResolution(12);
  #endif
  pinMode(BATTERY_SENSOR_ADC_PIN, INPUT);
#endif

  Log.infoln(F("Device initialized"));
}

CDevice::~CDevice() { 
#ifdef TEMP_SENSOR_DS18B20
  delete _ds18b20;
#endif
#ifdef TEMP_SENSOR_BME280
  delete _bme;
#endif
#ifdef TEMP_SENSOR_DHT
  delete _dht;
#endif
  Log.noticeln(F("Device destroyed"));
}

void CDevice::loop() {

  uint32_t delay = 1000;
  #ifdef TEMP_SENSOR_DHT
    delay = minDelayMs;
  #endif

  if (!sensorReady && millis() - tMillisTemp > delay) {
    sensorReady = true;
  }

  if (sensorReady && millis() - tMillisTemp > delay) {
    if (millis() - tLastReading < STALE_READING_AGE_MS) {
      tMillisTemp = millis();
    }
    #ifdef TEMP_SENSOR_DS18B20
      if (_ds18b20->isConversionComplete()) {
        _temperature = _ds18b20->getTempC();
        _ds18b20->setResolution(12);
        _ds18b20->requestTemperatures();
        tLastReading = millis();
        Log.verboseln(F("DS18B20 temp: %FC %FF"), _temperature, _temperature*1.8+32);
      } else {
        //Log.verboseln(F("DS18B20 conversion not complete"));
      }
    #endif
    #ifdef TEMP_SENSOR_BME280
      _temperature = _bme->readTemperature();
      _humidity = _bme->readHumidity();
      _baro_pressure = _bme->readPressure();
      tLastReading = millis();
    #endif
    #ifdef TEMP_SENSOR_DHT
      if (millis() - tLastReading > minDelayMs) {
        sensors_event_t event;
        bool goodRead = true;
        // temperature
        _dht->temperature().getEvent(&event);
        if (isnan(event.temperature)) {
          Log.warningln(F("Error reading DHT temperature!"));
          goodRead = false;
        } else {
          _temperature = event.temperature;
          Log.noticeln(F("DHT temp: %FC %FF"), _temperature, _temperature*1.8+32);
        }
        // humidity
        _dht->humidity().getEvent(&event);
        if (isnan(event.relative_humidity)) {
          Log.warningln(F("Error reading DHT humidity!"));
          goodRead = false;
        }
        else {
          _humidity = event.relative_humidity;
          Log.noticeln(F("DHT humidity: %F%%", _humidity);
        }
        
        tLastReading = millis();
      }
    #endif
  }

}

#if defined(TEMP_SENSOR_DS18B20) || defined(TEMP_SENSOR_DHT) || defined(TEMP_SENSOR_BME280)
float CDevice::getTemperature(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return _temperature;
}
#endif

#if defined(TEMP_SENSOR_DHT) || defined(TEMP_SENSOR_BME280)
float CDevice::getHumidity(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return _humidity;
}
#endif

#if defined(TEMP_SENSOR_BME280)
float CDevice::getBaroPressure(bool *current) {
  if (current != NULL) { 
    *current = millis() - tLastReading < STALE_READING_AGE_MS; 
  }
  return _baro_pressure;
}
#endif

float CDevice::getBatteryVoltage(bool *current) {  
  if (current != NULL) { *current = true; } 
  int vi = analogRead(BATTERY_SENSOR_ADC_PIN);
  float v = (float)vi/BATTERY_VOLTS_DIVIDER;
  Log.verboseln(F("Battery voltage raw: %i volts: %D"), vi, v);
  return v; 
}


