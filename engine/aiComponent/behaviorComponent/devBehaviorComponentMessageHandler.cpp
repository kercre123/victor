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
#include "webServerProcess/src/webService.h"

namespace Anki {
namespace Cozmo {

namespace {
static const BehaviorID kBehaviorIDForDevMessage = BEHAVIOR_ID(DevExecuteBehaviorRerun);
const std::string kWebVizModuleName = "behaviors";
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
  auto& bContainer = dependentComponents.GetValue<BehaviorContainer>();
  auto& bsm = dependentComponents.GetValue<BehaviorSystemManager>();
  auto& bei = dependentComponents.GetValue<BehaviorExternalInterface>();

  if(_robot.HasExternalInterface()){
    
    SubscribeToWebViz( bei, bsm );
    
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
  

void DevBehaviorComponentMessageHandler::SubscribeToWebViz(BehaviorExternalInterface& bei, const BehaviorSystemManager& bsm)
{
  DEV_ASSERT( _eventHandles.empty(), "only call once" );
  
  const auto* context = _robot.GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      
      auto onSubscribed = [&bei, &bsm](const std::function<void(const Json::Value&)>& sendToClient) {
        // resend just that client the new tree
        Json::Value data = bsm.BuildDebugBehaviorTree( bei );
        sendToClient( data );
        
        // also send them the list of behaviorIDs that can be created
        Json::Value allBehaviors;
        auto& list = allBehaviors["list"];
        for( uint8_t i=0; i<BehaviorTypesWrapper::GetBehaviorIDNumEntries(); ++i ) {
          list.append( EnumToString( static_cast<BehaviorID>(i) ) );
        }
        sendToClient( list );
      };
      auto onData = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
        // client wants us to run a specific behavior
        const auto& name = data["behaviorName"];
        if( name.isString() ) {
          PRINT_CH_DEBUG("BehaviorSystem",
                         "BehaviorStack.SubscribeToWebViz.Transition",
                         "WebViz just instructed us to transition to '%s'",
                         name.asString().c_str());
          auto* ei = _robot.GetExternalInterface();
          const int numRuns = 1;
          using namespace ExternalInterface;
          ei->Broadcast(MessageGameToEngine(ExecuteBehaviorByID( name.asString(), numRuns )));
        }
      };
      
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleName ).ScopedSubscribe( onSubscribed ) );
      _eventHandles.emplace_back( webService->OnWebVizData( kWebVizModuleName ).ScopedSubscribe( onData ) );
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki

