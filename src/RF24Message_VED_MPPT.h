#pragma once

#include "BaseMessage.h"

typedef struct r24_message_ved_mppt_t {
  uint8_t id;             // message id
  //
  float b_voltage;        // V
  float b_current;        // A - when > 0, the battery is being charged, < 0 the battery is being
  float p_voltage;        // V
  float p_power;          // W
  u_int8_t current_state;
  u_int8_t mppt;
  u_int8_t off_reason;
  u_int8_t error;
  //
  float today_yield;      // Wh
  float today_max_power;  // Wh
  //
  float temperature;      // C
} _r24_message_ved_mppt1_t;

class CRF24Message_VED_MPPT: public CBaseMessage {
private:
  r24_message_ved_mppt_t msg;
  bool error;
public:
  CRF24Message_VED_MPPT(const u_int8_t pipe, const r24_message_ved_mppt_t msg);
  CRF24Message_VED_MPPT(const u_int8_t pipe, const void* buf, const uint8_t length);

  static uint8_t getMessageLength() { return sizeof(r24_message_ved_mppt_t); }
  
  virtual const void* getMessageBuffer() { return &msg; } 
  virtual const bool isError() { return error; }
  virtual const String getString();
  virtual const uint8_t getId() { return msg.id; }
};
