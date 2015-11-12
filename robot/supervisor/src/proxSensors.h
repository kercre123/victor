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

#include "anki/types.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace ProxSensors {

      Result Update();

      // Returns the proximity sensor values
      void GetValues(u8 &left, u8 &forward, u8 &right);
      
      // Returns whether or not prox sensor beam is
      // blocked by the lift.
      bool IsSideBlocked();
      bool IsForwardBlocked();

      void EnableCliffDetector(bool enable);
      
    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki

#endif // PROX_SENSORS_H_
