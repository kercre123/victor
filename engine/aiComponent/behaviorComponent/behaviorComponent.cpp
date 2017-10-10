/**
* File: behaviorComponent.cpp
*
* Author: Kevin M. Karol
* Created: 9/22/17
*
* Description: Component responsible for maintaining all aspects of the AI system
* relating to behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"

#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorAudioComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/devBaseRunnable.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::BehaviorComponent(Robot& robot)
: _behaviorEventAnimResponseDirector(new BehaviorEventAnimResponseDirector())
, _behaviorHelperComponent( new BehaviorHelperComponent())
, _behaviorContainer(new BehaviorContainer(robot.GetContext()->GetDataLoader()->GetBehaviorJsons()))
, _audioClient(new Audio::BehaviorAudioComponent(robot.GetRobotAudioClient()))
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::~BehaviorComponent()
{
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _behaviorMgr.reset();
  _behaviorSysMgr.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Init(Robot& robot)
{
  _behaviorMgr = std::make_unique<BehaviorManager>();
  _behaviorSysMgr = std::make_unique<BehaviorSystemManager>();
  
  _delegationComponent.reset(new DelegationComponent(robot, *_behaviorSysMgr));
  _stateChangeComponent.reset(new StateChangeComponent(*this));
  
  _behaviorExternalInterface = std::make_unique<BehaviorExternalInterface>(robot,
                                                                           robot.GetAIComponent(),
                                                                           *_behaviorContainer,
                                                                           robot.GetBlockWorld(),
                                                                           robot.GetFaceWorld(),
                                                                           *_stateChangeComponent);
  
  _behaviorExternalInterface->SetOptionalInterfaces(_delegationComponent.get(),
                                                    &robot.GetMoodManager(),
                                                    robot.GetContext()->GetNeedsManager(),
                                                    &robot.GetProgressionUnlockComponent(),
                                                    &robot.GetPublicStateBroadcaster());

  _asyncMessageComponent.reset(new AsyncMessageGateComponent(
     robot.HasExternalInterface() ? robot.GetExternalInterface(): nullptr,
     robot.GetRobotMessageHandler(),
     robot.GetID()));

  assert(_behaviorContainer);
  _behaviorContainer->Init(*_behaviorExternalInterface, !static_cast<bool>(USE_BSM));
  
  _audioClient->Init(*_behaviorExternalInterface);

  // Configure behavior manager
  {
    Json::Value blankActivitiesConfig;
    
    const CozmoContext* context = robot.GetContext();
    
    if(context == nullptr ) {
      PRINT_NAMED_WARNING("BehaviorComponent.Init.NoContext",
                          "wont be able to load some componenets. May be OK in unit tests");
    }
    
    RobotDataLoader* dataLoader = nullptr;
    if(context){
      dataLoader = robot.GetContext()->GetDataLoader();
    }
    
    const Json::Value& oldActivitesConfig = (dataLoader != nullptr) ?
              dataLoader->GetLegacyCozmoActivitiesConfig() : blankActivitiesConfig;
    
    const Json::Value& reactionTriggerConfig = (dataLoader != nullptr) ?
              dataLoader->GetReactionTriggerMap() : blankActivitiesConfig;
    
    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
              dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;
    
    _behaviorMgr->InitConfiguration(*_behaviorExternalInterface,
                                    robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                    oldActivitesConfig);
    _behaviorMgr->InitReactionTriggerMap(*_behaviorExternalInterface,
                                         reactionTriggerConfig);
    
    if(USE_BSM){
      if(!behaviorSystemConfig.empty()){
        BehaviorID id = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
        
        ICozmoBehaviorPtr dataBasedRunnable = _behaviorContainer->FindBehaviorByID(id);
        DEV_ASSERT(dataBasedRunnable != nullptr,
                   "BehaviorComponent.Init.InvalidBaseRunnable");
        
        IBehavior* baseRunnable = dataBasedRunnable.get();
        if( ANKI_DEV_CHEATS ) {
          // create a dev base layer to put on the bottom, and pass the desired base in so that DevBaseRunnable
          // will automatically delegate to it
          baseRunnable = new DevBaseRunnable( dataBasedRunnable.get() );
          baseRunnable->Init( *_behaviorExternalInterface);
        }
        
        _behaviorSysMgr->InitConfiguration(*_behaviorExternalInterface,
                                           _asyncMessageComponent.get(),
                                           baseRunnable);
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Update(Robot& robot,
                               std::string& currentActivityName,
                               std::string& behaviorDebugStr)
{
  _behaviorHelperComponent->Update(*_behaviorExternalInterface);

  if(USE_BSM){
    _behaviorSysMgr->Update(*_behaviorExternalInterface);
  }else{
    // https://ankiinc.atlassian.net/browse/COZMO-1242 : moving too early causes pose offset
    static int ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 = 60;
    if(ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 <=0)
    {
      ICozmoBehaviorPtr currentBehavior;

      Result res = _behaviorMgr->Update(*_behaviorExternalInterface);
      if(res == RESULT_OK){
        currentActivityName = _behaviorMgr->GetCurrentActivity()->GetIDStr();
        
        behaviorDebugStr = currentActivityName;
        
        currentBehavior = _behaviorMgr->GetCurrentBehavior();
      }
      
      if(currentBehavior != nullptr) {
        behaviorDebugStr += " ";
        behaviorDebugStr +=  BehaviorIDToString(currentBehavior->GetID());
        const std::string& stateName = currentBehavior->GetDebugStateName();
        if (!stateName.empty())
        {
          behaviorDebugStr += "-" + stateName;
        }
      }
      
    } else {
      --ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242;
    }
  }
  
  robot.GetContext()->GetVizManager()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                               "%s", behaviorDebugStr.c_str());
  
  robot.GetContext()->SetSdkStatus(SdkStatusType::Behavior,
                                   std::string(currentActivityName) + std::string(":") + behaviorDebugStr);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::OnRobotDelocalized()
{
  if(!USE_BSM){
    _behaviorMgr->OnRobotDelocalized();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const
{
  _asyncMessageComponent->SubscribeToTags(subscriber, std::move(tags));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const
{
  _asyncMessageComponent->SubscribeToTags(subscriber, std::move(tags));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const
{
  _asyncMessageComponent->SubscribeToTags(subscriber, std::move(tags));
}

  
} // namespace Cozmo
} // namespace Anki

