/**
 * File: actionTypes.h
 *
 * Author: Andrew Stein
 * Date:   4/8/2015
 *
 * Description: Defines enumerated list of action types to be used for 
 *              IActionRunner::GetType() derived implementations.
 *
 * Copyright: Anki, Inc. 2015
 **/

// TODO: Generate this from a CLAD enum generator so it can be used by CSharp code too

#ifndef ANKI_COZMO_ROBOT_ACTION_ENUM_H
#define ANKI_COZMO_ROBOT_ACTION_ENUM_H

namespace Anki {
namespace Cozmo {
  
  enum class RobotActionType : s32 {
    COMPOUND = -2,
    UNKNOWN = -1,
    DRIVE_TO_POSE,
    DRIVE_TO_OBJECT,
    DRIVE_TO_PLACE_CARRIED_OBJECT,
    TURN_IN_PLACE,
    MOVE_HEAD_TO_ANGLE,
    PICKUP_OBJECT_LOW,         // Possible type of PickAndPlaceAction
    PICKUP_OBJECT_HIGH,        //   "
    PLACE_OBJECT_LOW,          //   "
    PLACE_OBJECT_HIGH,         //   "
    PICK_AND_PLACE_INCOMPLETE, // type for PickAndPlaceAction if fail before SelectDockAction ()
    CROSS_BRIDGE,
    ASCEND_OR_DESCEND_RAMP,
    TRAVERSE_OBJECT,
    DRIVE_TO_AND_TRAVERSE_OBJECT,
    FACE_OBJECT,
    VISUALLY_VERIFY_OBJECT,
    PLAY_ANIMATION,
    PLAY_SOUND,
    WAIT,
    MOVE_LIFT_TO_HEIGHT
  };
  
  enum class ActionResult : s32 {
    SUCCESS = 0,
    RUNNING,
    FAILURE_TIMEOUT,
    FAILURE_PROCEED,
    FAILURE_RETRY,
    FAILURE_ABORT,
    CANCELLED
  };
  
} // namespace Cozmo
} // namesapce Anki

#endif // ANKI_COZMO_ROBOT_ACTION_ENUM_H
