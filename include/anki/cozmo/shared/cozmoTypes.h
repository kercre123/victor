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
      DA_PICKUP_LOW = 0,  // Docking to block at level 0
      DA_PICKUP_HIGH,     // Docking to block at level 1
      DA_PLACE_HIGH,      // Placing block atop another block at level 0
      DA_PLACE_LOW,       // Placing block on level 0
      DA_ROLL_LOW,        // Rolling a block at level 0 by pulling it towards you
      DA_RAMP_ASCEND,     // Going up a ramp
      DA_RAMP_DESCEND,    // Going down a ramp
      DA_CROSS_BRIDGE,    // Crossing a bridge
      DA_MOUNT_CHARGER    // Reversing onto charger
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

      // Cycle through all LEDs with different colors
      TM_LIGHTS,

      // Draw and blink a test face
      TM_FACE_DISPLAY,
      
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
      DTF_ENABLE_DIRECT_HAL_TEST = 0x01,    // When enabled, speed parameter (p3) is interpretted as a percentage power value
      DTF_ENABLE_CYCLE_SPEEDS_TEST = 0x02,  // When enabled, cycles through different motor power values going up by powerPercentStep (p2)
      DTF_ENABLE_TOGGLE_DIR = 0x04          // When enabled, toggles direction going at the specified speed (p3)
    } DriveTestFlags;
    
    typedef enum {
      LiftTF_TEST_POWER = 0,
      LiftTF_TEST_HEIGHTS,
      LiftTF_NODDING,
      LiftTF_DISABLE_MOTOR
    } LiftTestFlags;

    typedef enum {
      HTF_TEST_POWER = 0,
      HTF_TEST_ANGLES,
      HTF_NODDING
    } HeadTestFlags;
    
    typedef enum {
      ITF_DO_TURNS = 0x01
    } IMUTestFlags;
    
    typedef enum {
      LTF_CYCLE_ALL = 0x01
    } LightTestFlags;
    

    // Bit flags for RobotState message
    typedef enum {
      IS_MOVING               = 0x00000001,  // Head, lift, or wheels
      IS_CARRYING_BLOCK       = 0x00000002,
      IS_PICKING_OR_PLACING   = 0x00000004,
      IS_PICKED_UP            = 0x00000008,
      IS_PROX_FORWARD_BLOCKED = 0x00000010,
      IS_PROX_SIDE_BLOCKED    = 0x00000020,
      IS_ANIMATING            = 0x00000040,
      IS_PERFORMING_ACTION    = 0x00000080,
      LIFT_IN_POS             = 0x00000100,
      HEAD_IN_POS             = 0x00000200,
      IS_ANIM_BUFFER_FULL     = 0x00000400,
      IS_ANIMATING_IDLE       = 0x00000800
    } RobotStatusFlag;
    
    // Status bit flags for game state
    // TODO: This belongs in cozmo-game rather than cozmo-engine
    typedef enum {
      IS_LOCALIZED            = 0x00000001
    } GameStatusFlag;


    // Bit flags for enabling/disabling animation tracks on the robot
    typedef enum {
      DISABLE_ALL_TRACKS = 0,
      HEAD_TRACK = 0x01,
      LIFT_TRACK = 0x02,
      BODY_TRACK = 0x04,
      FACE_IMAGE_TRACK = 0x08,
      FACE_POS_TRACK   = 0x10,
      BACKPACK_LIGHTS_TRACK = 0x20,
      AUDIO_TRACK = 0x40,
      BLINK_TRACK = 0x80,
      ENABLE_ALL_TRACKS = 0xFF
    } AnimTrackFlag;

    // A key associated with each computed pose retrieved from history
    // to be used to check its validity at a later time.
    typedef u32 HistPoseKey;

    // Prox sensors
    typedef enum {
      PROX_LEFT
      ,PROX_FORWARD
      ,PROX_RIGHT
      ,NUM_PROX
    } ProxSensor_t;


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

    
    typedef enum {
      SAVE_OFF = 0,
      SAVE_ONE_SHOT,
      SAVE_CONTINUOUS
    } SaveMode_t;
    
    typedef enum {
      CARRY_NONE = 0,
      CARRY_1_BLOCK,
      CARRY_2_BLOCK,
      NUM_CARRY_STATES
    } CarryState_t;
    
  }
}

#endif // COZMO_TYPES_H
