#pragma once

#include <map>

#include "BaseManager.h"
#include "SensorProvider.h"


class CVEDirectManager: public CBaseManager {

private:
  unsigned long tMillis;
  
  ISensorProvider* sensor;
  bool jobDone;

  Stream *VEDirectStream;
  std::map<String, String> mVEData;

  enum States {
        IDLE,
        RECORD_BEGIN,
        RECORD_NAME,
        RECORD_VALUE,
        CHECKSUM,
        RECORD_HEX
    };

  int mState;
  char* mTextPointer;
  char mName[9];
  char mValue[33];  
  uint8_t	mChecksum;
  
  void rxData(uint8_t inbyte);
  bool hexRxEvent(uint8_t inbyte);
  void frameEndEvent(bool valid);
  
public:
	CVEDirectManager(ISensorProvider* sensor);
  virtual ~CVEDirectManager();

  // CBaseManager
  virtual void loop();
  virtual void powerDown();
  virtual void powerUp();
  virtual const bool isJobDone() { return jobDone; }
};
