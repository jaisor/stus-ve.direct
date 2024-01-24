#pragma once

#include "BaseMessage.h"

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
