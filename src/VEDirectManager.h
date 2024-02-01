#pragma once

#include <map>

#include "BaseManager.h"
#include "VEDMessageProvider.h"
#include "SensorProvider.h"

class CVEDirectManager: public CBaseManager, public IVEDMessageProvider {

private:
  unsigned long tMillis;
  unsigned long tMillisError;
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
  uint8_t	mChecksum;
  char *mTextPointer;
  char mName[9];
  char mValue[33]; 

  CBaseMessage *msg;
  ISensorProvider* sensor;
  
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

  virtual CBaseMessage* checkForMessage() { return mState == IDLE ? msg : NULL; }
};
