#pragma once

#include <RF24.h>

#include "BaseManager.h"
#include "VEDMessageProvider.h"

class CRF24Manager: public CBaseManager {

private:
  unsigned long tMillis, tsLastTransmit;
  uint8_t retries;
  bool error;

  RF24 *radio;

  IVEDMessageProvider *vedProvider;
  bool jobDone;
  uint8_t transmittedCount;
    
public:
	CRF24Manager(IVEDMessageProvider *vedProvider);
  virtual ~CRF24Manager();

  // CBaseManager
  virtual void loop();
  virtual void powerDown();
  virtual void powerUp();
  virtual const bool isJobDone() { return jobDone; }
  virtual const bool isError() { return error; }
};
