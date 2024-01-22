#include <ArduinoLog.h>
#include <Arduino.h>
#include <stdexcept>

#include "RF24Message.h"

// Message Uptime-Voltage-Temperature-Humidity-BarometricPressure
#define MSG_UVTHP_ID 1

CRF24Message::CRF24Message(const u_int8_t pipe, const uint16_t uptime, const float voltage, const float temperature, const float humidity, const float baro_pressure)
: CBaseMessage(pipe) {  
  
  error = false;
  msg.id = MSG_UVTHP_ID;
  setUptime(uptime);
  setVoltage(voltage);
  setTemperature(temperature);
  setHumidity(humidity);
  setBaroPressure(baro_pressure);
}

CRF24Message::CRF24Message(const u_int8_t pipe, const void* buf, const uint8_t length)
: CBaseMessage(pipe) { 
  if (length != getMessageLength()) {
    error = true;
    return;
  }
  memcpy(&msg, buf, length <= getMessageLength() ? length : getMessageLength());
  if (msg.id != MSG_UVTHP_ID) {
    //Log.errorln(F("Message ID %i doesn't match MSG_UVTHP_ID(%i)"), msg.id, MSG_UVTHP_ID);
    memset(&msg, 0, getMessageLength());
    error = true;
  } else {
    error = false;
  }
}

const void* CRF24Message::getMessageBuffer() {
  return &msg;
}

uint32_t CRF24Message::getUptime() {
  return msg.uptime;
}

void CRF24Message::setUptime(uint32_t value) {
  msg.uptime = value;
}

float CRF24Message::getVoltage() {
  return msg.voltage;
}

void CRF24Message::setVoltage(float value) {
  msg.voltage = value;
}

float CRF24Message::getTemperature() {
  return msg.temperature;
}

void CRF24Message::setTemperature(float value) {
  msg.temperature = value;
}

float CRF24Message::getHumidity() {
  return msg.humidity;
}

void CRF24Message::setHumidity(float value) {
  msg.humidity = value;
}

float CRF24Message::getBaroPressure() {
  return msg.baro_pressure;
}

void CRF24Message::setBaroPressure(float value) {
  msg.baro_pressure = value;
}

const String CRF24Message::getString() {
  char c[255];
  snprintf_P(c, 255, PSTR("[%u] (V=%0.2fV, T=%0.2fC, H=%0.2f%%, BP=%0.2fKPa U=%0.2fsec)"), pipe, 
        msg.voltage, msg.temperature, msg.humidity, msg.baro_pressure/1000.0, (float)(msg.uptime)/1000.0);
  Log.verboseln(F("CRF24Message::getString() : %s"), c);
  return String(c);
}