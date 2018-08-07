/**
 * File: proxSensors.h
 *
 * Author: Kevin Yoon
 * Created: 8/12/2014
 *
 * Description:
 *
 *   Reads proximity sensors and
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef PROX_SENSORS_H_
#define PROX_SENSORS_H_

#include "clad/types/proxMessages.h"

#include "coretech/common/shared/types.h"

namespace Anki {
  
  namespace Vector {
    
    namespace ProxSensors {

      // Since this re-enables 'cliff detect' and 'stop on cliff'
      // it should only be called when the robot disconnects,
      // otherwise you could desync stopOnCliff state with engine.
      void Reset();
      
      Result Update();

      void EnableCliffDetector(bool enable);
      
      void EnableStopOnCliff(bool enable);

      void EnableStopOnWhite(bool enable);

      bool IsAnyCliffDetected();

      bool IsCliffDetected(u32 ind);
      
      bool IsAnyWhiteDetected();

      bool IsWhiteDetected(u32 ind);

      void SetCliffDetectThreshold(u32 ind, u16 level);
      
      void SetAllCliffDetectThresholds(u16 level);
      
      u8 GetCliffDetectedFlags();
      
      u8 GetWhiteDetectedFlags();
      
      u16 GetCliffValue(u32 ind);

      // Get corrected ToF distance sensor data
      ProxSensorDataRaw GetProxData();

    } // namespace ProxSensors
  } // namespace Vector
} // namespace Anki

#endif // PROX_SENSORS_H_
