#pragma once

#include "BaseMessage.h"

class IVEDMessageProvider {
public:
  virtual CBaseMessage* checkForMessage();
};
