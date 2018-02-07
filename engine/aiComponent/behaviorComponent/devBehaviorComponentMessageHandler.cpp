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

#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRerun.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const BehaviorID kBehaviorIDForDevMessage = BEHAVIOR_ID(DevExecuteBehaviorRerun);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::DevBehaviorComponentMessageHandler(Robot& robot)
: IDependencyManagedComponent<BCComponentID>(this, BCComponentID::DevBehaviorComponentMessageHandler)
, _robot(robot)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DevBehaviorComponentMessageHandler::~DevBehaviorComponentMessageHandler()
{
  _eventHandles.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBehaviorComponentMessageHandler::GetInitDependencies(BCCompIDSet& dependencies) const
{ 
  dependencies.insert(BCComponentID::BehaviorExternalInterface);
  dependencies.insert(BCComponentID::BehaviorSystemManager);
  dependencies.insert(BCComponentID::BehaviorContainer);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DevBehaviorComponentMessageHandler::InitDependent(Robot* robot, const BCCompMap& dependentComponents) 
{
  auto& bContainer = dependentComponents.GetValue<BehaviorContainer>(BCComponentID::BehaviorContainer);
  auto& bsm = dependentComponents.GetValue<BehaviorSystemManager>(BCComponentID::BehaviorSystemManager);
  auto& bei = dependentComponents.GetValue<BehaviorExternalInterface>(BCComponentID::BehaviorExternalInterface);

  if(_robot.HasExternalInterface()){
    auto handlerCallback = [this, &bContainer, &bsm, &bei](const GameToEngineEvent& event) {
      const auto& msg = event.GetData().Get_ExecuteBehaviorByID();
      
      BehaviorID behaviorID = BehaviorTypesWrapper::BehaviorIDFromString(msg.behaviorID);
      ICozmoBehaviorPtr behaviorToRun = bContainer.FindBehaviorByID(behaviorID);
      if(behaviorToRun != nullptr){
        ICozmoBehaviorPtr newRerunBaseBehavior =
          WrapRequestedBehaviorInDispatcherRerun(bContainer,
                                                 behaviorID,
                                                 msg.numRuns);
        newRerunBaseBehavior->Init(bei);
        bsm.ResetBehaviorStack(newRerunBaseBehavior.get());
        _rerunBehavior = newRerunBaseBehavior;
      }
    };

    _eventHandles.push_back(
      _robot.GetExternalInterface()->Subscribe(GameToEngineTag::ExecuteBehaviorByID, 
                                               handlerCallback)
    );
  }
};


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
  rerunDispatcher = bContainer.CreateBehaviorFromConfig(config);
  return rerunDispatcher;
}

  
} // namespace Cozmo
} // namespace Anki

