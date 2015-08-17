/**
 * File: behaviorInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
  const ActionList::SlotHandle IBehavior::sActionSlot = 0;

  IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _robot(robot)
  , _name("none")
  , _isRunning(false)
  {
  
  }
  
} // namespace Cozmo
} // namespace Anki