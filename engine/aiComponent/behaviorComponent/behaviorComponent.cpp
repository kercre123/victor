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

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorAudioComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/faceWorld.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::BehaviorComponent()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::BehaviorComponent)
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::~BehaviorComponent()
{
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _comps->ClearEntity();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::GenerateManagedComponents(Robot& robot,
                                                  ComponentPtr& entity)
{
  //////
  // Begin comps by reference
  /////

  // AIComponent
  {
    DEV_ASSERT(entity->HasComponent(BCComponentID::AIComponent), "BehaviorComponent.GenerateManagedComponents.NoAIComponentProvided");
  }
  // Face World
  if(!entity->HasComponent(BCComponentID::FaceWorld)){
    entity->AddDependentComponent(BCComponentID::FaceWorld,
      robot.GetComponentPtr<FaceWorld>(), false);
  }

  // Block World
  if(!entity->HasComponent(BCComponentID::BlockWorld)){
    entity->AddDependentComponent(BCComponentID::BlockWorld,
      robot.GetComponentPtr<BlockWorld>(), false);
  }

  //////
  // End comps by reference
  /////

  // Async Message Component
  if(!entity->HasComponent(BCComponentID::AsyncMessageComponent)){
    auto messagePtr = new AsyncMessageGateComponent(robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                                                  robot.GetRobotMessageHandler());
    entity->AddDependentComponent(BCComponentID::AsyncMessageComponent, std::move(messagePtr));
  }

  // Behavior Audio Component
  if(!entity->HasComponent(BCComponentID::BehaviorAudioComponent)){
    auto audioPtr = new Audio::BehaviorAudioComponent(robot.GetAudioClient());
    entity->AddDependentComponent(BCComponentID::BehaviorAudioComponent,
                                  std::move(audioPtr));
  }

  // User intent component (and receiver of cloud intents)
  if(!entity->HasComponent(BCComponentID::UserIntentComponent)){
    entity->AddDependentComponent(BCComponentID::UserIntentComponent,
                                  new UserIntentComponent(robot, robot.GetContext()->GetDataLoader()->GetUserIntentConfig()));
  }

  // Behavior Container
  if(!entity->HasComponent(BCComponentID::BehaviorContainer)){
    if(robot.GetContext() != nullptr){
      auto bcPtr = new BehaviorContainer(robot.GetContext()->GetDataLoader()->GetBehaviorJsons());
      entity->AddDependentComponent(BCComponentID::BehaviorContainer, std::move(bcPtr));
    }else{
      BehaviorContainer::BehaviorIDJsonMap map;
      auto bcPtr = new BehaviorContainer(map);
      entity->AddDependentComponent(BCComponentID::BehaviorContainer, std::move(bcPtr));
    }
  }

  // BaseBehavior Wrapper
  if(!entity->HasComponent(BCComponentID::BaseBehaviorWrapper)){
    ICozmoBehaviorPtr baseBehavior;

    const CozmoContext* context = robot.GetContext();
    RobotDataLoader* dataLoader = nullptr;
    if(context == nullptr){
      PRINT_NAMED_WARNING("BehaviorComponent.GenerateManagedComponents.NoContext",
                          "wont be able to load behaviors from context componenets. May be OK in unit tests");
    }else{
      dataLoader = context->GetDataLoader();
    }
    Json::Value blankActivitiesConfig;
    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
      dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;


    BehaviorContainer& bc = entity->GetValue<BehaviorContainer>();
    if(!behaviorSystemConfig.empty()){
      BehaviorID baseBehaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
      baseBehavior = bc.FindBehaviorByID(baseBehaviorID);
      DEV_ASSERT(baseBehavior != nullptr,
                 "BehaviorComponent.Init.InvalidbaseBehavior");
    }else{
      // Need a base behavior, so make it base behavior wait
      Json::Value config = ICozmoBehavior::CreateDefaultBehaviorConfig(BEHAVIOR_CLASS(Wait), BEHAVIOR_ID(Wait));
      const bool ret = bc.CreateAndStoreBehavior(config);
      DEV_ASSERT(ret, "BehaviorComponent.CreateWaitBehavior.Failed");
      baseBehavior = bc.FindBehaviorByID(BEHAVIOR_ID(Wait));
      DEV_ASSERT(baseBehavior != nullptr, "BehaviorComponent.CreateWaitBehavior.WaitNotInContainers");
    }
    entity->AddDependentComponent(BCComponentID::BaseBehaviorWrapper,
                                  new BaseBehaviorWrapper(baseBehavior.get()));
  }

  // Behavior Event Anim Response Director
  if(!entity->HasComponent(BCComponentID::BehaviorEventAnimResponseDirector)){
    entity->AddDependentComponent(BCComponentID::BehaviorEventAnimResponseDirector,
                                  new BehaviorEventAnimResponseDirector());
  }

  // Behavior Event Component
  if(!entity->HasComponent(BCComponentID::BehaviorEventComponent)){
    entity->AddDependentComponent(BCComponentID::BehaviorEventComponent,
                                  new BehaviorEventComponent());
  }

  // Behavior External Interface
  if(!entity->HasComponent(BCComponentID::BehaviorExternalInterface)){
    entity->AddDependentComponent(BCComponentID::BehaviorExternalInterface,
                                  new BehaviorExternalInterface());
  }

  // Behavior System Manager
  if(!entity->HasComponent(BCComponentID::BehaviorSystemManager)){
    entity->AddDependentComponent(BCComponentID::BehaviorSystemManager,
                                  new BehaviorSystemManager());
  }

  // Behavior timers
  if(!entity->HasComponent(BCComponentID::BehaviorTimerManager)){
    entity->AddDependentComponent(BCComponentID::BehaviorTimerManager,
                                  new BehaviorTimerManager());
  }

  // Delegation Component
  if(!entity->HasComponent(BCComponentID::DelegationComponent)){
    entity->AddDependentComponent(BCComponentID::DelegationComponent,
                                  new DelegationComponent());
  }

  // Dev Behavior Component Message Handler
  if(!entity->HasComponent(BCComponentID::DevBehaviorComponentMessageHandler)){
    entity->AddDependentComponent(BCComponentID::DevBehaviorComponentMessageHandler,
                                  new DevBehaviorComponentMessageHandler(robot));
  }

  // Robot Info
  if(!entity->HasComponent(BCComponentID::RobotInfo)){
    entity->AddDependentComponent(BCComponentID::RobotInfo,
                                  new BEIRobotInfo(robot));
  }

  if(!entity->HasComponent(BCComponentID::ActiveFeature)) {
    entity->AddDependentComponent(BCComponentID::ActiveFeature, new ActiveFeatureComponent);
  }

  if(!entity->HasComponent(BCComponentID::ActiveBehaviorIterator)) {
    entity->AddDependentComponent(BCComponentID::ActiveBehaviorIterator, new ActiveBehaviorIterator);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SetComponents(ComponentPtr&& components)
{
  _comps = std::move(components);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitDependent(Robot* robot, const AICompMap& dependentComponents)
{
  if(_comps == nullptr){
    _comps = std::make_unique<EntityType>();
    _comps->AddDependentComponent(BCComponentID::AIComponent, robot->GetComponentPtr<AIComponent>(), false);
    BehaviorComponent::GenerateManagedComponents(*robot, _comps);
  }
  _comps->InitComponents(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::UpdateDependent(const AICompMap& dependentComps)
{
  _comps->UpdateComponents();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const
{
  AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>();
  gateComp.SubscribeToTags(subscriber, std::move(tags));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const
{
  AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>();
  gateComp.SubscribeToTags(subscriber, std::move(tags));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const
{
  AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>();
  gateComp.SubscribeToTags(subscriber, std::move(tags));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& BehaviorComponent::GetBehaviorContainer()
{
  return GetComponent<BehaviorContainer>();
}


} // namespace Cozmo
} // namespace Anki
