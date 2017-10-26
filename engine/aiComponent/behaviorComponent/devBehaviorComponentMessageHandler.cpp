/**
* File: devBehaviorComponentMessageHandler.cpp
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

#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatchers/behaviorDispatcherRerun.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const BehaviorID kBehaviorIDForDevMessage = BehaviorID::DevExecuteBehaviorRerun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::DevBehaviorComponentMessageHandler(Robot& robot, BehaviorComponent& bComponent, BehaviorContainer& bContainer)
: _behaviorComponent(bComponent)
{
  if(robot.HasExternalInterface()){
    auto handlerCallback = [this, &bContainer](const GameToEngineEvent& event) {
      const auto& msg = event.GetData().Get_ExecuteBehaviorByID();
      
      ICozmoBehaviorPtr behaviorToRun = bContainer.FindBehaviorByID(msg.behaviorID);
      if(behaviorToRun != nullptr){
        ICozmoBehaviorPtr rerunBehavior =
           WrapRequestedBehaviorInDispatcherRerun(bContainer,
                                                  msg.behaviorID,
                                                  msg.numRuns);
        rerunBehavior->Init(_behaviorComponent._components->_behaviorExternalInterface);
        auto& subComps = _behaviorComponent._components;
        subComps->_behaviorSysMgr.ResetBehaviorStack(rerunBehavior.get());
      }
    };

    _eventHandles.push_back(
      robot.GetExternalInterface()->Subscribe(GameToEngineTag::ExecuteBehaviorByID, 
                                              handlerCallback)
    );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::~DevBehaviorComponentMessageHandler()
{
  _eventHandles.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr DevBehaviorComponentMessageHandler::WrapRequestedBehaviorInDispatcherRerun(BehaviorContainer& bContainer, 
                                                                                             BehaviorID requestedBehaviorID, 
                                                                                             const int numRuns)
{
  // See if behaviorID has already been created
  ICozmoBehaviorPtr rerunDispatcher = bContainer.FindBehaviorByID(kBehaviorIDForDevMessage);
  if(rerunDispatcher != nullptr){
    bContainer.RemoveBehaviorFromMap(rerunDispatcher);
  }

  Json::Value config = BehaviorDispatcherRerun::CreateConfig(kBehaviorIDForDevMessage, requestedBehaviorID, numRuns);
  rerunDispatcher = bContainer.CreateBehavior(config);
  return rerunDispatcher;
}

  
} // namespace Cozmo
} // namespace Anki

