/**
 * File: ICozmoBehavior_fwd.h
 *
 * Author: Brad Neuman
 * Created: 2017-02-17
 *
 * Description: Forward declarations for ICozmoBehavior
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_ICozmoBehavior_fwd_H__
#define __Cozmo_Basestation_Behaviors_ICozmoBehavior_fwd_H__

#include <functional>
#include "clad/types/actionResults.h"

namespace Anki {
namespace Cozmo {

class ICozmoBehavior;

enum class BehaviorStatus {
  Failure,
  Running,
  Complete
};

namespace ExternalInterface {
struct RobotCompletedAction;
}

class BehaviorExternalInterface;

using ICozmoBehaviorPtr                                              = std::shared_ptr<ICozmoBehavior>;
using BehaviorRobotCompletedActionCallback                      = std::function<void(const ExternalInterface::RobotCompletedAction&)>;
using BehaviorRobotCompletedActionWithExternalInterfaceCallback = std::function<void(const ExternalInterface::RobotCompletedAction&, BehaviorExternalInterface&)>;
using BehaviorActionResultCallback                              = std::function<void(ActionResult)>;
using BehaviorActionResultWithExternalInterfaceCallback         = std::function<void(const ActionResult&, BehaviorExternalInterface&)>;
using BehaviorSimpleCallback                                    = std::function<void(void)>;
using BehaviorSimpleCallbackWithExternalInterface               = std::function<void(BehaviorExternalInterface& behaviorExternalInterface)>;
using BehaviorStatusCallbackWithExternalInterface               = std::function<BehaviorStatus(BehaviorExternalInterface& behaviorExternalInterface)>;


}
}


#endif
