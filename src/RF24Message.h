#pragma once

#include "BaseMessage.h"

class CRF24Message: public CBaseMessage {
private:
  r24_message_uvthp_t msg;
  bool error;
public:
  CRF24Message(const u_int8_t pipe, const uint16_t uptime, const float voltage, const float temperature, const float humidity, const float baro_pressure);
  CRF24Message(const u_int8_t pipe, const void* buf, const uint8_t length);

  static uint8_t getMessageLength() { return sizeof(r24_message_uvthp_t); }
  
  virtual const void* getMessageBuffer() { return &msg; } 
  virtual const bool isError() { return error; }
  virtual const String getString();
  virtual const uint8_t getId() { return msg.id; }

  uint32_t getUptime();
  void setUptime(uint32_t value);

  float getVoltage();
  void setVoltage(float value);

  float getTemperature();
  void setTemperature(float value);

  float getHumidity();
  void setHumidity(float value);

  float getBaroPressure();
  void setBaroPressure(float value);

};