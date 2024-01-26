#pragma once

#include "Configuration.h"
#include "BaseMessage.h"

class IVEDMessageProvider {
public:
  virtual CBaseMessage* checkForMessage();
};
