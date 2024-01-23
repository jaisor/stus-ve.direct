#pragma once

#include "BaseMessage.h"

typedef struct r24_message_ved_inv_t {
  uint8_t id;             // message id
  //
  float b_voltage;        // V
  float ac_current;       // A - when > 0, the battery is being charged, < 0 the battery is being
  float ac_voltage;       // V
  float ac_va_power;      // VA
  u_int8_t current_state;
  int8_t mode;
  u_int8_t off_reason;
  u_int8_t alarm;
  u_int8_t warning;
  //
  float temperature;      // C
} _r24_message_ved_inv_t;

class CRF24Message_VED_INV: public CBaseMessage {
private:
  r24_message_ved_inv_t msg;
  bool error;
public:
  CRF24Message_VED_INV(const u_int8_t pipe, const r24_message_ved_inv_t msg);
  CRF24Message_VED_INV(const u_int8_t pipe, const void* buf, const uint8_t length);

  static uint8_t getMessageLength() { return sizeof(r24_message_ved_inv_t); }
  
  virtual const void* getMessageBuffer() { return &msg; } 
  virtual const bool isError() { return error; }
  virtual const String getString();
  virtual const uint8_t getId() { return msg.id; }
};
