#pragma once

#include <RF24.h>

#include "BaseManager.h"
#include "SensorProvider.h"
#include "VEDMessageProvider.h"

class CRF24Manager: public CBaseManager {

private:
  unsigned long tMillis;
  uint8_t retries;

  RF24 *radio;

  ISensorProvider* sensor;
  IVEDMessageProvider *vedProvider;
  bool jobDone;
    
public:
	CRF24Manager(ISensorProvider* sensor, IVEDMessageProvider *vedProvider);
  virtual ~CRF24Manager();

  // CBaseManager
  virtual void loop();
  virtual void powerDown();
  virtual void powerUp();
  virtual const bool isJobDone() { return jobDone; }
};
