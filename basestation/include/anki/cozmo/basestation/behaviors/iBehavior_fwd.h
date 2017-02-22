/**
 * File: iBehavior_fwd.h
 *
 * Author: Brad Neuman
 * Created: 2017-02-17
 *
 * Description: Forward declarations for IBehavior
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_IBehavior_fwd_H__
#define __Cozmo_Basestation_Behaviors_IBehavior_fwd_H__

#include <functional>
#include "clad/types/actionResults.h"

namespace Anki {
namespace Cozmo {

class IBehavior;

enum class BehaviorStatus {
  Failure,
  Running,
  Complete
};

namespace ExternalInterface {
struct RobotCompletedAction;
}

class Robot;

using BehaviorRobotCompletedActionCallback = std::function<void(const ExternalInterface::RobotCompletedAction&)>;
using BehaviorActionResultCallback = std::function<void(ActionResult)>;
using BehaviorActionResultWithRobotCallback = std::function<void(ActionResult, Robot&)>;
using BehaviorSimpleCallback = std::function<void(void)>;
using BehaviorSimpleCallbackWithRobot = std::function<void(Robot& robot)>;


}
}


#endif
