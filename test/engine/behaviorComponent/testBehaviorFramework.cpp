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
#include "engine/aiComponent/behaviorHelperComponent.h"
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
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                         std::function<void(const BehaviorComponent::ComponentsPtr&)> initializeBehavior,
                                         bool shouldCallInitOnBase)
{
  BehaviorContainer* empty = nullptr;
  InitializeStandardBehaviorComponent(baseBehavior, initializeBehavior,
                                      shouldCallInitOnBase, empty);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseBehavior,
                                         std::function<void(const BehaviorComponent::ComponentsPtr&)> initializeBehavior,
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
  
  BehaviorComponent::ComponentsPtr subComponents = BehaviorComponent::GenerateComponents(*_robot,
                                                                                         nullptr,
                                                                                         nullptr,
                                                                                         _behaviorContainer.get());
  
  // stack is unused - put an arbitrary behavior on it
  if(baseBehavior == nullptr){
    baseBehavior = subComponents->_behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(Wait)).get();
    shouldCallInitOnBase = false;
  }
  
  BehaviorComponent* bc = new BehaviorComponent();
  subComponents->_aiComponent.Init(subComponents->_robot, bc);
  _behaviorComponent = &subComponents->_aiComponent.GetBehaviorComponent();
  _behaviorComponent->Init(std::move(subComponents),
                           baseBehavior);
  
  if(shouldCallInitOnBase){
    baseBehavior->Init(_behaviorComponent->_components->_behaviorExternalInterface);
  }
  if(initializeBehavior != nullptr){
    initializeBehavior(_behaviorComponent->_components);
  }
  
  std::string empty;
  _behaviorComponent->Update(*_robot, empty, empty);
  
  // Grab components from the behaviorComponent
  _behaviorExternalInterface = &_behaviorComponent->_components->_behaviorExternalInterface;
  _behaviorSystemManager = &_behaviorComponent->_components->_behaviorSysMgr;
  _aiComponent = &_behaviorComponent->_components->_aiComponent;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DoTicks_(TestBehaviorFramework& testFramework, Robot& robot, TestBehaviorWithHelpers& behavior, int num, bool expectComplete)
{
  int updateTicks = behavior._updateCount;

  BehaviorComponent& behaviorComponent = testFramework.GetBehaviorComponent();
  
  std::string currentActivityName;
  std::string behaviorDebugStr;
  bool behaviorIsActive = true;
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
    behaviorComponent.Update(robot, currentActivityName, behaviorDebugStr);
    
    updateTicks++;
    ASSERT_EQ(behavior._updateCount, updateTicks) << "behavior not ticked as often as it should be";
    
    // If the test behavior with helpers' helper finishes it automatically cancels and won't be activated anymore 
    behaviorIsActive = (behavior._currentActivationState == IBehavior::ActivationState::Activated);
    if( expectComplete && !behaviorIsActive ) {
      break;
    }
    ASSERT_TRUE(behaviorIsActive) << "behavior should still be running i="<<i;
    IncrementBaseStationTimerTicks();
  }
  
  if( expectComplete ) {
    ASSERT_FALSE(behaviorIsActive) << "behavior was expected to complete";
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
void DoBehaviorInterfaceTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorExternalInterface& behaviorExternalInterface, int num)
{
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
    behavior.Update(behaviorExternalInterface);
    IncrementBaseStationTimerTicks();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void InjectBehaviorIntoStack(ICozmoBehavior& injectBehavior, TestBehaviorFramework& testFramework)
{
  auto& behaviorStack = testFramework.GetBehaviorComponent()._components->_behaviorSysMgr._behaviorStack->_behaviorStack;
  if(ANKI_VERIFY(!behaviorStack.empty(),"InjectBehaviorIntoStack.NoBaseBehavior","")){
    IBehavior* lastEntry = behaviorStack.back();
    auto& delegatesMap = testFramework.GetBehaviorComponent()._components->_behaviorSysMgr._behaviorStack->_delegatesMap;
    delegatesMap[lastEntry].insert(&injectBehavior);    
    testFramework.GetBehaviorComponent()._components->_behaviorSysMgr.Delegate(lastEntry, &injectBehavior);
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
    delegated->WantsToBeActivated(testFramework.GetBehaviorExternalInterface());
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
void TestSuperPoweredBehavior::InitInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnEnteredActivatableScopeInternal() {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestSuperPoweredBehavior::WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const {
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredBehavior::OnLeftActivatableScopeInternal() {
  
}


//////////
/// Setup a test behavior class that tracks data for testing
//////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) {
  auto robotInfo = behaviorExternalInterface.GetRobotInfo();
  auto robotExternalInterface = robotInfo.HasExternalInterface() ? robotInfo.GetExternalInterface() : nullptr;
  if(robotExternalInterface != nullptr) {
    SubscribeToTags({EngineToGameTag::Ping});
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)  {
  _inited = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)  {
  if(!IsActivated()){
    return;
  }
  _numUpdates++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)  {
  _stopped = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::AlwaysHandleInScope(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
  _alwaysHandleCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
  _handleWhileRunningCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileInScopeButNotActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
  _handleWhileNotRunningCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::Foo() {
  _calledVoidFunc++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::Bar(BehaviorExternalInterface& behaviorExternalInterface) {
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


//////////
/// Setup a test behavior class that uses helpers
//////////

// for debugging, controls what UpdateInternal will return
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::SetUpdateResult(UpdateResult res) {
  _updateResult = res;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::DelegateToHelperOnNextUpdate(HelperHandle handleToRun,
                                  SimpleCallbackWithRobot successCallback,
                                  SimpleCallbackWithRobot failureCallback) {
  _helperHandleToDelegate = handleToRun;
  _successCallbackToDelegate = successCallback;
  _failureCallbackToDelegate = failureCallback;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::StopHelperOnNextUpdate() {
  _stopHelper = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::SetActionToRunOnNextUpdate(IActionRunner* action) {
  _nextActionToRun = action;
  _lastDelegateIfInControlResult = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehaviorWithHelpers::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const {
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehaviorWithHelpers::CarryingObjectHandledInternally() const {return true;}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) {
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehaviorWithHelpers::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface) {
  if(!IsActivated()){
    return; 
  }

  _updateCount++;
  
  if( _stopHelper ) {
    _stopHelper = false;
    StopHelperWithoutCallback();
  }
  
  if( _helperHandleToDelegate ) {
    _lastDelegateSuccess = SmartDelegateToHelper(behaviorExternalInterface,
                                                 _helperHandleToDelegate,
                                                 _successCallbackToDelegate,
                                                 _failureCallbackToDelegate);
    _helperHandleToDelegate.reset();
  }
  
  if( _nextActionToRun ) {
    if( IsControlDelegated() ) {
      printf("TestBehaviorWithHelpers: canceling previous action to start new one\n");
      CancelDelegates();
    }
    
    printf("TestBehaviorWithHelpers: starting action\n");
    _lastDelegateIfInControlResult = DelegateIfInControl(_nextActionToRun);
    _nextActionToRun = nullptr;
  }
  
  switch(_updateResult) {
    case UpdateResult::UseBaseClass: {
      printf("TestBehaviorWithHelpers.Update UseBaseClass: IsControlDelegated:%d\n", IsControlDelegated());
      if(!IsControlDelegated()){
        CancelSelf();
        return;
      }
    }
    case UpdateResult::Running: {
      printf("TestBehaviorWithHelpers.Update Running\n");
      return;
    }
    case UpdateResult::Complete: {
      printf("TestBehaviorWithHelpers.Update Complete\n");
      if(behaviorExternalInterface.HasDelegationComponent()){
        behaviorExternalInterface.GetDelegationComponent().CancelSelf(this);
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void  TestBehaviorWithHelpers::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) {
}



////////////////////////////////////////////////////////////////////////////////
// Test Helper which runs actions and delegates to other helpers
////////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TestHelper::TestHelper(BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior, const std::string& name)
: IHelper("", behaviorExternalInterface, behavior, behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory())
, _name(name)
, _behavior(behavior)
{
  if( _name.empty() ) {
    _name = "unnamed";
  }
  _name = _name + std::to_string(_TestHelper_g_num++);
  SetName(name);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::SetActionToRunOnNextUpdate(IActionRunner* action) {
  _nextActionToRun = action;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::StartAutoAction(BehaviorExternalInterface& behaviorExternalInterface) {
  _selfActionDone = false;
  _nextActionToRun = new WaitForLambdaAction([this](Robot& t) {
    printf("%s: ShouldStopActing: %d\n", _name.c_str(), _selfActionDone);
    return _selfActionDone;
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::StopAutoAction() {
  _selfActionDone = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::OnActivatedHelper(BehaviorExternalInterface& behaviorExternalInterface) {
  _initOnStackCount++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::StopInternal(bool isActive) {
  printf("%s: StopInternal\n", _name.c_str());
  _stopCount++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestHelper::ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const {
  _shouldCancelCount++;
  printf("%s: ShouldCancel:%d\n", _name.c_str(), _cancelDelegates);
  bool ret = false;
  if( _cancelDelegates ) {
    ret = true;
    _cancelDelegates = false;
  }
  return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus TestHelper::InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface) {
  _initCount++;
  
  printf("%s: Init. Action=%p\n", _name.c_str(), _nextActionToRun);
  
  CheckActions();
  
  return IHelper::HelperStatus::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IHelper::HelperStatus TestHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  _updateCount++;
  
  printf("%s: Update. IsControlDelegated:%d, _delegateAfter:%d\n",
         _name.c_str(),
         IsControlDelegated(),
         _delegateAfterAction);
  
  CheckActions();
  
  if( !IsActing() && _delegateAfterAction ) {
    _delegateAfterAction = false;
    _subHelperRaw = new TestHelper(behaviorExternalInterface, _behavior, _name + "_child");
    _subHelperRaw->StartAutoAction(behaviorExternalInterface);
    auto newHelperHandle = std::shared_ptr<IHelper>(_subHelperRaw);
    _subHelper = newHelperHandle;
    DelegateProperties delegateProperties;
    delegateProperties.SetDelegateToSet(newHelperHandle);
    delegateProperties.SetOnSuccessFunction([this](BehaviorExternalInterface& behaviorExternalInterface) {
      if( _immediateCompleteOnSubSuccess ) {
        return IHelper::HelperStatus::Complete;
      }
      else {
        _updateResult = IHelper::HelperStatus::Complete;
        return IHelper::HelperStatus::Running;
      }
    });
    
    DelegateAfterUpdate(delegateProperties);
    return IHelper::HelperStatus::Running;
  }
  
  return _updateResult;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::CheckActions() {
  if( _nextActionToRun ) {
    DelegateIfInControl(_nextActionToRun, [this](ActionResult res, BehaviorExternalInterface& behaviorExternalInterface) {
      _actionCompleteCount++;
      if( _thisSucceedsOnActionSuccess ) {
        _updateResult = IHelper::HelperStatus::Complete;
      }});
    _nextActionToRun = nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WeakHelperHandle TestHelper::GetSubHelper() {
  return _subHelper;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TestHelper* TestHelper::GetSubHelperRaw() {
  return _subHelperRaw;
}
  
}
}


