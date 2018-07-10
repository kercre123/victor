/**
* File: testBehaviorFramework
*
* Author: Kevin M. Karol
* Created: 10/02/17
*
* Description: Framework that provides helper classes and functions for
* testing behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

// Access internals for tests
#define private public
#define protected public

#include "test/engine/behaviorComponent/testBehaviorFramework.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorStack.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/unitTestKey.h"
#include "gtest/gtest.h"

#include <string>

namespace {

}

using namespace Anki::Cozmo;


namespace Anki{
namespace Cozmo{

namespace {

const std::string kNamedBehaviorStackFileName = "test/aiTests/namedBehaviorStacks.json";
const char* kStackIsForTestOnlyKey = "stackIsForTestOnly";
const char* kStackKey = "stack";

template<typename T>
T* GetFromMap(const BEIComponentMap& map, const BEIComponentID componentID) {
  auto it = map.find(componentID);
  if( it != map.end() ) {
    return static_cast<T*>(it->second);
  }
  return nullptr;
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InitBEIPartial( const BEIComponentMap& map, BehaviorExternalInterface& bei )
{
  bei.Init(GetFromMap<AIComponent>(map, BEIComponentID::AIComponent),
           GetFromMap<AnimationComponent>(map, BEIComponentID::Animation),
           GetFromMap<BeatDetectorComponent>(map, BEIComponentID::BeatDetector),
           GetFromMap<BehaviorContainer>(map, BEIComponentID::BehaviorContainer),
           GetFromMap<BehaviorEventComponent>(map, BEIComponentID::BehaviorEvent),
           GetFromMap<BehaviorTimerManager>(map, BEIComponentID::BehaviorTimerManager),
           GetFromMap<BlockWorld>(map, BEIComponentID::BlockWorld),
           GetFromMap<BackpackLightComponent>(map, BEIComponentID::BackpackLightComponent),
           GetFromMap<CubeAccelComponent>(map, BEIComponentID::CubeAccel),
           GetFromMap<CubeCommsComponent>(map, BEIComponentID::CubeComms),
           GetFromMap<CubeConnectionCoordinator>(map, BEIComponentID::CubeConnectionCoordinator),
           GetFromMap<CubeLightComponent>(map, BEIComponentID::CubeLight),
           GetFromMap<CliffSensorComponent>(map, BEIComponentID::CliffSensor),
           GetFromMap<DelegationComponent>(map, BEIComponentID::Delegation),
           GetFromMap<FaceWorld>(map, BEIComponentID::FaceWorld),
           GetFromMap<HabitatDetectorComponent>(map, BEIComponentID::HabitatDetector),
           GetFromMap<MapComponent>(map, BEIComponentID::Map),
           GetFromMap<MicComponent>(map, BEIComponentID::MicComponent),
           GetFromMap<MoodManager>(map, BEIComponentID::MoodManager),
           GetFromMap<MovementComponent>(map, BEIComponentID::MovementComponent),
           GetFromMap<ObjectPoseConfirmer>(map, BEIComponentID::ObjectPoseConfirmer),
           GetFromMap<PetWorld>(map, BEIComponentID::PetWorld),
           GetFromMap<PhotographyManager>(map, BEIComponentID::PhotographyManager),
           GetFromMap<ProgressionUnlockComponent>(map, BEIComponentID::ProgressionUnlock),
           GetFromMap<ProxSensorComponent>(map, BEIComponentID::ProxSensor),
           GetFromMap<PublicStateBroadcaster>(map, BEIComponentID::PublicStateBroadcaster),
           GetFromMap<SDKComponent>(map, BEIComponentID::SDK),
           GetFromMap<Audio::EngineRobotAudioClient>(map, BEIComponentID::RobotAudioClient),
           GetFromMap<BEIRobotInfo>(map, BEIComponentID::RobotInfo),
           GetFromMap<DataAccessorComponent>(map, BEIComponentID::DataAccessor),
           GetFromMap<TextToSpeechCoordinator>(map, BEIComponentID::TextToSpeechCoordinator),
           GetFromMap<TouchSensorComponent>(map, BEIComponentID::TouchSensor),
           GetFromMap<VariableSnapshotComponent>(map, BEIComponentID::VariableSnapshotComponent),
           GetFromMap<VisionComponent>(map, BEIComponentID::Vision),
           GetFromMap<VisionScheduleMediator>(map, BEIComponentID::VisionScheduleMediator),
           GetFromMap<SettingsManager>(map, BEIComponentID::SettingsManager));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TestBehaviorFramework::TestBehaviorFramework(int robotID,
                                             Anki::Cozmo::CozmoContext* context)
: _behaviorExternalInterface(nullptr)
, _behaviorSystemManager(nullptr)
{
  if(context == nullptr){
    context = cozmoContext;
  }
  _robot = std::make_unique<Robot>(robotID, context);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TestBehaviorFramework::~TestBehaviorFramework()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                         std::function<void(const BehaviorComponent::ComponentPtr&)> initializeBehavior,
                                         bool shouldCallInitOnBase)
{
  BehaviorContainer* empty = nullptr;
  InitializeStandardBehaviorComponent(baseBehavior, initializeBehavior,
                                      shouldCallInitOnBase, empty);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                                                std::function<void(const BehaviorComponent::ComponentPtr&)> initializeBehavior,
                                                                bool shouldCallInitOnBase,
                                                                BehaviorContainer*& customContainer)
{
  if(customContainer != nullptr){
    _behaviorContainer.reset(customContainer);
    customContainer = nullptr;
  }else{
    _behaviorContainer = std::make_unique<BehaviorContainer>(
                             cozmoContext->GetDataLoader()->GetBehaviorJsons());
  }

  // stack is unused - put an arbitrary behavior on it
  if(baseBehavior == nullptr){
    baseBehavior = _behaviorContainer->FindBehaviorByID(BEHAVIOR_ID(Wait)).get();
    DEV_ASSERT(baseBehavior != nullptr,
               "TestBehaviorFramework.InitializeStandardBehaviorComponent.NoDefaultBehaviorInContainer");
    shouldCallInitOnBase = false;
  }

  // swap out the robot's ai component for a custom one
  {
    {
      auto aiComp = new AIComponent();
      _robot->DevReplaceAIComponent(aiComp, true);
    }

    auto entity = std::make_unique<BehaviorComponent::EntityType>();

    entity->AddDependentComponent(BCComponentID::AIComponent, _robot->GetComponentPtr<AIComponent>(), false);
    auto bootLoader = new BehaviorsBootLoader(baseBehavior, {});
    entity->AddDependentComponent(BCComponentID::BehaviorsBootLoader, bootLoader);
    entity->AddDependentComponent(BCComponentID::BehaviorContainer, _behaviorContainer.get(), false);

    BehaviorComponent::GenerateManagedComponents(*_robot, entity);

    DependencyManagedEntity<RobotComponentID> dependencies;
    dependencies.AddDependentComponent(RobotComponentID::MicComponent, _robot->GetComponentPtr<MicComponent>(), false);
    dependencies.AddDependentComponent(RobotComponentID::Vision, _robot->GetComponentPtr<VisionComponent>(), false);
    dependencies.AddDependentComponent(RobotComponentID::FaceWorld, _robot->GetComponentPtr<FaceWorld>(), false);
    _robot->GetAIComponent().InitDependent(_robot.get(), dependencies);
    _behaviorComponent = _robot->GetAIComponent().GetComponentPtr<BehaviorComponent>();

    _behaviorComponent->SetComponents(std::move(entity));
    DependencyManagedEntity<AIComponentID> dependentComps;
    _behaviorComponent->InitDependent(_robot.get(), dependentComps);
  }

  if(shouldCallInitOnBase){
    auto& bei = _behaviorComponent->GetComponent<BehaviorExternalInterface>();
    baseBehavior->Init(bei);

    ICozmoBehavior* cozmoBehavior = dynamic_cast<ICozmoBehavior*>(baseBehavior);
    if( cozmoBehavior ){
      cozmoBehavior->InitBehaviorOperationModifiers();
    }
  }
  if(initializeBehavior != nullptr){
    initializeBehavior(_behaviorComponent->_comps);
  }

  DependencyManagedEntity<AIComponentID> dependentComps;
  _behaviorComponent->UpdateDependent(dependentComps);

  // Grab components from the behaviorComponent
  {
    auto& bei = _behaviorComponent->GetComponent<BehaviorExternalInterface>();
    _behaviorExternalInterface = &bei;
  }
  {
    auto& bsm = _behaviorComponent->GetComponent<BehaviorSystemManager>();
    _behaviorSystemManager = &bsm;
  }
  {
    auto& aiComp = _behaviorComponent->GetComponent<AIComponent>();
    _aiComponent = &aiComp;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::SetDefaultBaseBehavior()
{
  std::vector<IBehavior*> newStack;
  // Load base behavior in from data
  {
    IBehavior* baseBehavior = nullptr;
    auto dataLoader = GetRobot().GetContext()->GetDataLoader();
    
    Json::Value blankActivitiesConfig;

    const Json::Value& behaviorSystemConfig = (dataLoader != nullptr) ?
           dataLoader->GetVictorFreeplayBehaviorConfig() : blankActivitiesConfig;

    BehaviorID baseBehaviorID = BEHAVIOR_ID(Anonymous);
    const bool validBehavior = BehaviorIDFromString(behaviorSystemConfig["normalBaseBehavior"].asString(), baseBehaviorID);
    DEV_ASSERT(validBehavior, "Base behavior not found");
    
    auto& bc = GetBehaviorContainer();
    baseBehavior = bc.FindBehaviorByID(baseBehaviorID).get();
    DEV_ASSERT(baseBehavior != nullptr,
               "BehaviorComponent.Init.InvalidbaseBehavior");
    newStack.push_back(baseBehavior);
  }
  
  ReplaceBehaviorStack(newStack);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<IBehavior*> TestBehaviorFramework::GetCurrentBehaviorStack()
{
  return GetBehaviorSystemManager()._behaviorStack->_behaviorStack;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::ReplaceBehaviorStack(std::vector<IBehavior*> newStack)
{
  BehaviorSystemManager& bsm = GetBehaviorSystemManager();
  bsm._behaviorStack->ClearStack();

  if(newStack.size() > 0){
    auto baseBehaviorIter = newStack.begin();
    bsm._behaviorStack->InitBehaviorStack(*baseBehaviorIter, 
      GetBehaviorExternalInterface().GetRobotInfo().GetExternalInterface());
    newStack.erase(baseBehaviorIter);
  }
  
  // Add all delegates to the stack one by one
  for(auto& delegate: newStack){
    AddDelegateToStack(delegate);
    auto& delegationComponent = GetBehaviorExternalInterface().GetDelegationComponent();
    delegationComponent.CancelDelegates(delegate);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::AddDelegateToStack(IBehavior* delegate)
{
  auto& bsm = GetBehaviorSystemManager();
  IBehavior* topOfStack = bsm._behaviorStack->GetTopOfStack();

  // cancel all delegates (including actions) because the behaviors OnActivated may have delegated to
  // something
  auto& delegationComponent = GetBehaviorExternalInterface().GetDelegationComponent();
  delegationComponent.CancelDelegates(topOfStack);

  // Apply special cases to system
  ApplyAdditionalRequirementsBeforeDelegation(delegate);

  delegate->WantsToBeActivated();
  bsm.Delegate(topOfStack, delegate);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::ApplyAdditionalRequirementsBeforeDelegation(IBehavior* delegate)
{
  auto& bsm = GetBehaviorSystemManager();
  IBehavior* topOfStack = bsm._behaviorStack->GetTopOfStack();

  // Special case logic for weather
  {
    ICozmoBehavior* delCozPtr = dynamic_cast<ICozmoBehavior*>(delegate);
    const bool delegatingToWeather = (delCozPtr != nullptr) &&
                                     (delCozPtr->GetID() == BehaviorID::WeatherResponses);

    ICozmoBehavior* topWeatherPtr = dynamic_cast<ICozmoBehavior*>(topOfStack);
    const bool delegatingFromWeather = (topWeatherPtr != nullptr) &&
                                         (topWeatherPtr->GetID() == BehaviorID::WeatherResponses);
    if(delegatingToWeather || delegatingFromWeather){
      auto& uic = GetBehaviorExternalInterface().GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

      UserIntent_WeatherResponse weatherResponse;
      UserIntent intent = UserIntent::Createweather_response(std::move(weatherResponse));
      uic.SetUserIntentPending(std::move(intent), UserIntentSource::Unknown);
    }
  }
  
  // meet victor
  {
    ICozmoBehavior* delCozPtr = dynamic_cast<ICozmoBehavior*>(delegate);
    const bool delegatingToEnrollFace = (delCozPtr != nullptr) &&
                                        (delCozPtr->GetClass() == BEHAVIOR_CLASS(EnrollFace));
    
    if( delegatingToEnrollFace ) {
      auto& uic =
        GetBehaviorExternalInterface().GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
      
      UserIntent_MeetVictor meetVictor{"Cozmo"};
      UserIntent intent = UserIntent::Createmeet_victor(std::move(meetVictor));
      uic.SetUserIntentPending(std::move(intent), UserIntentSource::Unknown);
    }
  }

  // message record
  {
    ICozmoBehavior* delCozPtr = dynamic_cast<ICozmoBehavior*>(delegate);
    const bool delegatingToBehavior = (delCozPtr != nullptr)
                                   && (delCozPtr->GetClass() == BEHAVIOR_CLASS(LeaveAMessage));

    if( delegatingToBehavior ) {
      auto& uic =
      GetBehaviorExternalInterface().GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

      UserIntent_RecordMessage intentData{"Bentley"};
      UserIntent intent = UserIntent::Createmessage_record(std::move(intentData));
      uic.SetUserIntentPending(std::move(intent), UserIntentSource::Unknown);
    }
  }

  // message playback
  {
    ICozmoBehavior* delCozPtr = dynamic_cast<ICozmoBehavior*>(delegate);
    const bool delegatingToBehavior = (delCozPtr != nullptr)
                                   && (delCozPtr->GetClass() == BEHAVIOR_CLASS(PlaybackMessage));

    if( delegatingToBehavior ) {
      auto& uic =
      GetBehaviorExternalInterface().GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();

      UserIntent_PlaybackMessage intentData{"Bentley"};
      UserIntent intent = UserIntent::Createmessage_playback(std::move(intentData));
      uic.SetUserIntentPending(std::move(intent), UserIntentSource::Unknown);
    }
  }

  // Cube connection required
  {
    ICozmoBehavior* delCozPtr = dynamic_cast<ICozmoBehavior*>(delegate);
    const bool delegatingToBehavior = (delCozPtr != nullptr)
                                   && (delCozPtr->GetClass() == BEHAVIOR_CLASS(VectorPlaysCubeSpinner));
    if( delegatingToBehavior ) {
      auto& blockWorld = GetBehaviorExternalInterface().GetBlockWorld();
      blockWorld.AddConnectedActiveObject(0, "AA:AA:AA:AA:AA:AA", ObjectType::Block_LIGHTCUBE1);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<IBehavior*> TestBehaviorFramework::GetNamedBehaviorStack(const std::string& stackName)
{
  if(_namedBehaviorStacks.empty()){
    LoadNamedBehaviorStacks();
  }

  ASSERT_NAMED(_namedBehaviorStacks.find(stackName) != _namedBehaviorStacks.end(),
               ("No namedBehaviorStack exists with name: " + stackName).c_str());
  return _namedBehaviorStacks[stackName];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::SetBehaviorStackByName(const std::string& stackName)
{
  if(_namedBehaviorStacks.empty()){
    LoadNamedBehaviorStacks();
  }

  ASSERT_NAMED(_namedBehaviorStacks.find(stackName) != _namedBehaviorStacks.end(),
               ("No namedBehaviorStack exists with name: " + stackName).c_str());
  ReplaceBehaviorStack(_namedBehaviorStacks[stackName]);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::FullTreeWalk(std::map<IBehavior*,std::set<IBehavior*>>& delegateMap,
                                         std::function<void(void)> evaluateTreeCallback)
{
  FullTreeWalk(delegateMap, [&](bool leaf){ evaluateTreeCallback(); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::FullTreeWalk(std::map<IBehavior*,std::set<IBehavior*>>& delegateMap,
                                         std::function<void(bool)> evaluateTreeCallback)
{
  if(delegateMap.empty()){
    return;
  }else{
    auto& bsm = GetBehaviorSystemManager();
    IBehavior* topOfStack = bsm._behaviorStack->GetTopOfStack();
    auto iter = delegateMap.find(topOfStack);
    if(iter != delegateMap.end()){
      for(auto& delegate: iter->second){
        AddDelegateToStack(delegate);
        // Cancel any behaviors delegated to on activation
        // we'll push them on manually later
        bsm.CancelDelegates(delegate);

        // if we don't have the new delegates in the map yet then add them now
        auto nextDelegates = delegateMap.find(delegate);
        if( nextDelegates == delegateMap.end() ) {
          std::set<IBehavior*> tmpDelegates;
          delegate->GetAllDelegates(tmpDelegates);
          nextDelegates = delegateMap.insert(std::make_pair(delegate, std::move(tmpDelegates))).first;
        }

        ASSERT_NAMED(nextDelegates != delegateMap.end(), "TestBehaviorFramework.TreeWalk.InvalidDelegates");
        
        if(evaluateTreeCallback != nullptr){
          const bool isLeaf = nextDelegates->second.empty();
          evaluateTreeCallback(isLeaf);
        }
        
        FullTreeWalk(delegateMap, evaluateTreeCallback);
      }
      delegateMap.erase(iter);
      bsm.CancelSelf(topOfStack);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehaviorFramework::CanStackOccurDuringFreeplay(const std::vector<IBehavior*>& stackToBuild)
{
  // stack comparison helper
  auto compareStacks = [](const std::vector<IBehavior*>& stackOne,
                          const std::vector<IBehavior*>& stackTwo){
    // check sizes match
    if(stackOne.size() != stackTwo.size()){
      return false;
    }

    // check all behavior IDs match
    auto iterTwo = stackTwo.begin();
    for(auto& behaviorOne : stackOne){
      auto castBehaviorOne = dynamic_cast<ICozmoBehavior*>(behaviorOne);
      auto castBehaviorTwo = dynamic_cast<ICozmoBehavior*>(*iterTwo);
      if(castBehaviorOne->_id != castBehaviorTwo->_id){
        return false;
      }
      iterTwo++;
    }

    return true;
  };

  // Get the base behavior for default stack
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  tbf.SetDefaultBaseBehavior();
  auto currentStack = tbf.GetCurrentBehaviorStack();  
  DEV_ASSERT(1 == currentStack.size(), "CanStackOccurDuringFreeplay.SizeMismatch");
  IBehavior* base = currentStack.front();

  // Get ready for a full tree walk to compare stacks 
  std::map<IBehavior*,std::set<IBehavior*>> delegateMap;
  std::set<IBehavior*> tmpDelegates;
  base->GetAllDelegates(tmpDelegates);
  delegateMap.insert(std::make_pair(base, tmpDelegates));

  bool sawMatch = false;
  // tree walk callback
  auto evaluateTree = [&sawMatch, &stackToBuild, &tbf, &compareStacks](){
    auto currentStack = tbf.GetCurrentBehaviorStack();
    sawMatch |= compareStacks(stackToBuild, currentStack);
  };

  tbf.FullTreeWalk(delegateMap, evaluateTree);
  return sawMatch;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DoBehaviorComponentTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorComponent& behaviorComponent, int num)
{
  DependencyManagedEntity<RobotComponentID> robotComps;
  DependencyManagedEntity<AIComponentID> aiComps;
  for(int i=0; i<num; i++) {
    robot.GetActionList().UpdateDependent(robotComps);
    behaviorComponent.UpdateDependent(aiComps);
    IncrementBaseStationTimerTicks();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DoBehaviorInterfaceTicks(Robot& robot, ICozmoBehavior& behavior, int num)
{
  DependencyManagedEntity<RobotComponentID> dependentComps;
  for(int i=0; i<num; i++) {
    robot.GetActionList().UpdateDependent(dependentComps);
    behavior.Update();
    IncrementBaseStationTimerTicks();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InjectBehaviorIntoStack(ICozmoBehavior& injectBehavior, TestBehaviorFramework& testFramework)
{
  auto& bc = testFramework.GetBehaviorComponent();
  auto& bsm = bc.GetComponent<BehaviorSystemManager>();

  auto& behaviorStack = bsm._behaviorStack->_behaviorStack;
  if(ANKI_VERIFY(!behaviorStack.empty(),"InjectBehaviorIntoStack.NoBaseBehavior","")){
    IBehavior* lastEntry = behaviorStack.back();
    auto& delegatesMap = bsm._behaviorStack->_stackMetadataMap;
    delegatesMap.find(lastEntry)->second.delegates.insert(&injectBehavior);
    bsm.Delegate(lastEntry, &injectBehavior);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IncrementBaseStationTimerTicks()
{
  BaseStationTime_t t = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds() + 1e6f * ROBOT_TIME_STEP_MS;
  BaseStationTimer::getInstance()->UpdateTime( t );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InjectValidDelegateIntoBSM(TestBehaviorFramework& testFramework,
                                IBehavior* delegator,
                                IBehavior* delegated,
                                bool shouldMarkAsEnteredScope){
  auto& bsm = testFramework.GetBehaviorSystemManager();
  auto iter = bsm._behaviorStack->_stackMetadataMap.find(delegator);
  if(iter == bsm._behaviorStack->_stackMetadataMap.end()){
    // Stack must be empty - inject anyways
    DEV_ASSERT(bsm._behaviorStack->_behaviorStack.empty(),
               "TestBehaviorFramework.InjectValidDelegate.StackNotEmptyButDelegatMapMiss");
    bsm._behaviorStack->_stackMetadataMap[delegator] = BehaviorStack::StackMetadataEntry(delegator, static_cast<int>(bsm._behaviorStack->_behaviorStack.size()));
    bsm._behaviorStack->_stackMetadataMap[delegator].delegates.insert(delegated);
  }else{
    iter->second.delegates.insert(delegated);
  }

  if (shouldMarkAsEnteredScope) {
    delegated->OnEnteredActivatableScope();
    delegated->WantsToBeActivated();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InjectAndDelegate(TestBehaviorFramework& testFramework,
                       IBehavior* delegator,
                       IBehavior* delegated){
  InjectValidDelegateIntoBSM(testFramework,
                             delegator,
                             delegated);
  auto& bsm = testFramework.GetBehaviorSystemManager();
  EXPECT_TRUE(bsm.Delegate(delegator, delegated));
}


//////////
/// Setup a test behavior class that tracks data for testing
//////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::GetAllDelegates(std::set<IBehavior*>& delegates) const {
  for(auto& entry: _bc->_idToBehaviorMap){
    delegates.insert(entry.second.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::InitInternal() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnEnteredActivatableScopeInternal() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::UpdateInternal() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestSuperPoweredBehavior::WantsToBeActivatedInternal() const {
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnActivatedInternal() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnDeactivatedInternal() {

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnLeftActivatableScopeInternal() {

}


//////////
/// Setup a test behavior class that tracks data for testing
//////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::InitBehavior() {
  auto robotInfo = GetBEI().GetRobotInfo();
  auto robotExternalInterface = robotInfo.HasExternalInterface() ? robotInfo.GetExternalInterface() : nullptr;
  if(robotExternalInterface != nullptr) {
    SubscribeToTags({EngineToGameTag::Ping});
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::OnBehaviorActivated()  {
  _inited = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::BehaviorUpdate()  {
  if(!IsActivated()){
    return;
  }
  _numUpdates++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::OnBehaviorDeactivated()  {
  _stopped = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::AlwaysHandleInScope(const EngineToGameEvent& event)  {
  _alwaysHandleCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileActivated(const EngineToGameEvent& event)  {
  _handleWhileRunningCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileInScopeButNotActivated(const EngineToGameEvent& event)  {
  _handleWhileNotRunningCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::Foo() {
  _calledVoidFunc++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::Bar() {
  _calledRobotFunc++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction([&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlExternalCallback1(Robot& robot,
                                                            bool& actionCompleteRef,
                                                            ICozmoBehavior::RobotCompletedActionCallback callback)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction([&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlExternalCallback2(Robot& robot,
                                                            bool& actionCompleteRef,
                                                            ICozmoBehavior::ActionResultCallback callback)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction([&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlInternalCallbackVoid(Robot& robot,
                                                               bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction([&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, &TestBehavior::Foo);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlInternalCallbackRobot(Robot& robot,
                                                                bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction([&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, &TestBehavior::Bar);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::LoadNamedBehaviorStacks()
{
  ASSERT_NAMED( _namedBehaviorStacks.empty(), "Named Behavior Stacks should be loaded only once");

  Json::Value stackList;
  const bool readSuccess = cozmoContext->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources,
                                                                       kNamedBehaviorStackFileName,
                                                                       stackList);
  ASSERT_NAMED( readSuccess, ("TestBehaviorFramework error reading file: " + kNamedBehaviorStackFileName).c_str() );

  for (const auto& behaviorStackName : stackList.getMemberNames() ){
    std::vector<IBehavior*> behaviorStack;
    Json::Value stackInfo = stackList[behaviorStackName];
    bool stackIsForTestOnly = stackInfo[kStackIsForTestOnlyKey].asBool();
    Json::Value jsonStack = stackInfo[kStackKey];
    for(auto& behaviorName : jsonStack){
      BehaviorID behaviorId;
      ASSERT_NAMED(EnumFromString(behaviorName.asString(), behaviorId), 
                   ("no behavior exists of name: " + behaviorName.asString()).c_str());

      behaviorStack.push_back(GetBehaviorContainer().FindBehaviorByID(behaviorId).get());
    }
    const auto& it = _namedBehaviorStacks.find(behaviorStackName);
    ASSERT_NAMED( it == _namedBehaviorStacks.end(), 
                 ("Name already exists for behavior stack: " + behaviorStackName).c_str());
    ASSERT_NAMED(stackIsForTestOnly || CanStackOccurDuringFreeplay(behaviorStack),
                 ("Named Behavior Stack: " + behaviorStackName + " is not achievable during freeplay").c_str());
    _namedBehaviorStacks[behaviorStackName] = behaviorStack;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckStackSuffixMatch(const std::vector<IBehavior*>& a, const std::vector<IBehavior*>& b)
{
  auto aIter = a.rbegin();
  auto bIter = b.rbegin();

  while( aIter != a.rend() && bIter != b.rend() ) {
    if( *aIter != *bIter ) {
      return false;
    }
    aIter++;
    bIter++;
  }

  return true;
}

}
}
