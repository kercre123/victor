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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/devBehaviorComponentMessageHandler.h"
#include "engine/aiComponent/behaviorEventAnimResponseDirector.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/audio/engineRobotAudioClient.h"

#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

namespace ComponentWrappers{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponentComponents::BehaviorComponentComponents(Robot& robot,
                                                         AIComponent& aiComponent,
                                                         BlockWorld& blockWorld,
                                                         FaceWorld& faceWorld,
                                                         BehaviorSystemManager& behaviorSysMgr,
                                                         BehaviorExternalInterface& behaviorExternalInterface,
                                                         BehaviorContainer& behaviorContainer,
                                                         BehaviorEventComponent& behaviorEventComponent,
                                                         AsyncMessageGateComponent& asyncMessageComponent,
                                                         DelegationComponent& delegationComponent)
: _robot(robot)
, _aiComponent(aiComponent)
, _blockWorld(blockWorld)
, _faceWorld(faceWorld)
, _behaviorSysMgr(behaviorSysMgr)
, _behaviorExternalInterface(behaviorExternalInterface)
, _behaviorContainer(behaviorContainer)
, _behaviorEventComponent(behaviorEventComponent)
, _asyncMessageComponent(asyncMessageComponent)
, _delegationComponent(delegationComponent)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponentComponents::~BehaviorComponentComponents()
{
  
}
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::BehaviorComponent()
: _behaviorEventAnimResponseDirector(new BehaviorEventAnimResponseDirector())
, _behaviorHelperComponent( new BehaviorHelperComponent())
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
BehaviorComponent::ComponentsPtr BehaviorComponent::GenerateComponents(
                                      Robot& robot,
                                      BehaviorSystemManager*     behaviorSysMgrPtr,
                                      BehaviorExternalInterface* behaviorExternalInterfacePtr,
                                      BehaviorContainer*         behaviorContainerPtr,
                                      BehaviorEventComponent*      behaviorEventComponentPtr,
                                      AsyncMessageGateComponent* asyncMessageComponentPtr,
                                      DelegationComponent*       delegationComponentPtr)
{
  
  std::unique_ptr<BehaviorSystemManager> behaviorSysMgr;
  if(behaviorSysMgrPtr == nullptr){
    behaviorSysMgr = std::make_unique<BehaviorSystemManager>();
    behaviorSysMgrPtr = behaviorSysMgr.get();
  }
  
  std::unique_ptr<BehaviorExternalInterface> behaviorExternalInterface;
  if(behaviorExternalInterfacePtr == nullptr){
    behaviorExternalInterface = std::make_unique<BehaviorExternalInterface>();
    behaviorExternalInterfacePtr = behaviorExternalInterface.get();
  }
  
  std::unique_ptr<BehaviorContainer> behaviorContainer;
  if(behaviorContainerPtr == nullptr){
    if(robot.GetContext() != nullptr){
      behaviorContainer = std::make_unique<BehaviorContainer>(
                      robot.GetContext()->GetDataLoader()->GetBehaviorJsons());
    }else{
      BehaviorContainer::BehaviorIDJsonMap emptyMap;
      behaviorContainer = std::make_unique<BehaviorContainer>(emptyMap);
    }
    behaviorContainerPtr = behaviorContainer.get();
  }
  
  std::unique_ptr<BehaviorEventComponent> behaviorEventComponent;
  if(behaviorEventComponentPtr == nullptr){
    behaviorEventComponent = std::make_unique<BehaviorEventComponent>();
    behaviorEventComponentPtr = behaviorEventComponent.get();
  }
  
  std::unique_ptr<AsyncMessageGateComponent> asyncMessageComponent;
  if(asyncMessageComponentPtr == nullptr){
    asyncMessageComponent = std::make_unique<AsyncMessageGateComponent>(
             robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
             robot.GetRobotMessageHandler(),
             robot.GetID());
    asyncMessageComponentPtr = asyncMessageComponent.get();
  }
  
  std::unique_ptr<DelegationComponent> delegationComponent;
  if(delegationComponentPtr == nullptr){
    delegationComponent = std::make_unique<DelegationComponent>();
    delegationComponentPtr = delegationComponent.get();
  }
  
  ComponentsPtr components = std::make_unique<ComponentWrappers::BehaviorComponentComponents>
     (robot,
      robot.GetAIComponent(),
      robot.GetBlockWorld(),
      robot.GetFaceWorld(),
      *behaviorSysMgrPtr,
      *behaviorExternalInterfacePtr,
      *behaviorContainerPtr,
      *behaviorEventComponentPtr,
      *asyncMessageComponentPtr,
      *delegationComponentPtr);

  components->_behaviorSysMgrPtr            = std::move(behaviorSysMgr);
  components->_behaviorExternalInterfacePtr = std::move(behaviorExternalInterface);
  components->_behaviorContainerPtr         = std::move(behaviorContainer);
  components->_behaviorEventComponentPtr      = std::move(behaviorEventComponent);
  components->_asyncMessageComponentPtr     = std::move(asyncMessageComponent);
  components->_delegationComponentPtr       = std::move(delegationComponent);
  
  return components;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitializeSubComponents(Robot& robot,
                                                IBehavior* baseBehavior,
                                                BehaviorSystemManager& behaviorSysMgr,
                                                IBehaviorMessageSubscriber& messageSubscriber,
                                                BehaviorExternalInterface& behaviorExternalInterface,
                                                AIComponent& aiComponent,
                                                BehaviorContainer& behaviorContainer,
                                                BlockWorld& blockWorld,
                                                FaceWorld& faceWorld,
                                                BehaviorEventComponent& behaviorEventComponent,
                                                AsyncMessageGateComponent& asyncMessageComponent,
                                                DelegationComponent& delegationComponent)
{  
  delegationComponent.Init(robot, behaviorSysMgr);
  behaviorEventComponent.Init(messageSubscriber);
  behaviorExternalInterface.Init(robot,
                                 aiComponent,
                                 behaviorContainer,
                                 blockWorld,
                                 faceWorld,
                                 behaviorEventComponent);
  
  behaviorContainer.Init(behaviorExternalInterface);

  behaviorSysMgr.InitConfiguration(baseBehavior,
                                   behaviorExternalInterface,
                                   &asyncMessageComponent);
  
  //aiComponent.Init(robot, this);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Init(ComponentsPtr&& components, IBehavior* baseBehavior)
{
  _components = std::move(components);
  InitHelper(baseBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitHelper(IBehavior* baseBehavior)
{
  _audioClient.reset(new Audio::BehaviorAudioComponent(_components->_robot.GetAudioClient()));  
  
  // Extract base behavior from config if not passed in through init
  if(baseBehavior == nullptr){
    Json::Value blankActivitiesConfig;
    
    const CozmoContext* context = _components->_robot.GetContext();
    
    if(context == nullptr ) {
      PRINT_NAMED_WARNING("BehaviorComponent.Init.NoContext",
                          "wont be able to load some componenets. May be OK in unit tests");
    }
    
    RobotDataLoader* dataLoader = nullptr;
    if(context){
      dataLoader = _components->_robot.GetContext()->GetDataLoader();
    }
    
    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
         dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;
    
    if(!behaviorSystemConfig.empty()){
      BehaviorID baseBehaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
      baseBehavior = _components->_behaviorContainer.FindBehaviorByID(baseBehaviorID).get();
      DEV_ASSERT(baseBehavior != nullptr,
                  "BehaviorComponent.Init.InvalidbaseBehavior");
    }else{
      // Need a base behavior, so make it base behavior wait
      Json::Value config = ICozmoBehavior::CreateDefaultBehaviorConfig(
        BEHAVIOR_CLASS(Wait), BEHAVIOR_ID(Wait));
      _components->_behaviorContainer.CreateBehaviorFromConfig(config);
      baseBehavior = _components->_behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(Wait)).get();
    }
  }
  
  InitializeSubComponents(_components->_robot,
                          baseBehavior,
                          _components->_behaviorSysMgr,
                          *this,
                          _components->_behaviorExternalInterface,
                          _components->_aiComponent,
                          _components->_behaviorContainer,
                          _components->_blockWorld,
                          _components->_faceWorld,
                          _components->_behaviorEventComponent,
                          _components->_asyncMessageComponent,
                          _components->_delegationComponent
  );

  _components->_behaviorExternalInterface.SetOptionalInterfaces(
                    &_components->_delegationComponent,
                    &_components->_robot.GetMoodManager(),
                    _components->_robot.GetContext()->GetNeedsManager(),
                    &_components->_robot.GetProgressionUnlockComponent(),
                    &_components->_robot.GetPublicStateBroadcaster());

  _audioClient->Init(_components->_behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Update(Robot& robot,
                               std::string& currentActivityName,
                               std::string& behaviorDebugStr)
{
  if(_messageHandler == nullptr){
    _messageHandler.reset(new DevBehaviorComponentMessageHandler(robot, *this, _components->_behaviorContainer));
  }

  if(_cloudReceiver == nullptr){
    _cloudReceiver.reset(new BehaviorComponentCloudReceiver(robot));
  }

  _behaviorHelperComponent->Update(_components->_behaviorExternalInterface);

  _components->_behaviorSysMgr.Update(_components->_behaviorExternalInterface);
  
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
    _components->_asyncMessageComponent.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<ExternalInterface::MessageEngineToGameTag>&& tags) const
{
  if(ANKI_VERIFY(_components != nullptr,
                 "BehaviorComponent.SubscribeToTags.NocomponentsPtr",
                 "")){
    _components->_asyncMessageComponent.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::SubscribeToTags(IBehavior* subscriber, std::set<RobotInterface::RobotToEngineTag>&& tags) const
{
  if(ANKI_VERIFY(_components != nullptr,
                 "BehaviorComponent.SubscribeToTags.NocomponentsPtr",
                 "")){
    _components->_asyncMessageComponent.SubscribeToTags(subscriber, std::move(tags));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& BehaviorComponent::GetBehaviorContainer()
{
  assert(_components); return _components->_behaviorContainer;
}

  
} // namespace Cozmo
} // namespace Anki

