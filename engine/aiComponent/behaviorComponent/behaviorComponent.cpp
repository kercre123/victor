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
#include "engine/aiComponent/behaviorComponent/attentionTransferComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponentMessageHandler.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorAudioComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "engine/aiComponent/behaviorComponent/userDefinedBehaviorTreeComponent/userDefinedBehaviorTreeComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/powerStateManager.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/faceWorld.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

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
    DEV_ASSERT(entity->HasComponent<AIComponent>(), "BehaviorComponent.GenerateManagedComponents.NoAIComponentProvided");
  }
  // Face World
  if(!entity->HasComponent<FaceWorld>()){
    entity->AddDependentComponent(BCComponentID::FaceWorld,
      robot.GetComponentPtr<FaceWorld>(), false);
  }

  // Block World
  if(!entity->HasComponent<BlockWorld>()){
    entity->AddDependentComponent(BCComponentID::BlockWorld,
      robot.GetComponentPtr<BlockWorld>(), false);
  }

  if(!entity->HasComponent<RobotStatsTracker>()) {
    entity->AddDependentComponent(BCComponentID::RobotStatsTracker,
      robot.GetComponentPtr<RobotStatsTracker>(), false);
  }

  if(!entity->HasComponent<PowerStateManager>()) {
    entity->AddDependentComponent(BCComponentID::PowerStateManager,
      robot.GetComponentPtr<PowerStateManager>(), false);
  }

  //////
  // End comps by reference
  /////

  // Async Message Component
  if(!entity->HasComponent<AsyncMessageGateComponent>()){
    auto messagePtr = new AsyncMessageGateComponent(robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                                    robot.HasGatewayInterface() ? robot.GetGatewayInterface() : nullptr,
                                                                  robot.GetRobotMessageHandler());
    entity->AddDependentComponent(BCComponentID::AsyncMessageComponent, std::move(messagePtr));
  }

  // Behavior Audio Component
  if(!entity->HasComponent<Audio::BehaviorAudioComponent>()){
    auto audioPtr = new Audio::BehaviorAudioComponent(robot.GetAudioClient());
    entity->AddDependentComponent(BCComponentID::BehaviorAudioComponent,
                                  std::move(audioPtr));
  }

  // User intent component (and receiver of cloud intents)
  if(!entity->HasComponent<UserIntentComponent>()){
    entity->AddDependentComponent(BCComponentID::UserIntentComponent,
                                  new UserIntentComponent(robot, robot.GetContext()->GetDataLoader()->GetUserIntentConfig()));
  }

  // Behavior Container
  if(!entity->HasComponent<BehaviorContainer>()){
    if(robot.GetContext() != nullptr){
      auto bcPtr = new BehaviorContainer(robot.GetContext()->GetDataLoader()->GetBehaviorJsons());
      entity->AddDependentComponent(BCComponentID::BehaviorContainer, std::move(bcPtr));
    }else{
      BehaviorContainer::BehaviorIDJsonMap map;
      auto bcPtr = new BehaviorContainer(map);
      entity->AddDependentComponent(BCComponentID::BehaviorContainer, std::move(bcPtr));
    }
  }

  // Behavior Event Anim Response Director
  if(!entity->HasComponent<BehaviorEventAnimResponseDirector>()){
    entity->AddDependentComponent(BCComponentID::BehaviorEventAnimResponseDirector,
                                  new BehaviorEventAnimResponseDirector());
  }

  // Behavior Event Component
  if(!entity->HasComponent<BehaviorEventComponent>()){
    entity->AddDependentComponent(BCComponentID::BehaviorEventComponent,
                                  new BehaviorEventComponent());
  }

  // Behavior External Interface
  if(!entity->HasComponent<BehaviorExternalInterface>()){
    entity->AddDependentComponent(BCComponentID::BehaviorExternalInterface,
                                  new BehaviorExternalInterface());
  }

  // Behavior System Manager
  if(!entity->HasComponent<BehaviorSystemManager>()){
    entity->AddDependentComponent(BCComponentID::BehaviorSystemManager,
                                  new BehaviorSystemManager());
  }

  // Behavior timers
  if(!entity->HasComponent<BehaviorTimerManager>()){
    entity->AddDependentComponent(BCComponentID::BehaviorTimerManager,
                                  new BehaviorTimerManager());
  }

  // Delegation Component
  if(!entity->HasComponent<DelegationComponent>()){
    entity->AddDependentComponent(BCComponentID::DelegationComponent,
                                  new DelegationComponent());
  }

  // Dev Behavior Component Message Handler
  if(!entity->HasComponent<BehaviorComponentMessageHandler>()){
    entity->AddDependentComponent(BCComponentID::BehaviorComponentMessageHandler,
                                  new BehaviorComponentMessageHandler(robot));
  }

  // Robot Info
  if(!entity->HasComponent<BEIRobotInfo>()){
    entity->AddDependentComponent(BCComponentID::RobotInfo,
                                  new BEIRobotInfo(robot));
  }

  // Robot Info
  if(!entity->HasComponent<UserDefinedBehaviorTreeComponent>()){
    entity->AddDependentComponent(BCComponentID::UserDefinedBehaviorTreeComponent,
                                  new UserDefinedBehaviorTreeComponent());
  }

  if(!entity->HasComponent<ActiveFeatureComponent>()) {
    entity->AddDependentComponent(BCComponentID::ActiveFeature, new ActiveFeatureComponent);
  }

  if(!entity->HasComponent<ActiveBehaviorIterator>()) {
    entity->AddDependentComponent(BCComponentID::ActiveBehaviorIterator, new ActiveBehaviorIterator);
  }

  if(!entity->HasComponent<AttentionTransferComponent>()) {
    entity->AddDependentComponent(BCComponentID::AttentionTransferComponent, new AttentionTransferComponent);
  }
  
  if(!entity->HasComponent<BehaviorsBootLoader>()) {
    const CozmoContext* context = robot.GetContext();
    RobotDataLoader* dataLoader = nullptr;
    if(context == nullptr){
      PRINT_NAMED_WARNING("BehaviorComponent.GenerateManagedComponents.NoContext",
                          "wont be able to load behaviors from context componenets. May be OK in unit tests");
    }else{
      dataLoader = context->GetDataLoader();
    }
    Json::Value blankActivitiesConfig;
    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr)
                                              ? dataLoader->GetVictorFreeplayBehaviorConfig()
                                              : blankActivitiesConfig;
    
    entity->AddDependentComponent(BCComponentID::BehaviorsBootLoader, new BehaviorsBootLoader{behaviorSystemConfig});    
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SetComponents(ComponentPtr&& components)
{
  _comps = std::move(components);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitDependent(Robot* robot, const AICompMap& dependentComps)
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
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<AppToEngineTag>&& tags) const
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
