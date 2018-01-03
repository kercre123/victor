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

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"


using namespace Anki;
using namespace Anki::Cozmo;


static const BehaviorClass emptyClass = BEHAVIOR_CLASS(Wait);
static const BehaviorID emptyID = BEHAVIOR_ID(Wait);

////////////////////////////////////////////////////////////////////////////////
// Test behavior to run actions and delegate to helpers
////////////////////////////////////////////////////////////////////////////////

// Just test that our test behavior works, and can run and compelte
TEST(BehaviorHelperSystem, BehaviorComplete)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);

  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 3);
  ASSERT_EQ(b._updateCount, 4); // includes the tick on initialization

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Complete);
  DoTicksToComplete(testBehaviorFramework, robot, b, 5);
}


// Test that our test behavior works with actions (still no helpers)
TEST(BehaviorHelperSystem, BehaviorWithActions)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  IncrementBaseStationTimerTicks();
  Robot& robot = testBehaviorFramework.GetRobot();
  DoTicks(testBehaviorFramework, robot, b, 3);

  bool done1 = false;
  WaitForLambdaAction* action1 = new WaitForLambdaAction([&done1](Robot& r){
      printf("action1: %d\n", done1);
      return done1;
    });
  
  b.SetActionToRunOnNextUpdate(action1);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);
  DoTicks(testBehaviorFramework, robot, b, 1);
  ASSERT_EQ(b._lastDelegateIfInControlResult, true);
  DoTicks(testBehaviorFramework, robot, b, 2);

  bool done2 = false;
  WaitForLambdaAction* action2 = new WaitForLambdaAction([&done2](Robot& r){
      printf("action2: %d\n", done2);
      return done2;
    });
  b.SetActionToRunOnNextUpdate(action2);
  DoTicks(testBehaviorFramework, robot, b, 1);
  ASSERT_EQ(b._lastDelegateIfInControlResult, true);
  DoTicks(testBehaviorFramework, robot, b, 5);

  done1 = true;
  DoTicks(testBehaviorFramework, robot, b, 4);

  done2 = true;
  DoTicksToComplete(testBehaviorFramework, robot, b, 5);
}


// basic test which delegates out to a helper and lets it succeed or fail
TEST(BehaviorHelperSystem, SimpleDelegate)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }

  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 3);

  bool helperFailed = false;
  bool helperSucceeded = false;

  std::weak_ptr<IHelper> weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(behaviorExternalInterface, b);
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](BehaviorExternalInterface& behaviorExternalInterface) {helperSucceeded = true;},
                                   [&helperFailed](BehaviorExternalInterface& behaviorExternalInterface) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());
  
  DoTicks(testBehaviorFramework, robot, b, 5);

  ASSERT_FALSE(weak.expired());
  ASSERT_EQ(rawPtr->_initOnStackCount, 1) << "helper not initied properly";
  ASSERT_GT(rawPtr->_updateCount, 0) << "helper should have been updated";
  ASSERT_EQ(rawPtr->_stopCount, 0);
  ASSERT_EQ(rawPtr->_shouldCancelCount, 0) << "no sub-helper, so shouldn't tick this";

  rawPtr->_updateResult = IHelper::HelperStatus::Complete;

  DoTicks(testBehaviorFramework, robot, b, 3);

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
    rawPtr = new TestHelper(behaviorExternalInterface, b);
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](BehaviorExternalInterface& behaviorExternalInterface) {helperSucceeded = true;},
                                   [&helperFailed](BehaviorExternalInterface& behaviorExternalInterface) {helperFailed = true;});
  }

  ASSERT_TRUE(rawPtr != nullptr);
  ASSERT_FALSE(weak.expired());
  
  DoTicks(testBehaviorFramework, robot, b, 5);

  ASSERT_FALSE(weak.expired());
  ASSERT_EQ(rawPtr->_initOnStackCount, 1) << "helper not initied properly";
  ASSERT_GT(rawPtr->_updateCount, 0) << "helper should have been updated";
  ASSERT_EQ(rawPtr->_stopCount, 0);
  ASSERT_EQ(rawPtr->_shouldCancelCount, 0) << "no sub-helper, so shouldn't tick this";

  rawPtr->_updateResult = IHelper::HelperStatus::Failure;

  DoTicks(testBehaviorFramework, robot, b, 3);

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
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);

  bool helperFailed = false;
  bool helperSucceeded = false;
  bool stopAction = false;
  int actionChecks = 0;

  WeakHelperHandle weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(behaviorExternalInterface, b);
    rawPtr->_thisSucceedsOnActionSuccess = true;
    rawPtr->SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopAction, &actionChecks](Robot& r){
          printf("action: %d\n", stopAction);
          actionChecks++;
          return stopAction;
        }));    
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](BehaviorExternalInterface& behaviorExternalInterface) {helperSucceeded = true;},
                                   [&helperFailed](BehaviorExternalInterface& behaviorExternalInterface) {helperFailed = true;});
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
  
  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 5);

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
    HelperHandle strongHandle = weak.lock();
    
    DoTicks(testBehaviorFramework, robot, b, 5);
    // Ensure that the handle was cleared within the behavior system
    // and only this strong reference remains
    ASSERT_EQ(strongHandle.use_count(), 1);

    
    EXPECT_EQ(rawPtr->_initOnStackCount, 1);
    EXPECT_EQ(rawPtr->_stopCount, 1);
    EXPECT_EQ(rawPtr->_shouldCancelCount, 0);
    EXPECT_EQ(rawPtr->_initCount, 1);
    EXPECT_GT(rawPtr->_updateCount, 0);
    EXPECT_EQ(rawPtr->_actionCompleteCount, 1);
    EXPECT_GT(actionChecks, currActionCheckCount);

    EXPECT_TRUE(helperSucceeded);
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
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);

  bool helperFailed = false;
  bool helperSucceeded = false;
  bool stopHelperAction = false;

  WeakHelperHandle weak;
  TestHelper* rawPtr = nullptr;

  {
    rawPtr = new TestHelper(behaviorExternalInterface, b);
    rawPtr->SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopHelperAction](Robot& r){
          printf("helper action: %d\n", stopHelperAction);
          return stopHelperAction;
        }));    
    auto testHelper = std::shared_ptr<IHelper>( rawPtr );
    weak = testHelper;
    ASSERT_FALSE(weak.expired());

    b.DelegateToHelperOnNextUpdate(testHelper,
                                   [&helperSucceeded](BehaviorExternalInterface& behaviorExternalInterface) {helperSucceeded = true;},
                                   [&helperFailed](BehaviorExternalInterface& behaviorExternalInterface) {helperFailed = true;});
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
  
  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 5);

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
    b.SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopBehaviorAction](Robot& r) {
          printf("behavior action: %d\n", stopBehaviorAction);
          return stopBehaviorAction;
        }));
    

    // hold a helper reference so we can keep track of stuff
    ASSERT_FALSE(weak.expired());
    auto strongHelper = weak.lock();
    
    DoTicks(testBehaviorFramework, robot, b, 10);

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
    DoTicks(testBehaviorFramework, robot, b, 5);
    ASSERT_FALSE(weak.expired());
    EXPECT_EQ(rawPtr->_actionCompleteCount, 0);
    EXPECT_FALSE(helperSucceeded);
    EXPECT_FALSE(helperFailed);
  }
  // now the helper should be destroyed

  EXPECT_TRUE(weak.expired());
  rawPtr = nullptr;

  DoTicks(testBehaviorFramework, robot, b, 5);
  stopBehaviorAction = true;
  DoTicksToComplete(testBehaviorFramework, robot, b, 5);

  EXPECT_TRUE(weak.expired());
}


// Utility functions for stacking sub-helpers

struct HelperPointers {
  TestHelper* raw = nullptr;
  HelperHandle strong;
  WeakHelperHandle weak;
};

#define AddSubHelper(testFramework, r, b, p) do { SCOPED_TRACE(__LINE__); AddSubHelper_(testFramework, r, b, p); } while(0)

void AddSubHelper_(TestBehaviorFramework& testBehaviorFramework, Robot& robot,
                   TestBehaviorWithHelpers& behavior, std::vector<HelperPointers>& ptrs)
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

  DoTicks(testBehaviorFramework, robot, behavior, 2); // TODO:(bn) should try to make this work in 1 tick...
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
  DoTicks(testBehaviorFramework, robot, behavior, 8);
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

// Make a bunch of helpers which always DelegateIfInControl right away. Add more helpers to the stack by causing the
// current top one to stop acting and then immediately delegate to another helper. Eventually, the top helper
// succeeds, causing everything to succeed
TEST(BehaviorHelperSystem, MultiLayerSuccess)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(behaviorExternalInterface, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(behaviorExternalInterface);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](BehaviorExternalInterface& behaviorExternalInterface) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](BehaviorExternalInterface& behaviorExternalInterface) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());

  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(testBehaviorFramework, robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(testBehaviorFramework, robot, b, 10);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  // now stop the action on the top helper without making another delegate
  ptrs[size-1].raw->_thisSucceedsOnActionSuccess = true;
  ptrs[size-1].raw->StopAutoAction();

  DoTicks(testBehaviorFramework, robot, b, 10);

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
  DoTicksToComplete(testBehaviorFramework, robot, b, 10);
}

// Build a stack of helpers similar to the previous test, but this time test what happens if you cancel one in
// the middle, and if you cancel all of them
TEST(BehaviorHelperSystem, CancelDelegates)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(behaviorExternalInterface, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(behaviorExternalInterface);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](BehaviorExternalInterface& behaviorExternalInterface) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](BehaviorExternalInterface& behaviorExternalInterface) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());

  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(testBehaviorFramework, robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(testBehaviorFramework, robot, b, 10);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  // have some action in the middle cancel it's delegates
  ptrs[6].raw->_cancelDelegates = true;
  ptrs[6].raw->StartAutoAction(behaviorExternalInterface);
  
  DoTicks(testBehaviorFramework, robot, b, 2);

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

  DoTicks(testBehaviorFramework, robot, b, 10);
  ASSERT_EQ(ptrs.size(), size);
  CheckPtrsRunning(ptrs);

  // add a few more helpers back
  while( size < 10 ) {
    AddSubHelper(testBehaviorFramework, robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(testBehaviorFramework, robot, b, 2);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  DoTicks(testBehaviorFramework, robot, b, 10);
  ASSERT_EQ(ptrs.size(), size);
  CheckPtrsRunning(ptrs);

  // ask the behavior to stop delegates (should clear all), but keep running
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  b.StopHelperOnNextUpdate();

  DoTicks(testBehaviorFramework, robot, b, 10);
    
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
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehaviorWithHelpers b(empty);
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Running);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::UseBaseClass);

  std::vector<HelperPointers> ptrs;
  ptrs.resize(1);
  
  bool baseHelperFailed = false;
  bool baseHelperSucceeded = false;
  bool stopBehaviorAction = false;
  
  {
    ptrs[0].raw = new TestHelper(behaviorExternalInterface, b);
    auto strong = std::shared_ptr<IHelper>( ptrs[0].raw );
    ptrs[0].weak = strong; // store weak pointer
    
    ptrs[0].raw->StartAutoAction(behaviorExternalInterface);
    ptrs[0].raw->_delegateAfterAction = true;

    ASSERT_FALSE(ptrs[0].weak.expired());
    // on success, set bool and also queue another action
    b.DelegateToHelperOnNextUpdate(
      strong,
      [&baseHelperSucceeded, &b, &stopBehaviorAction](BehaviorExternalInterface& behaviorExternalInterface) {
        baseHelperSucceeded = true;
        b.SetActionToRunOnNextUpdate(new WaitForLambdaAction([&stopBehaviorAction](Robot& r) {
              printf("behavior post-helper action: %d\n", stopBehaviorAction);
              return stopBehaviorAction;
            }));
      },
      [&baseHelperFailed](BehaviorExternalInterface& behaviorExternalInterface) {baseHelperFailed = true;});
    
    ASSERT_FALSE(ptrs[0].weak.expired());
    ptrs[0].strong = strong; // store strong pointer now, to make sure it stays alive during the delegation
  }

  ASSERT_FALSE(ptrs[0].weak.expired());

  IncrementBaseStationTimerTicks();
  DoTicks(testBehaviorFramework, robot, b, 10);
  ASSERT_EQ(ptrs.size(), 1);
  CheckPtrsRunning(ptrs);

  size_t size = ptrs.size();
  
  while( size < 10 ) {
    AddSubHelper(testBehaviorFramework, robot, b, ptrs);
    size++;
    ASSERT_EQ(ptrs.size(), size);
    DoTicks(testBehaviorFramework, robot, b, 2);
    ASSERT_EQ(ptrs.size(), size);
    CheckPtrsRunning(ptrs);
  }

  DoTicks(testBehaviorFramework, robot, b, 10);
  CheckPtrsRunning(ptrs);

  b.SetUpdateResult(TestBehaviorWithHelpers::UpdateResult::Complete);

  DoTicksToComplete(testBehaviorFramework, robot, b, 3);

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
