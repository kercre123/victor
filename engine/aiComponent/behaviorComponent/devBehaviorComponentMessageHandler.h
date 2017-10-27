/**
* File: devBehaviorComponentMessageHandler.h
*
* Author: Kevin M. Karol
* Created: 10/24/17
*
* Description: Component that receives dev messages and performs actions on
* the behaviorComponent in response
*
* Copyright: Anki, Inc. 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__
#define __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__

#include "clad/externalInterface/messageEngineToGameTag.h"
#include "clad/externalInterface/messageGameToEngineTag.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// forward declarations
class Robot;
class BehaviorComponent;
class BehaviorContainer;

class DevBehaviorComponentMessageHandler : private Util::noncopyable
{
public:
  DevBehaviorComponentMessageHandler(Robot& robot, BehaviorComponent& bComponent, BehaviorContainer& bContainer);
  virtual ~DevBehaviorComponentMessageHandler();
  
private:
  BehaviorComponent& _behaviorComponent;
  std::vector<::Signal::SmartHandle> _eventHandles;

  ICozmoBehaviorPtr WrapRequestedBehaviorInDispatcherRerun(BehaviorContainer& bContainer, 
                                                           BehaviorID requestedBehaviorID, 
                                                           const int numRuns);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_DevBehaviorComponentMessageHandler_H__
