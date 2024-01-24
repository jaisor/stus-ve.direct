#include <ArduinoLog.h>
#include <Arduino.h>
#include <stdexcept>

#include "RF24Message_VED_MPPT.h"

CRF24Message_VED_MPPT::CRF24Message_VED_MPPT(const u_int8_t pipe, const r24_message_ved_mppt_t msg)
: CBaseMessage(pipe) {  
  
  error = false;
  this->msg = msg;
  this->msg.id = MSG_VED_MPPT_ID;
  
}

CRF24Message_VED_MPPT::CRF24Message_VED_MPPT(const u_int8_t pipe, const void* buf, const uint8_t length)
: CBaseMessage(pipe) { 
  if (length != getMessageLength()) {
    error = true;
    return;
  }
  memcpy(&msg, buf, length <= getMessageLength() ? length : getMessageLength());
  if (msg.id != MSG_VED_MPPT_ID) {
    //Log.errorln(F("Message ID %i doesn't match MSG_VED_MPPT_ID(%i)"), msg.id, MSG_VED_MPPT_ID);
    memset(&msg, 0, getMessageLength());
    error = true;
  } else {
    error = false;
  }
}

const String CRF24Message_VED_MPPT::getString() {
  char c[255];
  snprintf_P(c, 255, PSTR("[%u] (V=%0.2fV, I=%0.2fA, VPV=%0.2fV, PPV=%0.2fW T=%0.2fC)"), pipe, 
        msg.b_voltage, msg.b_current, msg.p_voltage, msg.p_power, msg.temperature);
  Log.verboseln(F("CRF24Message_VED_MPPT::getString() : %s"), c);
  return String(c);
}