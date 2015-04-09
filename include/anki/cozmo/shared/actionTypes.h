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
  
  enum RobotActionTypes : s32 {
    ACTION_UNKNOWN = -1,
    ACTION_DRIVE_TO_POSE,
    ACTION_DRIVE_TO_OBJECT,
    ACTION_DRIVE_TO_PLACE_CARRIED_OBJECT,
    ACTION_TURN_IN_PLACE,
    ACTION_MOVE_HEAD_TO_ANGLE,
    ACTION_PICKUP_OBJECT_LOW,
    ACTION_PICKUP_OBJECT_HIGH,
    ACTION_PLACE_OBJECT_LOW,
    ACTION_PLACE_OBJECT_HIGH,
    ACTION_CROSS_BRIDGE,
    ACTION_ASCEND_OR_DESCEND_RAMP,
    ACTION_TRAVERSE_OBJECT,
    ACTION_DRIVE_TO_AND_TRAVERSE_OBJECT,
    ACTION_PLAY_ANIMATION,
    ACTION_PLAY_SOUND,
    ACTION_WAIT
  };
  
} // namespace Cozmo
} // namesapce Anki

#endif // ANKI_COZMO_ROBOT_ACTION_ENUM_H
