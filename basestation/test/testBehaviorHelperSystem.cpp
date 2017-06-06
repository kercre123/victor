/**
 * File: testBehaviorHelperSystem.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-02-15
 *
 * Description: Tests for the behavior helper system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "gtest/gtest.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"


using namespace Anki;
using namespace Anki::Cozmo;


static constexpr BehaviorID emptyID = BehaviorID::NoneBehavior;

////////////////////////////////////////////////////////////////////////////////
// Test behavior to run actions and delegate to helpers
////////////////////////////////////////////////////////////////////////////////

class TestBehaviorWithHelpers : public IBehavior
{
public:

  TestBehaviorWithHelpers(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
    {
    }

  enum class UpdateResult {
    UseBaseClass,
    Running,
    Complete
  };

  // for debugging, controls what UpdateInternal will return
  void SetUpdateResult(UpdateResult res) {
    _updateResult = res;
  }

  void DelegateToHelperOnNextUpdate(HelperHandle handleToRun,
                                    SimpleCallbackWithRobot successCallback,
                                    SimpleCallbackWithRobot failureCallback) {
    _helperHandleToDelegate = handleToRun;
    _successCallbackToDelegate = successCallback;
    _failureCallbackToDelegate = failureCallback;
  }

  void StopHelperOnNextUpdate() {
    _stopHelper = true;
  }

  void SetActionToRunOnNextUpdate(IActionRunner* action) {
    _nextActionToRun = action;
    _lastStartActingResult = false;
  }

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override {
    return true;
  }

  virtual bool CarryingObjectHandledInternally() const override {return true;}

  virtual Result InitInternal(Robot& robot) override {
    return RESULT_OK;
  }

  virtual Status UpdateInternal(Robot& robot) override {

    _updateCount++;

    if( _stopHelper ) {
      _stopHelper = false;
      StopHelperWithoutCallback();
    }

    if( _helperHandleToDelegate ) {
      _lastDelegateSuccess = SmartDelegateToHelper(robot,
                                                   _helperHandleToDelegate,
                                                   _successCallbackToDelegate,
                                                   _failureCallbackToDelegate);
      _helperHandleToDelegate.reset();
    }

    if( _nextActionToRun ) {
      if( IsActing() ) {
        printf("TestBehaviorWithHelpers: canceling previous action to start new one\n");
        StopActing();
      }

      printf("TestBehaviorWithHelpers: starting action\n");
      _lastStartActingResult = StartActing(_nextActionToRun);
      _nextActionToRun = nullptr;
    }
    
    switch(_updateResult) {
      case UpdateResult::UseBaseClass: {
        printf("TestBehaviorWithHelpers.Update UseBaseClass: IsActing:%d\n", IsActing());
        return IBehavior::UpdateInternal(robot);
      }
      case UpdateResult::Running: {
        printf("TestBehaviorWithHelpers.Update Running\n");
        return Status::Running;
      }
      case UpdateResult::Complete: {
        printf("TestBehaviorWithHelpers.Update Complete\n");
        return Status::Complete;
      }
    }
  }

  virtual void  StopInternal(Robot& robot) override {
  }

  bool _lastDelegateSuccess = false;
  bool _lastStartActingResult = false;
  int _updateCount = 0;
  
private:
  UpdateResult _updateResult = UpdateResult::UseBaseClass;
  IActionRunner* _nextActionToRun = nullptr;
  bool _stopHelper = false;
  HelperHandle _helperHandleToDelegate;
  SimpleCallbackWithRobot _successCallbackToDelegate;
  SimpleCallbackWithRobot _failureCallbackToDelegate;
 

};

////////////////////////////////////////////////////////////////////////////////
// Test Helper which runs actions and delegates to other helpers
////////////////////////////////////////////////////////////////////////////////

static int _TestHelper_g_num = 0;

class TestHelper : public IHelper
{
public:

  TestHelper(Robot& robot, IBehavior& behavior, const std::string& name = "")
    : IHelper("", robot, behavior, robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory())
    , _name(name)
    , _behavior(behavior)
    {
      if( _name.empty() ) {
        _name = "unnamed";
      }
      _name = _name + std::to_string(_TestHelper_g_num++);
      SetName(name);
    }
  
  void SetActionToRunOnNextUpdate(IActionRunner* action) {
    _nextActionToRun = action;
  }

  void StartAutoAction(Robot& robot) {
    _selfActionDone = false;
    _nextActionToRun = new WaitForLambdaAction(robot, [this](Robot& t) {
        printf("%s: ShouldStopActing: %d\n", _name.c_str(), _selfActionDone);
        return _selfActionDone;
      });
  }

  void StopAutoAction() {
    _selfActionDone = true;
  }


  virtual void InitializeOnStackInternal() override {
    _initOnStackCount++;
  }

  virtual void StopInternal(bool isActive) override {
    printf("%s: StopInternal\n", _name.c_str());
    _stopCount++;
  }

  virtual bool ShouldCancelDelegates(const Robot& robot) const override {
    _shouldCancelCount++;
    printf("%s: ShouldCancel:%d\n", _name.c_str(), _cancelDelegates);
    bool ret = false;
    if( _cancelDelegates ) {
      ret = true;
      _cancelDelegates = false;
    }
    return ret;
  }

  virtual IBehavior::Status Init(Robot& robot) override {
    _initCount++;

    printf("%s: Init. Action=%p\n", _name.c_str(), _nextActionToRun);

    CheckActions();

    return IBehavior::Status::Running;
  }

  virtual IBehavior::Status UpdateWhileActiveInternal(Robot& robot) override {
    _updateCount++;

    printf("%s: Update. IsActing:%d, _delegateAfter:%d\n",
           _name.c_str(),
           IsActing(),
           _delegateAfterAction);

    CheckActions();
    
    if( !IsActing() && _delegateAfterAction ) {
      _delegateAfterAction = false;
      _subHelperRaw = new TestHelper(robot, _behavior, _name + "_child");
      _subHelperRaw->StartAutoAction(robot);
      auto newHelperHandle = std::shared_ptr<IHelper>(_subHelperRaw);
      _subHelper = newHelperHandle;
      DelegateProperties delegateProperties;
      delegateProperties.SetDelegateToSet(newHelperHandle);
      delegateProperties.SetOnSuccessFunction([this](Robot& robot) {
          if( _immediateCompleteOnSubSuccess ) {
            return IBehavior::Status::Complete;
          }
          else {
            _updateResult = IBehavior::Status::Complete;
            return IBehavior::Status::Running;
          }
        });
      
      DelegateAfterUpdate(delegateProperties);
      return IBehavior::Status::Running;
    }      
    
    return _updateResult;
  }

  void CheckActions() {
    if( _nextActionToRun ) {
      StartActing(_nextActionToRun, [this](ActionResult res, Robot& r) {
          _actionCompleteCount++;
          if( _thisSucceedsOnActionSuccess ) {
            _updateResult = IBehavior::Status::Complete;
          }});
      _nextActionToRun = nullptr;
    }
  }

  WeakHelperHandle GetSubHelper() {
    return _subHelper;
  }

  TestHelper* GetSubHelperRaw() {
    return _subHelperRaw;
  }

  IBehavior::Status _updateResult = IBehavior::Status::Running;
  mutable bool _cancelDelegates = false;
  bool _delegateAfterAction = false;
  bool _thisSucceedsOnActionSuccess = false;
  bool _immediateCompleteOnSubSuccess = false;
  
  int _initOnStackCount = 0;
  int _stopCount = 0;
  mutable int _shouldCancelCount = 0;
  int _initCount = 0;
  int _updateCount = 0;
  int _actionCompleteCount = 0;

  std::string _name;
  
private:
  IActionRunner* _nextActionToRun = nullptr;

  bool _selfActionDone = false;

  IBehavior& _behavior;

  WeakHelperHandle _subHelper;
  TestHelper* _subHelperRaw = nullptr;
};

#define DoTicks(r, b, n) do { SCOPED_TRACE(__LINE__); DoTicks_(r, b, n); } while(0)
#define DoTicksToComplete(r, b, n) do { SCOPED_TRACE(__LINE__); DoTicks_(r, b, n, true); } while(0)


void DoTicks_(Robot& robot, TestBehaviorWithHelpers& behavior, int num, bool expectComplete = false)
{
  int updateTicks = behavior._updateCount;

  IBehavior::Status status = IBehavior::Status::Running;
  
  for(int i=0; i<num; i++) {
    robot.GetAIComponent().Update();
    status = behavior.Update();
    robot.GetActionList().Update();
    
    updateTicks++;
    ASSERT_EQ(behavior._updateCount, updateTicks) << "behavior not ticked as often as it should be";

    if( expectComplete && status == IBehavior::Status::Complete ) {
      behavior.Stop();
      break;
    }
    ASSERT_EQ(status, IBehavior::Status::Running) << "behavior should still be running i="<<i;
  }

  if( expectComplete ) {
    ASSERT_EQ(status, IBehavior::Status::Complete) << "behavior was expected to complete";
  }
}

// Just test that our test behavior works, and can run and compelte
TEST(BehaviorHelperSystem, BehaviorComplete)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  b.Init();

  DoTicks(robot, b, 3);
  ASSERT_EQ(b._updateCount, 3);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Complete);
  DoTicksToComplete(robot, b, 5);
}


// Test that our test behavior works with actions (still no helpers)
TEST(BehaviorHelperSystem, BehaviorWithActions)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  b.Init();

  DoTicks(robot, b, 3);

  bool done1 = false;
  WaitForLambdaAction* action1 = new WaitForLambdaAction(robot, [&done1](Robot& r){
      printf("action1: %d\n", done1);
      return done1;
    });
  
  b.SetActionToRunOnNextUpdate(action1);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  DoTicks(robot, b, 1);
  ASSERT_EQ(b._lastStartActingResult, true);
  DoTicks(robot, b, 2);

  bool done2 = false;
  WaitForLambdaAction* action2 = new WaitForLambdaAction(robot, [&done2](Robot& r){
      printf("action2: %d\n", done2);
      return done2;
    });
  b.SetActionToRunOnNextUpdate(action2);
  DoTicks(robot, b, 1);
  ASSERT_EQ(b._lastStartActingResult, true);
  DoTicks(robot, b, 5);

  done1 = true;
  DoTicks(robot, b, 4);

  done2 = true;
  DoTicksToComplete(robot, b, 5);
}


// basic test which delegates out to a helper and lets it succeed or fail
TEST(BehaviorHelperSystem, SimpleDelegate)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  b.Init();

  DoTicks(robot, b, 3);

  bool helperFailed = false;
  bool helperSucceeded = false;

  std::weak_ptr<IHelper> weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(robot, b);
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](Robot& robot) {helperSucceeded = true;},
                                   [&helperFailed](Robot& robot) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());
  
  DoTicks(robot, b, 5);

  ASSERT_FALSE(weak.expired());
  ASSERT_EQ(rawPtr->_initOnStackCount, 1) << "helper not initied properly";
  ASSERT_GT(rawPtr->_updateCount, 0) << "helper should have been updated";
  ASSERT_EQ(rawPtr->_stopCount, 0);
  ASSERT_EQ(rawPtr->_shouldCancelCount, 0) << "no sub-helper, so shouldn't tick this";

  rawPtr->_updateResult = IBehavior::Status::Complete;

  DoTicks(robot, b, 3);

  // at this point, helper should be done, so pointer shouldn't be valid
  EXPECT_TRUE(weak.expired());
  rawPtr = nullptr;

  EXPECT_TRUE(helperSucceeded);
  EXPECT_FALSE(helperFailed);

  ////////////////////////////////////////////////////////////////////////////////
  // now do it again, but this time let the helper fail
  ////////////////////////////////////////////////////////////////////////////////
  
  helperSucceeded = false;
  helperFailed = false;
  
  {
    rawPtr = new TestHelper(robot, b);
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](Robot& robot) {helperSucceeded = true;},
                                   [&helperFailed](Robot& robot) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());
  
  DoTicks(robot, b, 5);

  ASSERT_FALSE(weak.expired());
  ASSERT_EQ(rawPtr->_initOnStackCount, 1) << "helper not initied properly";
  ASSERT_GT(rawPtr->_updateCount, 0) << "helper should have been updated";
  ASSERT_EQ(rawPtr->_stopCount, 0);
  ASSERT_EQ(rawPtr->_shouldCancelCount, 0) << "no sub-helper, so shouldn't tick this";

  rawPtr->_updateResult = IBehavior::Status::Failure;

  DoTicks(robot, b, 3);

  // at this point, helper should be done, so pointer shouldn't be valid
  EXPECT_TRUE(weak.expired());
  rawPtr = nullptr;

  EXPECT_FALSE(helperSucceeded);
  EXPECT_TRUE(helperFailed);

}

// delegate to a helper which immediately queues actions, so that we can rely on always having IsActing be true
TEST(BehaviorHelperSystem, DelegateWithActions)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  b.Init();

  bool helperFailed = false;
  bool helperSucceeded = false;
  bool stopAction = false;
  int actionChecks = 0;

  WeakHelperHandle weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(robot, b);
    rawPtr->SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopAction, &actionChecks](Robot& r){
          printf("action: %d\n", stopAction);
          actionChecks++;
          return stopAction;
        }));    
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](Robot& robot) {helperSucceeded = true;},
                                   [&helperFailed](Robot& robot) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());

  // shouldn't have been added to the stack yet
  EXPECT_EQ(rawPtr->_initOnStackCount, 0);
  EXPECT_EQ(rawPtr->_stopCount, 0);
  EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
  EXPECT_EQ(rawPtr->_initCount, 0);
  EXPECT_EQ(rawPtr->_updateCount, 0);
  EXPECT_EQ(rawPtr->_actionCompleteCount, 0);
  EXPECT_EQ(actionChecks, 0);
  
  DoTicks(robot, b, 5);

  ASSERT_FALSE(weak.expired());

  // at this point, helper should have taken over, helper's action should be running
  EXPECT_EQ(rawPtr->_initOnStackCount, 1);
  EXPECT_EQ(rawPtr->_stopCount, 0);
  EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
  EXPECT_EQ(rawPtr->_initCount, 1);
  EXPECT_GT(rawPtr->_updateCount, 0);
  EXPECT_EQ(rawPtr->_actionCompleteCount, 0);
  EXPECT_GT(actionChecks, 0);

  int currActionCheckCount = actionChecks;

  {
    // this should stop the action, which should end the behavior, which should stop the helper
    stopAction = true;

    // hold a helper reference so we can keep track of stuff
    ASSERT_FALSE(weak.expired());
    auto strongHelper = weak.lock();
    
    DoTicksToComplete(robot, b, 5);

    ASSERT_FALSE(weak.expired());
    EXPECT_EQ(rawPtr->_initOnStackCount, 1);
    EXPECT_EQ(rawPtr->_stopCount, 1);
    EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
    EXPECT_EQ(rawPtr->_initCount, 1);
    EXPECT_GT(rawPtr->_updateCount, 0);
    EXPECT_EQ(rawPtr->_actionCompleteCount, 1);
    EXPECT_GT(actionChecks, currActionCheckCount);

    // technically, helper didn't finish, behavior killed it because it wasn't Acting
    EXPECT_FALSE(helperSucceeded);
    EXPECT_FALSE(helperFailed);
  }
  // now the helper should be destroyed

  EXPECT_TRUE(weak.expired());
  rawPtr = nullptr;
}

// Test that the behavior correctly stops it's helper
TEST(BehaviorHelperSystem, BehaviorStopsHelper)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  b.Init();

  bool helperFailed = false;
  bool helperSucceeded = false;
  bool stopHelperAction = false;

  WeakHelperHandle weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(robot, b);
    rawPtr->SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopHelperAction](Robot& r){
          printf("helper action: %d\n", stopHelperAction);
          return stopHelperAction;
        }));    
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](Robot& robot) {helperSucceeded = true;},
                                   [&helperFailed](Robot& robot) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());

  // shouldn't have been added to the stack yet
  EXPECT_EQ(rawPtr->_initOnStackCount, 0);
  EXPECT_EQ(rawPtr->_stopCount, 0);
  EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
  EXPECT_EQ(rawPtr->_initCount, 0);
  EXPECT_EQ(rawPtr->_updateCount, 0);
  EXPECT_EQ(rawPtr->_actionCompleteCount, 0);
  
  DoTicks(robot, b, 5);

  ASSERT_FALSE(weak.expired());

  // at this point, helper should have taken over, helper's action should be running
  EXPECT_EQ(rawPtr->_initOnStackCount, 1);
  EXPECT_EQ(rawPtr->_stopCount, 0);
  EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
  EXPECT_EQ(rawPtr->_initCount, 1);
  EXPECT_GT(rawPtr->_updateCount, 0);
  EXPECT_EQ(rawPtr->_actionCompleteCount, 0);

  bool stopBehaviorAction = false;

  {
    b.StopHelperOnNextUpdate();
    b.SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopBehaviorAction](Robot& r) {
          printf("behavior action: %d\n", stopBehaviorAction);
          return stopBehaviorAction;
        }));
    

    // hold a helper reference so we can keep track of stuff
    ASSERT_FALSE(weak.expired());
    auto strongHelper = weak.lock();
    
    DoTicks(robot, b, 10);

    ASSERT_FALSE(weak.expired());
    EXPECT_EQ(rawPtr->_initOnStackCount, 1);
    EXPECT_EQ(rawPtr->_stopCount, 1);
    EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
    EXPECT_EQ(rawPtr->_initCount, 1);
    EXPECT_GT(rawPtr->_updateCount, 0);
    EXPECT_EQ(rawPtr->_actionCompleteCount, 0) << "action shouldn't have finished";

    // technically, helper didn't finish, behavior stopped it
    EXPECT_FALSE(helperSucceeded);
    EXPECT_FALSE(helperFailed);

    // this should do nothing
    stopHelperAction = true;
    DoTicks(robot, b, 5);
    ASSERT_FALSE(weak.expired());
    EXPECT_EQ(rawPtr->_actionCompleteCount, 0);
    EXPECT_FALSE(helperSucceeded);
    EXPECT_FALSE(helperFailed);
  }
  // now the helper should be destroyed

  EXPECT_TRUE(weak.expired());
  rawPtr = nullptr;

  DoTicks(robot, b, 5);
  stopBehaviorAction = true;
  DoTicksToComplete(robot, b, 5);

  EXPECT_TRUE(weak.expired());
}


// Utility functions for stacking sub-helpers

struct HelperPointers {
  TestHelper* raw = nullptr;
  HelperHandle strong;
  WeakHelperHandle weak;
};

#define AddSubHelper(r, b, p) do { SCOPED_TRACE(__LINE__); AddSubHelper_(r, b, p); } while(0)

void AddSubHelper_(Robot& robot, TestBehaviorWithHelpers& behavior, std::vector<HelperPointers>& ptrs)
{
  ASSERT_FALSE(ptrs.empty());
  ASSERT_FALSE(ptrs.back().weak.expired());

  TestHelper* curr = ptrs.back().raw;

  EXPECT_EQ(curr->_initOnStackCount, 1);
  EXPECT_EQ(curr->_stopCount, 0);
  EXPECT_EQ(curr->_shouldCancelCount, 0);
  EXPECT_EQ(curr->_initCount, 1);
  EXPECT_GT(curr->_updateCount, 0);

  curr->_delegateAfterAction = true;
  curr->StopAutoAction();

  DoTicks(robot, behavior, 2); // TODO:(bn) should try to make this work in 1 tick...
  ASSERT_FALSE(ptrs.back().weak.expired());

  EXPECT_EQ(curr->_initOnStackCount, 1);
  EXPECT_EQ(curr->_stopCount, 0);
  EXPECT_EQ(curr->_shouldCancelCount, 0) << "shouldn't call this until next tick";
  EXPECT_EQ(curr->_initCount, 1);

  auto topHandleWeak = curr->GetSubHelper();
  ASSERT_FALSE(topHandleWeak.expired());
  auto topHandleStrong = topHandleWeak.lock();
  auto topRaw = curr->GetSubHelperRaw();
  EXPECT_EQ(topRaw->_initOnStackCount, 1);
  EXPECT_EQ(topRaw->_stopCount, 0);
  EXPECT_EQ(topRaw->_shouldCancelCount, 0);
  EXPECT_EQ(topRaw->_initCount, 1);
  EXPECT_EQ(topRaw->_updateCount, 2);
  EXPECT_EQ(topRaw->_actionCompleteCount, 0);

  // alternate the value of this
  topRaw->_immediateCompleteOnSubSuccess = ptrs.size() % 2 == 1;

  // let it run for a bit and make sure things make sense
  DoTicks(robot, behavior, 8);
  ASSERT_FALSE(ptrs.back().weak.expired());
  EXPECT_EQ(curr->_initOnStackCount, 1);
  EXPECT_EQ(curr->_stopCount, 0);
  EXPECT_GT(curr->_shouldCancelCount, 0);
  EXPECT_EQ(curr->_initCount, 1);

  ASSERT_FALSE(topHandleWeak.expired());
  EXPECT_EQ(topRaw->_initOnStackCount, 1);
  EXPECT_EQ(topRaw->_stopCount, 0);
  EXPECT_EQ(topRaw->_shouldCancelCount, 0);
  EXPECT_EQ(topRaw->_initCount, 1);
  EXPECT_GT(topRaw->_updateCount, 1);

  ptrs.push_back({.raw=topRaw, .strong=topHandleStrong, .weak=topHandleWeak});
}

#define CheckPtrsRunning(p) do { SCOPED_TRACE(__LINE__); CheckPtrsRunning_(p); } while(0)

void CheckPtrsRunning_(std::vector<HelperPointers>& ptrs) {
  ASSERT_FALSE(ptrs.empty());

  // assume everything in vector is running.
  size_t end = ptrs.size();

  // update everything that's not active
  for(size_t i=0; i+1<end; ++i) {
    ASSERT_FALSE(ptrs[i].weak.expired());
    // remove the strong reference, weak should still be valid
    ptrs[i].strong.reset();
    ASSERT_FALSE(ptrs[i].weak.expired());
    // restore the strong, so we can check on it later
    ptrs[i].strong = ptrs[i].weak.lock();

    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1);
    EXPECT_EQ(ptrs[i].raw->_stopCount, 0);
    EXPECT_GT(ptrs[i].raw->_shouldCancelCount, 0);
    EXPECT_EQ(ptrs[i].raw->_initCount, 1);
    EXPECT_GT(ptrs[i].raw->_updateCount, 0);
    EXPECT_GT(ptrs[i].raw->_actionCompleteCount, 0);
  }

  // check the active one
  ASSERT_FALSE(ptrs.back().weak.expired());
  // remove the strong reference, weak should still be valid
  ptrs.back().strong.reset();
  ASSERT_FALSE(ptrs.back().weak.expired());
  // restore the strong, so we can check on it later
  ptrs.back().strong = ptrs.back().weak.lock();

  EXPECT_EQ(ptrs.back().raw->_initOnStackCount, 1);
  EXPECT_EQ(ptrs.back().raw->_stopCount, 0);
  EXPECT_EQ(ptrs.back().raw->_initCount, 1);
  EXPECT_GT(ptrs.back().raw->_updateCount, 0);
}

// Make a bunch of helpers which always StartActing right away. Add more helpers to the stack by causing the
// current top one to stop acting and then immediately delegate to another helper. Eventually, the top helper
// succeeds, causing everything to succeed
TEST(BehaviorHelperSystem, MultiLayerSuccess)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  b.Init();

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(robot, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(robot);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](Robot& robot) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](Robot& robot) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());


  DoTicks(robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(robot, b, 10);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  // now stop the action on the top helper without making another delegate
  ptrs[size-1].raw->_thisSucceedsOnActionSuccess = true;
  ptrs[size-1].raw->StopAutoAction();

  DoTicks(robot, b, 10);

  // all helpers should be done, behavior should be doing it's post-delegate action
  ASSERT_EQ(ptrs.size(), size);

  // all helpers should have succeeded and had one action completed. Top shouldn't have had ShouldCancel
  // called, other should
  for(int i=0; i<size; ++i) {
    const bool shouldHaveCancelChecks = i < size-1;

    ASSERT_FALSE(ptrs[i].weak.expired());
    
    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_stopCount, 1) << i;
    EXPECT_EQ(shouldHaveCancelChecks, ptrs[i].raw->_shouldCancelCount > 0) << i;
    EXPECT_EQ(ptrs[i].raw->_initCount, 1) << i;
    EXPECT_GT(ptrs[i].raw->_updateCount, 0) << i;
    EXPECT_EQ(ptrs[i].raw->_actionCompleteCount, 1) << i;

    // this strong helper should be the last one holding the weak ptr
    ptrs[i].strong.reset();
    ASSERT_TRUE(ptrs[i].weak.expired());
  }

  EXPECT_FALSE(baseHelperFailed);
  EXPECT_TRUE(baseHelperSucceeded);

  stopBehaviorAction = true;
  // now behavior should stop
  DoTicksToComplete(robot, b, 10);
}

// Build a stack of helpers similar to the previous test, but this time test what happens if you cancel one in
// the middle, and if you cancel all of them
TEST(BehaviorHelperSystem, CancelDelegates)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  b.Init();

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(robot, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(robot);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](Robot& robot) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](Robot& robot) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());


  DoTicks(robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(robot, b, 10);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  // have some action in the middle cancel it's delegates
  ptrs[6].raw->_cancelDelegates = true;
  ptrs[6].raw->StartAutoAction(robot);
  
  DoTicks(robot, b, 2);

  // reset should cancel count, so the top looks like it's "Fresh" (so the helper functions still work)
  ASSERT_FALSE(ptrs[6].weak.expired());
  ptrs[6].raw->_shouldCancelCount = 0;


  // these should all be unaffected
  for(int i=0; i<=6; ++i) {
    ASSERT_FALSE(ptrs[i].weak.expired());
    // remove the strong reference, weak should still be valid
    ptrs[i].strong.reset();
    ASSERT_FALSE(ptrs[i].weak.expired());
    // restore the strong, so we can check on it later
    ptrs[i].strong = ptrs[i].weak.lock();


    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1);
    EXPECT_EQ(ptrs[i].raw->_stopCount, 0);
    if(i<6) {
      EXPECT_GT(ptrs[i].raw->_shouldCancelCount, 0);
    }
    EXPECT_EQ(ptrs[i].raw->_initCount, 1);
  }

  // everything above should be canceled
  for(int i=7; i<10; ++i) {
    ASSERT_FALSE(ptrs[i].weak.expired());

    int expectedCompletedCount = i < 9 ? 1 : 0;  
    
    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_stopCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_initCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_actionCompleteCount, expectedCompletedCount)
      << i << ": should not count as action complete the second time, it was canceled";
    // this strong helper should be the last one holding the weak ptr
    ptrs[i].strong.reset();
    ASSERT_TRUE(ptrs[i].weak.expired());
  }

  // kill the bad pointers
  ptrs.pop_back();
  ptrs.pop_back();
  ptrs.pop_back();

  size -= 3;

  DoTicks(robot, b, 10);
  ASSERT_EQ(ptrs.size(), size);
  CheckPtrsRunning(ptrs);

  // add a few more helpers back
  while( size < 10 ) {
    AddSubHelper(robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(robot, b, 2);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  DoTicks(robot, b, 10);
  ASSERT_EQ(ptrs.size(), size);
  CheckPtrsRunning(ptrs);

  // ask the behavior to stop delegates (should clear all), but keep running
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  b.StopHelperOnNextUpdate();

  DoTicks(robot, b, 10);
    
  // all helpers should have stopped.
  for(int i=0; i<size; ++i) {
    ASSERT_FALSE(ptrs[i].weak.expired());
    
    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_stopCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_initCount, 1) << i;
    EXPECT_GT(ptrs[i].raw->_updateCount, 0) << i;

    // this strong helper should be the last one holding the weak ptr
    ptrs[i].strong.reset();
    ASSERT_TRUE(ptrs[i].weak.expired());
  }

  ptrs.clear();

  // didn't fail or pass, got canceled
  EXPECT_FALSE(baseHelperFailed);
  EXPECT_FALSE(baseHelperSucceeded);
}

// Build a stack of behaviors, and make sure that stopping the behavior behaviors correctly (stops all of the
// helpers)
TEST(BehaviorHelperSystem, StopBehavior)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(emptyID);
  TestBehaviorWithHelpers b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  b.Init();

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(robot, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(robot);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](Robot& robot) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction(robot, [&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](Robot& robot) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());


  DoTicks(robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(robot, b, 2);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  DoTicks(robot, b, 10);
  CheckPtrsRunning(ptrs);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Complete);

  DoTicksToComplete(robot, b, 3);

  // all helpers should have stopped.
  for(int i=0; i<size; ++i) {
    ASSERT_FALSE(ptrs[i].weak.expired());
    
    EXPECT_EQ(ptrs[i].raw->_initOnStackCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_stopCount, 1) << i;
    EXPECT_EQ(ptrs[i].raw->_initCount, 1) << i;
    EXPECT_GT(ptrs[i].raw->_updateCount, 0) << i;

    int expectedActionCount = i == size-1 ? 0 : 1;
    EXPECT_EQ(ptrs[i].raw->_actionCompleteCount, expectedActionCount) << i;

    // this strong helper should be the last one holding the weak ptr
    ptrs[i].strong.reset();
    ASSERT_TRUE(ptrs[i].weak.expired());
  }

  ptrs.clear();

  // didn't fail or pass, got canceled
  EXPECT_FALSE(baseHelperFailed);
  EXPECT_FALSE(baseHelperSucceeded);
}
