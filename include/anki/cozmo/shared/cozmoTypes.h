#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

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
      DA_PLACE_LOW        // Placing block on level 0
    } DockAction_t;

    
    typedef enum {
      TM_NONE,
      
      // Attempts to dock to a block that is placed in front of it and then place it on a block behind it.
      TM_PICK_AND_PLACE,
      
      // Follows a changing straight line path. Tests path following during docking.
      TM_DOCK_PATH,
      
      // Follows an example path. Requires localization
      TM_PATH_FOLLOW,
      
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
      
      TM_NUM_TESTS
    } TestMode;

    
    // Bit flags for RobotState message
    typedef enum {
      IS_TRAVERSING_PATH    = 1,
      IS_CARRYING_BLOCK     = 2,
      IS_PICKING_OR_PLACING = 4
    } RobotStatusFlag;
    
  }
}

#endif // COZMO_TYPES_H
