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

      void EnableCliffDetector(bool enable);
      
      void EnableStopOnCliff(bool enable);
      
    } // namespace ProxSensors
  } // namespace Cozmo
} // namespace Anki

#endif // PROX_SENSORS_H_
