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

namespace ComponentWrappers{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponentComponents::BehaviorComponentComponents(Robot& robot,
                                                         AIComponent& aiComponent,
                                                         BlockWorld& blockWorld,
                                                         FaceWorld& faceWorld,
                                                         BehaviorSystemManager& behaviorSysMgr,
                                                         BehaviorExternalInterface& behaviorExternalInterface,
                                                         BehaviorContainer& behaviorContainer,
                                                         StateChangeComponent& stateChangeComponent,
                                                         AsyncMessageGateComponent& asyncMessageComponent,
                                                         DelegationComponent& delegationComponent)
: _robot(robot)
, _aiComponent(aiComponent)
, _blockWorld(blockWorld)
, _faceWorld(faceWorld)
, _behaviorSysMgr(behaviorSysMgr)
, _behaviorExternalInterface(behaviorExternalInterface)
, _behaviorContainer(behaviorContainer)
, _stateChangeComponent(stateChangeComponent)
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
  _behaviorMgr.reset();
  _components.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorComponent::ComponentsPtr BehaviorComponent::GenerateComponents(
                                      Robot& robot,
                                      BehaviorSystemManager*     behaviorSysMgrPtr,
                                      BehaviorExternalInterface* behaviorExternalInterfacePtr,
                                      BehaviorContainer*         behaviorContainerPtr,
                                      StateChangeComponent*      stateChangeComponentPtr,
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
  
  std::unique_ptr<StateChangeComponent> stateChangeComponent;
  if(stateChangeComponentPtr == nullptr){
    stateChangeComponent = std::make_unique<StateChangeComponent>();
    stateChangeComponentPtr = stateChangeComponent.get();
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
      *stateChangeComponentPtr,
      *asyncMessageComponentPtr,
      *delegationComponentPtr);

  components->_behaviorSysMgrPtr            = std::move(behaviorSysMgr);
  components->_behaviorExternalInterfacePtr = std::move(behaviorExternalInterface);
  components->_behaviorContainerPtr         = std::move(behaviorContainer);
  components->_stateChangeComponentPtr      = std::move(stateChangeComponent);
  components->_asyncMessageComponentPtr     = std::move(asyncMessageComponent);
  components->_delegationComponentPtr       = std::move(delegationComponent);
  
  return components;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitializeSubComponents(Robot& robot,
                                                IBehavior* baseRunnable,
                                                BehaviorSystemManager& behaviorSysMgr,
                                                IBehaviorMessageSubscriber& messageSubscriber,
                                                BehaviorExternalInterface& behaviorExternalInterface,
                                                AIComponent& aiComponent,
                                                BehaviorContainer& behaviorContainer,
                                                BlockWorld& blockWorld,
                                                FaceWorld& faceWorld,
                                                StateChangeComponent& stateChangeComponent,
                                                AsyncMessageGateComponent& asyncMessageComponent,
                                                DelegationComponent& delegationComponent)
{  
  delegationComponent.Init(robot, behaviorSysMgr);
  stateChangeComponent.Init(messageSubscriber);
  behaviorExternalInterface.Init(robot,
                                 aiComponent,
                                 behaviorContainer,
                                 blockWorld,
                                 faceWorld,
                                 stateChangeComponent);
  
  behaviorContainer.Init(behaviorExternalInterface,
                         !static_cast<bool>(USE_BSM));

  behaviorSysMgr.InitConfiguration(baseRunnable,
                                   behaviorExternalInterface,
                                   &asyncMessageComponent);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Init(ComponentsPtr&& components, IBehavior* baseRunnable)
{
  _components = std::move(components);
  InitHelper(baseRunnable);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::InitHelper(IBehavior* baseRunnable)
{
  _audioClient.reset(new Audio::BehaviorAudioComponent(_components->_robot.GetRobotAudioClient()));
  
  _behaviorMgr = std::make_unique<BehaviorManager>();
  
  
  // Extract base runnable from config if not passed in through init
  if(baseRunnable == nullptr){
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
    
    if(USE_BSM){
      if(!behaviorSystemConfig.empty()){
        BehaviorID baseRunnableID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behaviorSystemConfig);
        baseRunnable = _components->_behaviorContainer.FindBehaviorByID(baseRunnableID).get();
        DEV_ASSERT(baseRunnable != nullptr,
                   "BehaviorComponent.Init.InvalidBaseRunnable");
      }
    }
  }
  
  
  if( ANKI_DEV_CHEATS ) {
    // create a dev base layer to put on the bottom, and pass the desired base in so that DevBaseRunnable
    // will automatically delegate to it
    baseRunnable = new DevBaseRunnable( baseRunnable );
  }
  
  
  InitializeSubComponents(_components->_robot,
                          baseRunnable,
                          _components->_behaviorSysMgr,
                          *this,
                          _components->_behaviorExternalInterface,
                          _components->_aiComponent,
                          _components->_behaviorContainer,
                          _components->_blockWorld,
                          _components->_faceWorld,
                          _components->_stateChangeComponent,
                          _components->_asyncMessageComponent,
                          _components->_delegationComponent
  );
  
  if( ANKI_DEV_CHEATS ) {
    // Initialize dev base runnable since it's not created as part of the behavior
    // factory
    baseRunnable->Init(_components->_behaviorExternalInterface);
  }

  _components->_behaviorExternalInterface.SetOptionalInterfaces(
                    &_components->_delegationComponent,
                    &_components->_robot.GetMoodManager(),
                    _components->_robot.GetContext()->GetNeedsManager(),
                    &_components->_robot.GetProgressionUnlockComponent(),
                    &_components->_robot.GetPublicStateBroadcaster());

  _audioClient->Init(_components->_behaviorExternalInterface);

  

  // LEGACY Configure behavior manager
  {
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
    
    const Json::Value& oldActivitesConfig = (dataLoader != nullptr) ?
              dataLoader->GetLegacyCozmoActivitiesConfig() : blankActivitiesConfig;
    
    const Json::Value& reactionTriggerConfig = (dataLoader != nullptr) ?
              dataLoader->GetReactionTriggerMap() : blankActivitiesConfig;
    

    
    _behaviorMgr->InitConfiguration(_components->_behaviorExternalInterface,
                                    _components->_robot.HasExternalInterface() ? _components->_robot.GetExternalInterface() : nullptr,
                                    oldActivitesConfig);
    _behaviorMgr->InitReactionTriggerMap(_components->_behaviorExternalInterface,
                                         reactionTriggerConfig);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorComponent::Update(Robot& robot,
                               std::string& currentActivityName,
                               std::string& behaviorDebugStr)
{
  _behaviorHelperComponent->Update(_components->_behaviorExternalInterface);

  if(USE_BSM){
    _components->_behaviorSysMgr.Update(_components->_behaviorExternalInterface);
  }else{
    // https://ankiinc.atlassian.net/browse/COZMO-1242 : moving too early causes pose offset
    static int ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 = 60;
    if(ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 <=0)
    {
      ICozmoBehaviorPtr currentBehavior;

      Result res = _behaviorMgr->Update(_components->_behaviorExternalInterface);
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

