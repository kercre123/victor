#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

#include "anki/common/types.h"

// Shared types between basestation and robot

namespace Anki {
  namespace Cozmo {

    typedef enum {
      ISM_OFF,
      ISM_STREAM,
      ISM_SINGLE_SHOT
    } ImageSendMode_t;

    typedef enum {
      IE_NONE,
      IE_YUYV,
      IE_BAYER,
      IE_JPEG,
      IE_WEBP
    } ImageEncoding_t;

    typedef enum {
      DA_PICKUP_LOW = 0,  // Docking to block at level 0
      DA_PICKUP_HIGH,     // Docking to block at level 1
      DA_PLACE_HIGH,      // Placing block atop another block at level 0
      DA_PLACE_LOW,       // Placing block on level 0
      DA_RAMP_ASCEND,     // Going up a ramp
      DA_RAMP_DESCEND,    // Going down a ramp
      DA_CROSS_BRIDGE
    } DockAction_t;


    typedef enum {
      TM_NONE,

      // Attempts to dock to a block that is placed in front of it and then place it on a block behind it.
      TM_PICK_AND_PLACE,

      // Follows a changing straight line path. Tests path following during docking.
      TM_DOCK_PATH,

      // Follows an example path. Requires localization
      TM_PATH_FOLLOW,

      // Cycles through PathFollower convenience functions: DriveStraight, DriveArc, DrivePointTurn
      TM_PATH_FOLLOW_CONVENIENCE_FUNCTIONS,

      // Tests ExecuteDirectDrive() or open loop control via HAL::MotorSetPower()
      TM_DIRECT_DRIVE,

      // Moves lift up and down
      TM_LIFT,

      // Toggles between 3 main lift heights: low dock, carry, and high dock
      TM_LIFT_TOGGLE,

      // Tilts head up and down
      TM_HEAD,

      // Prints gyro/accel data
      TM_IMU,

      // Cycles through all known animations
      TM_ANIMATION,

#if defined(HAVE_ACTIVE_GRIPPER) && HAVE_ACTIVE_GRIPPER
      // Engages and disengages gripper
      TM_GRIPPER,
#endif

      // Cycle through all LEDs with different colors
      TM_LIGHTS,

      // Drives slow and then stops.
      // Drives fast and then stops.
      // Reports stopping distance and time (in tics).
      TM_STOP_TEST,

      // Drives all motors at max power simultaneously.
      TM_MAX_POWER_TEST,

      // Turns on face tracking
      TM_FACE_TRACKING,

      TM_NUM_TESTS
    } TestMode;

    typedef enum {
      DTF_ENABLE_DIRECT_HAL_TEST = 0x01,
      DTF_ENABLE_TOGGLE_DIR = 0x02
    } DriveTestFlags;
    
    typedef enum {
      LTF_CYCLE_ALL = 0x01
    } LightTestFlags;
    

    // Bit flags for RobotState message
    typedef enum {
      //IS_TRAVERSING_PATH    = 1,
      IS_CARRYING_BLOCK       = 0x2,
      IS_PICKING_OR_PLACING   = 0x4,
      IS_PICKED_UP            = 0x8,
      IS_PROX_FORWARD_BLOCKED = 0x10,
      IS_PROX_SIDE_BLOCKED    = 0x20,
      IS_ANIMATING            = 0x40
    } RobotStatusFlag;


    // A key associated with each computed pose retrieved from history
    // to be used to check its validity at a later time.
    typedef u32 HistPoseKey;


    // Animation ID
    // TODO: Eventually, we might want a way of sending animation definitions down from basestation
    //       but for now they're hard-coded on the robot
    /*typedef enum {
      ANIM_IDLE
      ,ANIM_HEAD_NOD
      ,ANIM_BACK_AND_FORTH_EXCITED
      ,ANIM_WIGGLE
      ,ANIM_NUM_ANIMATIONS
    } AnimationID_t;*/
    typedef s32 AnimationID_t;

    // Prox sensors
    typedef enum {
      PROX_LEFT
      ,PROX_FORWARD
      ,PROX_RIGHT
      ,NUM_PROX
    } ProxSensor_t;

    // LED identifiers and colors
    // Updated for "neutral" (non-hardware specific) order in 2.1
    enum LEDId {
      LED_RIGHT_EYE_TOP = 0,
      LED_RIGHT_EYE_RIGHT,
      LED_RIGHT_EYE_BOTTOM,
      LED_RIGHT_EYE_LEFT,
      LED_LEFT_EYE_TOP,
      LED_LEFT_EYE_RIGHT,
      LED_LEFT_EYE_BOTTOM,
      LED_LEFT_EYE_LEFT,
      NUM_LEDS
    };

    // The color format is identical to HTML Hex Triplets (RGB)
    enum LEDColor {
      LED_CURRENT_COLOR = 0xffffffff, // Don't change color: leave as is

      LED_OFF =   0x000000,
      LED_RED =   0xff0000,
      LED_GREEN = 0x00ff00,
      LED_YELLOW= 0xffff00,
      LED_BLUE =  0x0000ff,
      LED_PURPLE= 0xff00ff,
      LED_CYAN =  0x00ffff,
      LED_WHITE = 0xffffff
    };

    enum WhichEye {
      EYE_LEFT,
      EYE_RIGHT,
      EYE_BOTH
    };

    enum EyeShape {
      EYE_CURRENT_SHAPE = -1, // Don't change shape: leave as is
      EYE_OPEN,
      EYE_HALF,
      EYE_SLIT,
      EYE_CLOSED,
      EYE_OFF_PUPIL_UP,
      EYE_OFF_PUPIL_DOWN,
      EYE_OFF_PUPIL_LEFT,
      EYE_OFF_PUPIL_RIGHT,
      EYE_ON_PUPIL_UP,
      EYE_ON_PUPIL_DOWN,
      EYE_ON_PUPIL_LEFT,
      EYE_ON_PUPIL_RIGHT
    };

    // For DEV only
    typedef struct {
      s32 autoexposureOn;
      f32 exposureTime;      
      s32 integerCountsIncrement;
      f32 minExposureTime;
      f32 maxExposureTime;
      f32 percentileToMakeHigh;
      s32 limitFramerate;
      u8 highValue;
    } VisionSystemParams_t;

    typedef struct {
      f32 scaleFactor;
      s32 minNeighbors;
      s32 minObjectHeight;
      s32 minObjectWidth;
      s32 maxObjectHeight;
      s32 maxObjectWidth;
    } FaceDetectParams_t;

  }
}

#endif // COZMO_TYPES_H
