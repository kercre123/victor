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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponentCloudReceiver.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorAudioComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/faceWorld.h"
#include "engine/audio/engineRobotAudioClient.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::BCComponentWrapper::BCComponentWrapper(DependencyManagedEntity<BCComponentID, BCComponentID::Count>&& entity)
: _array({
  {BCComponentID::AIComponent,                        entity.GetComponent(BCComponentID::AIComponent)},
  {BCComponentID::AsyncMessageComponent,              entity.GetComponent(BCComponentID::AsyncMessageComponent)},
  {BCComponentID::BehaviorAudioComponent,             entity.GetComponent(BCComponentID::BehaviorAudioComponent)},
  {BCComponentID::BehaviorComponentCloudReceiver,     entity.GetComponent(BCComponentID::BehaviorComponentCloudReceiver)},
  {BCComponentID::BehaviorContainer,                  entity.GetComponent(BCComponentID::BehaviorContainer)},
  {BCComponentID::BehaviorEventAnimResponseDirector,  entity.GetComponent(BCComponentID::BehaviorEventAnimResponseDirector)},
  {BCComponentID::BehaviorEventComponent,             entity.GetComponent(BCComponentID::BehaviorEventComponent)},
  {BCComponentID::BehaviorExternalInterface,          entity.GetComponent(BCComponentID::BehaviorExternalInterface)},
  {BCComponentID::BehaviorHelperComponent,            entity.GetComponent(BCComponentID::BehaviorHelperComponent)},
  {BCComponentID::BehaviorSystemManager,              entity.GetComponent(BCComponentID::BehaviorSystemManager)},
  {BCComponentID::BlockWorld,                         entity.GetComponent(BCComponentID::BlockWorld)},
  {BCComponentID::DelegationComponent,                entity.GetComponent(BCComponentID::DelegationComponent)},
  {BCComponentID::DevBehaviorComponentMessageHandler, entity.GetComponent(BCComponentID::DevBehaviorComponentMessageHandler)},
  {BCComponentID::FaceWorld,                          entity.GetComponent(BCComponentID::FaceWorld)},
  {BCComponentID::RobotInfo,                          entity.GetComponent(BCComponentID::RobotInfo)},
  {BCComponentID::BaseBehaviorWrapper,                entity.GetComponent(BCComponentID::BaseBehaviorWrapper)}
})
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::BehaviorComponent()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::~BehaviorComponent()
{
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _components.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::UniqueComponents BehaviorComponent::GenerateManagedComponents(
  Robot& robot,
  AIComponent&               aiComponent,
  IBehavior*                 baseBehavior,
  BehaviorSystemManager*     behaviorSysMgrPtr,
  BehaviorExternalInterface* behaviorExternalInterfacePtr,
  BehaviorContainer*         behaviorContainerPtr, 
  BehaviorEventComponent*    behaviorEventComponentPtr,
  AsyncMessageGateComponent* asyncMessageComponentPtr,
  DelegationComponent*       delegationComponentPtr)
{
  DependencyManagedEntity<BCComponentID, BCComponentID::Count> entity;

  // AIComponent
  {  
    BCComp unmanagedAI(&aiComponent);
    entity.AddDependentComponent(BCComponentID::AIComponent, std::move(unmanagedAI));
  }
  
  // Async Message Component
  {
    bool manageMessageComp = false;
    if(asyncMessageComponentPtr == nullptr){
      asyncMessageComponentPtr = new AsyncMessageGateComponent(robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                                               robot.GetRobotMessageHandler(),
                                                               robot.GetID());
      manageMessageComp = true;
    }
    BCComp messageComp(asyncMessageComponentPtr, manageMessageComp);
    entity.AddDependentComponent(BCComponentID::AsyncMessageComponent, std::move(messageComp));
  }
 
  // Behavior Audio Component
  {
    auto audioComp = new Audio::BehaviorAudioComponent(robot.GetAudioClient());
    BCComp managedAudio(audioComp, true);
    entity.AddDependentComponent(BCComponentID::BehaviorAudioComponent, std::move(managedAudio));
  }

  // Cloud Receiver
  {
    auto cloudReceiver = new BehaviorComponentCloudReceiver(robot);
    BCComp managedCloud(cloudReceiver, true);
    entity.AddDependentComponent(BCComponentID::BehaviorComponentCloudReceiver, std::move(managedCloud));
  }

  // Behavior Container
  {
    bool manageBC = false;
    if(behaviorContainerPtr == nullptr){
      if(robot.GetContext() != nullptr){
        behaviorContainerPtr = new BehaviorContainer(robot.GetContext()->GetDataLoader()->GetBehaviorJsons());
      }else{
        BehaviorContainer::BehaviorIDJsonMap empty;
        behaviorContainerPtr = new BehaviorContainer(empty);
      }
      manageBC = true;
    }
    BCComp behaviorContainerComp(behaviorContainerPtr, manageBC);
    entity.AddDependentComponent(BCComponentID::BehaviorContainer, std::move(behaviorContainerComp));
  }
  
  // BaseBehavior Wrapper
  {
    if(baseBehavior == nullptr){
      Json::Value blankActivitiesConfig;
      
      const CozmoContext* context = robot.GetContext();
      
      if(context == nullptr){
        PRINT_NAMED_WARNING("BehaviorComponent.Init.NoContext",
                            "wont be able to load some componenets. May be OK in unit tests");
      }
      
      RobotDataLoader* dataLoader = nullptr;
      if(context){
        dataLoader = context->GetDataLoader();
      }
      
      const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
      dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;
      
      BehaviorContainer& bc = entity.GetComponent(BCComponentID::BehaviorContainer).GetValue<BehaviorContainer>();
      if(!behaviorSystemConfig.empty()){
        BehaviorID baseBehaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
        baseBehavior = bc.FindBehaviorByID(baseBehaviorID).get();
        DEV_ASSERT(baseBehavior != nullptr,
                   "BehaviorComponent.Init.InvalidbaseBehavior");
      }else{
        // Need a base behavior, so make it base behavior wait
        Json::Value config = ICozmoBehavior::CreateDefaultBehaviorConfig(BEHAVIOR_CLASS(Wait), BEHAVIOR_ID(Wait));
        bc.CreateBehaviorFromConfig(config);
        baseBehavior = bc.FindBehaviorByID(BEHAVIOR_ID(Wait)).get();
      }
    }
    BaseBehaviorWrapper* wrapper = new BaseBehaviorWrapper(baseBehavior);
    BCComp managedBaseBehaviorWrapper(wrapper, true);
    entity.AddDependentComponent(BCComponentID::BaseBehaviorWrapper, std::move(managedBaseBehaviorWrapper));
  }

  // Behavior Event Anim Response Director
  {
    auto animDirector = new BehaviorEventAnimResponseDirector();
    BCComp managedAnimDirector(animDirector, true);
    entity.AddDependentComponent(BCComponentID::BehaviorEventAnimResponseDirector, std::move(managedAnimDirector));
  }

  // Behavior Event Component
  {
    bool manageBehaviorEvent = false;
    if(behaviorEventComponentPtr == nullptr){
      behaviorEventComponentPtr = new BehaviorEventComponent();
      manageBehaviorEvent = true;
    }
    BCComp eventComp(behaviorEventComponentPtr, manageBehaviorEvent);
    entity.AddDependentComponent(BCComponentID::BehaviorEventComponent, std::move(eventComp));
  }

  // Behavior External Interface
  {
    bool manageBEI = false;
    if(behaviorExternalInterfacePtr == nullptr){
      behaviorExternalInterfacePtr = new BehaviorExternalInterface();
      manageBEI = true;
    }
    BCComp bei(behaviorExternalInterfacePtr, manageBEI);
    entity.AddDependentComponent(BCComponentID::BehaviorExternalInterface, std::move(bei));
  }

  // Behavior Helper Component
  {
    auto helperComp = new BehaviorHelperComponent();
    BCComp managedHelper(helperComp, true);
    entity.AddDependentComponent(BCComponentID::BehaviorHelperComponent, std::move(managedHelper));
  }

  // Behavior System Manager
  {
    bool manageBSM = false;
    if(behaviorSysMgrPtr == nullptr){
      behaviorSysMgrPtr = new BehaviorSystemManager();
      manageBSM = true;
    }
    BCComp managedBSM(behaviorSysMgrPtr, manageBSM);
    entity.AddDependentComponent(BCComponentID::BehaviorSystemManager, std::move(managedBSM));
  }
  
  // Block World
  {
    BCComp unmanagedBlockWorld(&robot.GetBlockWorld());
    entity.AddDependentComponent(BCComponentID::BlockWorld, std::move(unmanagedBlockWorld));
  }

  // Delegation Component
  {
    bool manageDelegationComp = false;
    if(delegationComponentPtr == nullptr){
      delegationComponentPtr = new DelegationComponent();
      manageDelegationComp = true;
    }
    BCComp delegationComp(delegationComponentPtr, manageDelegationComp);
    entity.AddDependentComponent(BCComponentID::DelegationComponent, std::move(delegationComp));
  }

  // Dev Behavior Component Message Handler
  {
    auto devBehaviorMessageHandler = new DevBehaviorComponentMessageHandler(robot);
    BCComp devBehaviorComp(devBehaviorMessageHandler, true);
    entity.AddDependentComponent(BCComponentID::DevBehaviorComponentMessageHandler, std::move(devBehaviorComp));
  }

  // Face World
  {
    BCComp unmanagedFaceWorld(&robot.GetFaceWorld());
    entity.AddDependentComponent(BCComponentID::FaceWorld, std::move(unmanagedFaceWorld));
  }

  // Robot Info
  {
    auto robotInfo = new BEIRobotInfo(robot);
    BCComp robotInfoComp(robotInfo, true);
    entity.AddDependentComponent(BCComponentID::RobotInfo, std::move(robotInfoComp));
  }

  return std::make_unique<BCComponentWrapper>(std::move(entity));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Init(Robot* robot, UniqueComponents&& components)
{
  _components = std::move(components);
  _components->_array.InitComponents(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Update(Robot& robot,
                               std::string& currentActivityName,
                               std::string& behaviorDebugStr)
{

  BehaviorExternalInterface& bei = GetComponent<BehaviorExternalInterface>(BCComponentID::BehaviorExternalInterface);

  GetBehaviorHelperComponent().Update(bei);
  
  {
    BehaviorSystemManager& bsm = GetComponent<BehaviorSystemManager>(BCComponentID::BehaviorSystemManager);
    bsm.Update(bei);
  }
  
  robot.GetContext()->GetVizManager()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                               "%s", behaviorDebugStr.c_str());
  
  robot.GetContext()->SetSdkStatus(SdkStatusType::Behavior,
                                   std::string(currentActivityName) + std::string(":") + behaviorDebugStr);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageGameToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_components != nullptr,
                 "BehaviorComponent.SubscribeToTags.NocomponentsPtr",
                 "")){
    AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>(BCComponentID::AsyncMessageComponent);
    gateComp.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const
{
  if(ANKI_VERIFY(_components != nullptr,
                 "BehaviorComponent.SubscribeToTags.NocomponentsPtr",
                 "")){
    AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>(BCComponentID::AsyncMessageComponent);
    gateComp.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_components != nullptr,
                 "BehaviorComponent.SubscribeToTags.NocomponentsPtr",
                 "")){
    AsyncMessageGateComponent& gateComp = GetComponent<AsyncMessageGateComponent>(BCComponentID::AsyncMessageComponent);
    gateComp.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& BehaviorComponent::GetBehaviorContainer()
{
  return GetComponent<BehaviorContainer>(BCComponentID::BehaviorContainer);
}

  
} // namespace Cozmo
} // namespace Anki

