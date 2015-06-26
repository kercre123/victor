/**
 * File: streamedKeyframe.h
 *
 * Author: Andrew Stein
 * Created: 6/24/15
 *
 * Description:
 *
 *   Defines a streamed KeyFrame struct for streaming animations. It holds
 *   everything in a single container without unions, for simplicity vs. the
 *   old KeyFrame type.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef ANKI_COZMO_ROBOT_STREAMEDKEYFRAME_H
#define ANKI_COZMO_ROBOT_STREAMEDKEYFRAME_H

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoTypes.h"


namespace Anki {
  namespace Cozmo {
    
    struct StreamedKeyFrame
    {
      // Bit flag to specify which things this keyframe sets
      typedef enum {
        KF_SETS_AUDIO          = 0x01,
        KF_SETS_FACE_FRAME     = 0x02,
        KF_SETS_FACE_POSITION  = 0x04,
        KF_SETS_HEAD           = 0x08,
        KF_SETS_LIFT           = 0x10,
        KF_SETS_BACKPACK_LEDS  = 0x20,
        KF_SETS_WHEELS         = 0x40,
        KF_IS_TERMINATION      = 0x80
      } WhichTracks;
      
      u8 setsWhichTracks; // bits set using the enum values above
      
      // Audio
      u8 audioSample[HAL::AUDIO_SAMPLE_SIZE];
      
      // Face
      s8 faceCenX, faceCenY;
      u8 faceFrame[HAL::MAX_FACE_FRAME_SIZE];
      
      // Head speed in rad/sec
      // TODO: discretize / use fixed point
      //f32 headSpeed;
      
      // Desired head angle and how long from "now" we want to reach that angle
      // NOTE: we store angle and time instead of storing speed directly so that
      //  the robot can compute the speed on the fly, using the *actual* current
      //  angle of the head when this keyframe is played, rather than the
      //  *assumed* angle we expect to be at.
      s8  headAngle_deg;
      u16 headTime_ms;
      
      
      // Lift speed in mm/sec
      // TODO: discretize / use fixed point
      //f32 liftSpeed;
      
      // Desired lift height and how long from "now" we want to reach that height
      // (See "NOTE" above for head angle)
      u8  liftHeight_mm;
      u16 liftTime_ms;
      
      // Backpack light colors
      u32 backpackLEDs[NUM_BACKPACK_LEDS];
      
      // Wheel speeds
      f32 wheelSpeedL, wheelSpeedR;
      
    }; // StreamedKeyFrame

  } // namesapce Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_STREAMEDKEYFRAME_H
