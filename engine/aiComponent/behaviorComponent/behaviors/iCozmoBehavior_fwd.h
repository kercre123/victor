/**
 * File: iCozmoBehavior_fwd.h
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
#include "engine/robotInterface/messageHandler.h"
#include "clad/types/actionResults.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"

namespace Anki {
namespace Cozmo {

class ICozmoBehavior;
enum class BehaviorID: uint8_t;
enum class BehaviorClass: uint8_t;
enum class ExecutableBehaviorType: uint8_t;

enum class BehaviorStatus {
  Failure,
  Running,
  Complete
};

namespace ExternalInterface {
struct RobotCompletedAction;
}
  
template<typename TYPE> class AnkiEvent;

using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
using RobotToEngineEvent= AnkiEvent<RobotInterface::RobotToEngine>;
using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
using GameToEngineTag   = ExternalInterface::MessageGameToEngineTag;

class BehaviorExternalInterface;

using ICozmoBehaviorPtr                                         = std::shared_ptr<ICozmoBehavior>;
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
