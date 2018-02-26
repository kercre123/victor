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

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "gtest/gtest.h"

#include <string>

namespace {

}

using namespace Anki::Cozmo;


namespace Anki{
namespace Cozmo{

namespace {

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
           GetFromMap<BehaviorContainer>(map, BEIComponentID::BehaviorContainer),
           GetFromMap<BehaviorEventComponent>(map, BEIComponentID::BehaviorEvent),
           GetFromMap<BehaviorTimerManager>(map, BEIComponentID::BehaviorTimerManager),
           GetFromMap<BlockWorld>(map, BEIComponentID::BlockWorld),
           GetFromMap<BodyLightComponent>(map, BEIComponentID::BodyLightComponent),
           GetFromMap<CubeAccelComponent>(map, BEIComponentID::CubeAccel),
           GetFromMap<CubeLightComponent>(map, BEIComponentID::CubeLight),
           GetFromMap<DelegationComponent>(map, BEIComponentID::Delegation),
           GetFromMap<FaceWorld>(map, BEIComponentID::FaceWorld),
           GetFromMap<MapComponent>(map, BEIComponentID::Map),
           GetFromMap<MicDirectionHistory>(map, BEIComponentID::MicDirectionHistory),
           GetFromMap<MoodManager>(map, BEIComponentID::MoodManager),
           GetFromMap<ObjectPoseConfirmer>(map, BEIComponentID::ObjectPoseConfirmer),
           GetFromMap<PetWorld>(map, BEIComponentID::PetWorld),
           GetFromMap<ProgressionUnlockComponent>(map, BEIComponentID::ProgressionUnlock),
           GetFromMap<ProxSensorComponent>(map, BEIComponentID::ProxSensor),
           GetFromMap<PublicStateBroadcaster>(map, BEIComponentID::PublicStateBroadcaster),
           GetFromMap<Audio::EngineRobotAudioClient>(map, BEIComponentID::RobotAudioClient),
           GetFromMap<BEIRobotInfo>(map, BEIComponentID::RobotInfo),
           GetFromMap<TouchSensorComponent>(map, BEIComponentID::TouchSensor),
           GetFromMap<VisionComponent>(map, BEIComponentID::Vision),
           GetFromMap<VisionScheduleMediator>(map, BEIComponentID::VisionScheduleMediator));

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
                                         std::function<void(const BehaviorComponent::CompononentPtr&)> initializeBehavior,
                                         bool shouldCallInitOnBase)
{
  BehaviorContainer* empty = nullptr;
  InitializeStandardBehaviorComponent(baseBehavior, initializeBehavior,
                                      shouldCallInitOnBase, empty);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                                                std::function<void(const BehaviorComponent::CompononentPtr&)> initializeBehavior,
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

    entity->AddDependentComponent(BCComponentID::AIComponent, 
      _robot->GetComponentPtr<AIComponent>(), false);
    auto wrapper = new BaseBehaviorWrapper(baseBehavior);
    entity->AddDependentComponent(BCComponentID::BaseBehaviorWrapper, wrapper);
    entity->AddDependentComponent(BCComponentID::BehaviorContainer, _behaviorContainer.get(), false);

    BehaviorComponent::GenerateManagedComponents(*_robot, entity);

    BehaviorComponent* bc = new BehaviorComponent();
    _robot->GetAIComponent().Init(_robot.get(), bc);
    _behaviorComponent = &_robot->GetAIComponent().GetBehaviorComponent();
    _behaviorComponent->Init(_robot.get(), std::move(entity));
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
  
  std::string empty;
  _behaviorComponent->Update(*_robot, empty, empty);
  
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
void DoBehaviorComponentTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorComponent& behaviorComponent, int num)
{
  std::string currentActivityName;
  std::string behaviorDebugStr;
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
    behaviorComponent.Update(robot, currentActivityName, behaviorDebugStr);
    IncrementBaseStationTimerTicks();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DoBehaviorInterfaceTicks(Robot& robot, ICozmoBehavior& behavior, int num)
{
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
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
    auto& delegatesMap = bsm._behaviorStack->_delegatesMap;
    delegatesMap[lastEntry].insert(&injectBehavior);    
    bsm.Delegate(lastEntry, &injectBehavior);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IncrementBaseStationTimerTicks(int numTicks)
{
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + numTicks);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InjectValidDelegateIntoBSM(TestBehaviorFramework& testFramework,
                                IBehavior* delegator,
                                IBehavior* delegated,
                                bool shouldMarkAsEnterdScope){
  auto& bsm = testFramework.GetBehaviorSystemManager();
  auto iter = bsm._behaviorStack->_delegatesMap.find(delegator);
  if(iter == bsm._behaviorStack->_delegatesMap.end()){
    // Stack must be empty - inject anyways
    DEV_ASSERT(bsm._behaviorStack->_behaviorStack.empty(),
               "TestBehaviorFramework.InjectValidDelegate.StackNotEmptyButDelegatMapMiss");
    bsm._behaviorStack->_delegatesMap[delegator].insert(delegated);
  }else{
    iter->second.insert(delegated);
  }
  
  if(shouldMarkAsEnterdScope){
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
  
}
}


