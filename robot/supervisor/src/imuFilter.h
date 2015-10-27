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
#include "anki/types.h"
#include "anki/cozmo/robot/hal.h"

namespace Anki {
  
  namespace Cozmo {
    
    namespace IMUFilter {

      void Reset();
      
      // TODO: Add if/when needed?
      // ReturnCode Init();
      Result Update();
      
      // Returns the latest IMU data read in the last Update() call.
      HAL::IMU_DataStructure GetLatestRawData();

      // Rotation (or "yaw"). Turning left is positive.
      f32 GetRotation();
      f32 GetRotationSpeed();
      
      // Angle above gravity horizontal
      f32 GetPitch();

      // Starts recording a buffer of data for the specified time and sends it to basestation
      void RecordAndSend( const u32 length_ms );
      
      // Returns true when pickup detected.
      // Pickup detect is reset when the robot stops moving.
      bool IsPickedUp();
      
    } // namespace IMUFilter
  } // namespace Cozmo
} // namespace Anki

#endif // IMU_FILTER_H_
