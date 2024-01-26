#include <ArduinoLog.h>
#include <Arduino.h>
#include <stdexcept>

#include "RF24Message.h"

CRF24Message::CRF24Message(const u_int8_t pipe, const r24_message_uvthp_t msg)
: CBaseMessage(pipe), msg(msg) {  
  
  error = false;
  this->msg.id = MSG_UVTHP_ID;
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

const String CRF24Message::getString() {
  char c[255];
  snprintf_P(c, 255, PSTR("[%u] (V=%0.2fV, T=%0.2fC, H=%0.2f%%, BP=%0.2fKPa U=%0.2fsec)"), pipe, 
        msg.voltage, msg.temperature, msg.humidity, msg.baro_pressure/1000.0, (float)(msg.uptime)/1000.0);
  Log.verboseln(F("CRF24Message::getString() : %s"), c);
  return String(c);
}