#pragma once

#include "BaseManager.h"
#include "SensorProvider.h"

#include <VeDirectFrameHandler.h>

class CVEDirectManager: public CBaseManager {

private:
  unsigned long tMillis;
  
  ISensorProvider* sensor;
  bool jobDone;

  Stream *VEDirectStream;
  VeDirectFrameHandler VEDirectHandler;

  void LogHelper(const char* module, const char* error);
    
public:
	CVEDirectManager(ISensorProvider* sensor);
  virtual ~CVEDirectManager();

  // CBaseManager
  virtual void loop();
  virtual void powerDown();
  virtual void powerUp();
  virtual const bool isJobDone() { return jobDone; }
};
