/**
 * File: keyFrame.h
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Defines a KeyFrame for animations.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COZMO_ROBOT_KEYFRAME_H
#define ANKI_COZMO_ROBOT_KEYFRAME_H

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/cozmoTypes.h"


namespace Anki {
namespace Cozmo {

struct KeyFrame
{
  // Add a new KeyFrame Type by adding it to this enumerated list and then
  // defining it as a new struct in the union below.
  enum Type
  {
    HEAD_ANGLE = 0,
    START_HEAD_NOD,
    STOP_HEAD_NOD,
    LIFT_HEIGHT,
    DRIVE_LINE_SEGMENT,
    DRIVE_ARC,
    BACK_AND_FORTH,
    START_WIGGLE,
    POINT_TURN,
    PLAY_SOUND,
    EYE_BLINK,
    SET_LED_COLORS,
    START_LIFT_NOD,
    STOP_LIFT_NOD,
    NUM_TYPES
  };

  
  // Common Keyframe elements:
  Type           type;
  u16            relTime_ms; // time relative to first keyframe
  
  KeyFrameTransitionType transitionIn;
  KeyFrameTransitionType transitionOut;
  
  // Define all the possible type-specific KeyFrame info as structs here, and
  // then create an "instance" of each of these in the union below. It would
  // be cleaner to do an anonymous definition inside the union directly, but
  // Keil does not allow that.
  
  // Directly set the head's angle and speed
  struct SetHeadAngle_t {
    f32 targetAngle;
    f32 targetSpeed;
  };
  
  // Command a canned head nodding action between two angles
  // Must be used in conjunction with a StopHeadNod_t keyframe after it.
  struct StartHeadNod_t {
    f32 lowAngle;  // radians
    f32 highAngle; // radians
    u16 period_ms;
  };
  
  struct StopHeadNod_t {
    f32 finalAngle;
  };
  
  // Directly set lift's height and speed
  struct SetLiftHeight_t {
    u16 targetHeight; // mm
    u16 targetSpeed;  // mm/s
  };
  
  // Command a canned lift nodding action between two heights
  // Must be used in conjunction with a StopLiftNod_t keyframe after it.
  struct StartLiftNod_t {
    u8 lowHeight;  // mm
    u8 highHeight; // mm
    u16 period_ms;
  };
  
  struct StopLiftNod_t {
    u8 finalHeight;
  };
  
  struct DriveLineSegment_t {
    s16 relativeDistance; // in mm, +ve for fwd, -ve for backward
    f32 targetSpeed;
  };
  
  struct DriveArc_t {
    f32 radius;
    f32 sweepAngle; // +ve arcs left, -ve arcs right
    f32 targetSpeed;
  };
  
  // Drive forward and backward primitive
  // (Can use different forward/backward distances to get a net "shimmy"
  //  forward or backward)
  struct BackAndForth_t {
    u32 period;
    u8  forwardDist_mm;
    u8  backwardDist_mm;
  };
  
  // Side-to-side body wiggle primitive
  // (Can use different left/right angles to get a net "shimmy" left or right)
  struct StartWiggle_t {
    u32 period;
    u8  leftAngle_rad;
    u8  rightAngle_rad;
  };
  
  struct StopWiggle_t {
    
  };
  
  // Turn in place primitive
  struct TurnInPlace_t {
    f32 relativeAngle; // +ve turns left, -ve turns right
    f32 targetSpeed;
  };
  
  struct PlaySound_t {
    SoundID_t soundID;
    u8        numLoops;
  };
  
  // Turn eye(s) off and back on in specified color (use 0x000000 for
  // color to use the current LED color)
  struct BlinkEyes_t {
    // Can specify which eye(s) to blink in order to get winking
    enum WhichEye_t {
      LEFT_EYE   = 0,
      RIGHT_EYE  = 1,
      BOTH_EYES  = 2
    }        whichEye;
    
    u32      period; // for fast vs. slow blinks
    u32      color;
  };
  
  // Set the color for all LEDs
  //
  struct SetLEDcolors_t {
    u32 led[NUM_LEDS];
  };
  
  // Using a union of structs here (determined by a check of the Type above)
  // instead of inheritance.
  union {
    
    // NOTE: Keil does not like defining structs inside anonymous unions, so
    // we define them outside and "instantiate" them each here. We _could_
    // use anonymous structs and then do the definitions here as well, but
    // then things get really confusing. Seems better to group the type-specific
    // info by name this way.
    
    SetHeadAngle_t     SetHeadAngle;
    StartHeadNod_t     StartHeadNod;
    StopHeadNod_t      StopHeadNod;
    SetLiftHeight_t    SetLiftHeight;
    StartLiftNod_t     StartLiftNod;
    StopLiftNod_t      StopLiftNod;
    DriveLineSegment_t DriveLineSegment;
    DriveArc_t         DriveArc;
    BackAndForth_t     BackAndForth;
    StartWiggle_t      StartWiggle;
    StopWiggle_t       StopWiggle;
    TurnInPlace_t      TurnInPlace;
    PlaySound_t        PlaySound;
    BlinkEyes_t        BlinkEyes;
    SetLEDcolors_t     SetLEDcolors;
    
  }; // union of structs
  
  // Use this value to skip setting a given LED color and leave it at whatever
  // color it was.
  static const u32 UNSPECIFIED_COLOR = 0xff000000;
    
  void TransitionOutOf(const u32 animStartTime_ms) const;
  void TransitionInto(const u32 animStartTime_ms)  const;

  // Returns true if lift is at the target height, or head is at the target
  // angle, etc.
  bool IsInPosition();
  
  // Perform any necessary type-dependent stopping function
  // (E.g., for HeadNodding, call StopNodding())
  void Stop();
  
}; // struct KeyFrame

  /*
KeyFrame::KeyFrame(Type t)
: type(t)
{
  
  // Initialize type-specific members:
  switch(type)
  {
    case SET_LED_COLORS:
    {
      for(s32 iLED = 0; iLED < HAL::NUM_LEDS; ++iLED) {
        SetLEDcolors.led[iLED] = UNSPECIFIED_COLOR;
      }
    }
  }
}
  */
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ROBOT_KEYFRAME_H

