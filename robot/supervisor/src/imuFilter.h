/**
 * File: imuFilter.h
 *
 * Author: Kevin Yoon
 * Created: 4/1/2014
 *
 * Description:
 *
 *   Filter for gyro and accelerometer
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef IMU_FILTER_H_
#define IMU_FILTER_H_

#include "anki/common/shared/radians.h"
#include "anki/common/types.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace IMUFilter {

      void Reset();
      
      // TODO: Add if/when needed?
      // ReturnCode Init();
      ReturnCode Update();

      
      f32 GetRotation();
      f32 GetRotationSpeed();
      
    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki

#endif // IMU_FILTER_H_
