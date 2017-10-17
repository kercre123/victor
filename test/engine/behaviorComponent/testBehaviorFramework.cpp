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

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/devBaseRunnable.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/robot.h"
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
void TestBehaviorFramework::InitializeStandardBehaviorComponent(IBehavior* baseRunnable,
                                                                std::function<void(const BehaviorComponent::ComponentsPtr&)> initializeRunnable,
                                                                bool useCustomBehaviorMap,
                                                                RobotDataLoader::BehaviorIDJsonMap customMap)
{
  if(useCustomBehaviorMap){
    _behaviorContainer = std::make_unique<BehaviorContainer>(customMap);
  }else{
    _behaviorContainer = std::make_unique<BehaviorContainer>(
                             cozmoContext->GetDataLoader()->GetBehaviorJsons());
  }
  
  BehaviorComponent::ComponentsPtr subComponents = BehaviorComponent::GenerateComponents(*_robot,
                                                                                         nullptr,
                                                                                         nullptr,
                                                                                         _behaviorContainer.get());
  
  // stack is unused - put an arbitrary behavior on it
  bool shouldCallInitOnBase = true;
  if(baseRunnable == nullptr){
    baseRunnable = subComponents->_behaviorContainer.FindBehaviorByID(BehaviorID::Wait).get();
    shouldCallInitOnBase = false;
  }
  
  BehaviorComponent* bc = new BehaviorComponent();
  subComponents->_aiComponent.Init(subComponents->_robot, bc);
  _behaviorComponent = &subComponents->_aiComponent.GetBehaviorComponent();
  _behaviorComponent->Init(std::move(subComponents),
                           baseRunnable);
  
  if(shouldCallInitOnBase){
    baseRunnable->Init(_behaviorComponent->_components->_behaviorExternalInterface);
  }
  if(initializeRunnable != nullptr){
    initializeRunnable(_behaviorComponent->_components);
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


void InjectBehaviorIntoStack(ICozmoBehavior& injectBehavior, TestBehaviorFramework& testFramework)
{
  auto& backOfRunnableStack = testFramework.GetBehaviorComponent()._components->_behaviorSysMgr._runnableStack->_runnableStack.back();
  DevBaseRunnable* cozmoBehavior = dynamic_cast<DevBaseRunnable*>(backOfRunnableStack);
  if(ANKI_VERIFY(cozmoBehavior != nullptr, "InjectBehaviorIntoStack.NullptrCast", "")){
    cozmoBehavior->_possibleDelegates.insert(&injectBehavior);
    testFramework.GetBehaviorComponent()._components->_behaviorSysMgr.Delegate(cozmoBehavior, &injectBehavior);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IncrementBaseStationTimerTicks(int numTicks)
{
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + numTicks);
}

  
//////////
/// Setup a test runnable class that tracks data for testing
//////////
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::GetAllDelegates(std::set<IBehavior*>& delegates) const {
  for(auto& entry: _bc->_idToBehaviorMap){
    delegates.insert(entry.second.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::InitInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::OnEnteredActivatableScopeInternal() {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestSuperPoweredRunnable::WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const {
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) {
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestSuperPoweredRunnable::OnLeftActivatableScopeInternal() {
  
}


//////////
/// Setup a test behavior class that tracks data for testing
//////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) {
  
  auto robotExternalInterface = _robot->HasExternalInterface() ? _robot->GetExternalInterface() : nullptr;
  if(robotExternalInterface != nullptr) {
    SubscribeToTags({EngineToGameTag::Ping});
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result TestBehavior::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)  {
  _inited = true;
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStatus TestBehavior::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)  {
  _numUpdates++;
  return Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)  {
  _stopped = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
  _alwaysHandleCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
  _handleWhileRunningCalls++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestBehavior::HandleWhileNotRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)  {
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
float TestBehavior::EvaluateRunningScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const  {
  return kRunningScore;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float TestBehavior::EvaluateScoreInternal(BehaviorExternalInterface& behaviorExternalInterface) const {
  return kNotRunningScore;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });
  
  return DelegateIfInControl(action);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlExternalCallback1(Robot& robot,
                                                            bool& actionCompleteRef,
                                                            ICozmoBehavior::RobotCompletedActionCallback callback)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });
  
  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlExternalCallback2(Robot& robot,
                                                            bool& actionCompleteRef,
                                                            ICozmoBehavior::ActionResultCallback callback)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });
  
  return DelegateIfInControl(action, callback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlInternalCallbackVoid(Robot& robot,
                                                               bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });
  
  return DelegateIfInControl(action, &TestBehavior::Foo);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TestBehavior::CallDelegateIfInControlInternalCallbackRobot(Robot& robot,
                                                                bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
  new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });
  
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
Result TestBehaviorWithHelpers::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) {
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status TestBehaviorWithHelpers::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) {
  
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
      return ICozmoBehavior::UpdateInternal_WhileRunning(behaviorExternalInterface);
    }
    case UpdateResult::Running: {
      printf("TestBehaviorWithHelpers.Update Running\n");
      return Status::Running;
    }
    case UpdateResult::Complete: {
      printf("TestBehaviorWithHelpers.Update Complete\n");
      if(behaviorExternalInterface.HasDelegationComponent()){
        behaviorExternalInterface.GetDelegationComponent().CancelSelf(this);
      }
      return Status::Complete;
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
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  _selfActionDone = false;
  _nextActionToRun = new WaitForLambdaAction(robot, [this](Robot& t) {
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
ICozmoBehavior::Status TestHelper::Init(BehaviorExternalInterface& behaviorExternalInterface) {
  _initCount++;
  
  printf("%s: Init. Action=%p\n", _name.c_str(), _nextActionToRun);
  
  CheckActions();
  
  return ICozmoBehavior::Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehavior::Status TestHelper::UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) {
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
        return ICozmoBehavior::Status::Complete;
      }
      else {
        _updateResult = ICozmoBehavior::Status::Complete;
        return ICozmoBehavior::Status::Running;
      }
    });
    
    DelegateAfterUpdate(delegateProperties);
    return ICozmoBehavior::Status::Running;
  }
  
  return _updateResult;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestHelper::CheckActions() {
  if( _nextActionToRun ) {
    DelegateIfInControl(_nextActionToRun, [this](ActionResult res, BehaviorExternalInterface& behaviorExternalInterface) {
      _actionCompleteCount++;
      if( _thisSucceedsOnActionSuccess ) {
        _updateResult = ICozmoBehavior::Status::Complete;
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


