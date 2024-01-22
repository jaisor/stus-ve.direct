#pragma once

#include <Arduino.h>

#include "Configuration.h"

class CBaseMessage {

protected:
    unsigned long tMillis;
    const u_int8_t pipe;

public:
	CBaseMessage(const u_int8_t pipe);
    const uint8_t getPipe() { return pipe; };
    virtual const String getString() { return String(""); }
};
