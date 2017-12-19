/**
 * File: testBehaviorInterface.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-02-10
 *
 * Description: Test of the behavior interface and the functionality it provides
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Cozmo;

static const BehaviorClass emptyClass = BEHAVIOR_CLASS(Wait);
static const BehaviorID emptyID = BEHAVIOR_ID(Wait);
static const BehaviorID emptyID2 = BEHAVIOR_ID(Wait_TestInjectable);


TEST(BehaviorInterface, Create)
{
  CozmoContext context{};
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b.IsActivated() );
  EXPECT_TRUE( b.WantsToBeActivated(behaviorExternalInterface));
  EXPECT_FALSE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, Init)
{
  CozmoContext context{};
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true, bc);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b._inited );
  b.WantsToBeActivated(behaviorExternalInterface);
  b.OnActivated(behaviorExternalInterface);
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, InitWithInterface)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);

  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b._inited );
  b.WantsToBeActivated(behaviorExternalInterface);
  b.OnActivated(behaviorExternalInterface);
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}


TEST(BehaviorInterface, Run)
{
  CozmoContext context{};
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true, bc);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  
  BaseStationTimer::getInstance()->UpdateTime(0);

  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  
  b.WantsToBeActivated(behaviorExternalInterface);
  b.OnActivated(behaviorExternalInterface);


  BaseStationTimer::getInstance()->UpdateTime( BaseStationTimer::getInstance()->GetTickCount() + 1);

  b.OnDeactivated(behaviorExternalInterface);

  EXPECT_TRUE( b._inited );
  EXPECT_TRUE( b._stopped );
}


TEST(BehaviorInterface, HandleMessages)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  Json::Value empty2 = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID2);
  TestBehavior b2(empty2);

  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
    IncrementBaseStationTimerTicks();    
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();  
  b2.Init(behaviorExternalInterface);
  b2.OnEnteredActivatableScope();
  b2.WantsToBeActivated(behaviorExternalInterface);
  InjectBehaviorIntoStack(b2, testBehaviorFramework);

  Robot& robot = testBehaviorFramework.GetRobot();

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  EXPECT_EQ(b2._alwaysHandleCalls, 0);
  EXPECT_EQ(b2._handleWhileRunningCalls, 0);
  EXPECT_EQ(b2._handleWhileNotRunningCalls,  0);

  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( Ping() ) );
  
  // Tick the behavior component to send message to behavior
  {
    std::string currentActivityName;
    std::string behaviorDebugStr;
    testBehaviorFramework.GetBehaviorComponent().Update(robot, currentActivityName, behaviorDebugStr);
  }
  
  EXPECT_EQ(b._alwaysHandleCalls, 1);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  EXPECT_EQ(b2._alwaysHandleCalls, 1);
  EXPECT_EQ(b2._handleWhileRunningCalls, 1);
  EXPECT_EQ(b2._handleWhileNotRunningCalls,  0);

  IncrementBaseStationTimerTicks();
  b.OnDeactivated(behaviorExternalInterface);

  robot.Broadcast( MessageEngineToGame( Ping() ) );

  // Tick the behavior component to send message to behavior
  {
    std::string currentActivityName;
    std::string behaviorDebugStr;
    testBehaviorFramework.GetBehaviorComponent().Update(robot, currentActivityName, behaviorDebugStr);
  }
  
  EXPECT_EQ(b._alwaysHandleCalls, 2);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  1);

  EXPECT_EQ(b2._alwaysHandleCalls, 2);
  EXPECT_EQ(b2._handleWhileRunningCalls, 1);
  EXPECT_EQ(b2._handleWhileNotRunningCalls,  1);

};

TEST(BehaviorInterface, OutsideAction)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(nullptr, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Robot& robot = testBehaviorFramework.GetRobot();
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(behaviorExternalInterface);
  b.OnActivated(behaviorExternalInterface);

  
  b.Update(behaviorExternalInterface);
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + 1);
  b.Update(behaviorExternalInterface);
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + 1);

  bool done = false;

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  WaitForLambdaAction* action = new WaitForLambdaAction([&done](Robot& r){ return done; });
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  done = true;

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlSimple)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  IncrementBaseStationTimerTicks();
  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  bool done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlFailures)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, container);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_FALSE( b.CallCancelDelegates() );
  
  bool done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );
  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  // action hasn't updated yet, so it's done. Should still fail to start a new action
  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  EXPECT_TRUE( b.CallCancelDelegates() );
  // same tick, should be able to start a new one
  bool done2 = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done2) );

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done2 = true;

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlCallbacks)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, container);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();

  bool done = false;
  bool callbackCalled = false;
  bool ret = b.CallDelegateIfInControlExternalCallback1(robot, done,
                                                [&callbackCalled](const ExternalInterface::RobotCompletedAction& res) {
                                                  callbackCalled = true;
                                                });
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  callbackCalled = false;
  ret = b.CallDelegateIfInControlExternalCallback2(robot, done,
                                           [&callbackCalled](ActionResult res) {
                                             callbackCalled = true;
                                           });
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  ret = b.CallDelegateIfInControlInternalCallbackVoid(robot, done);
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_EQ(b._calledVoidFunc, 0);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);

  done = false;
  ret = b.CallDelegateIfInControlInternalCallbackRobot(robot, done);
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 1);
}

TEST(BehaviorInterface, DelegateIfInControlWhenNotRunning)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, container);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  IncrementBaseStationTimerTicks();

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  b.OnDeactivated(behaviorExternalInterface);

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });

  EXPECT_FALSE(ret) << "should fail to start acting since the behavior isn't running";

  b.OnLeftActivatableScope();
  b.OnEnteredActivatableScope();
  
  b.WantsToBeActivated(behaviorExternalInterface);
  b.OnActivated(behaviorExternalInterface);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  bool done2 = false;
  bool callbackCalled2 = false;
  ret = b.CallDelegateIfInControlExternalCallback2(robot, done2,
                                           [&callbackCalled2](ActionResult res) {
                                             callbackCalled2 = true;
                                           });

  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE(callbackCalled1);
  EXPECT_FALSE(callbackCalled2);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );  

  b.OnDeactivated(behaviorExternalInterface);
  
  robot.GetActionList().Update();
  robot.GetActionList().Update();
  robot.GetActionList().Update();

  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should have been canceled by stop";

  EXPECT_FALSE(callbackCalled1);
  EXPECT_FALSE(callbackCalled2) << "callback shouldn't happen if behavior isn't running";
}

TEST(BehaviorInterface, StopActingWithoutCallback)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, container);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();
  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallCancelDelegates(false);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should be canceled";
  EXPECT_FALSE(callbackCalled1) << "DelegateIfInControl callback should not have run";

  ///////////////////////////////////////////////////////////////////////////////
  // now do it with true

  callbackCalled1 = false;
  
  ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });
  EXPECT_TRUE(ret);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallCancelDelegates(true);

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);
  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should be canceled";
  EXPECT_TRUE(callbackCalled1) << "Should have gotten callback this time";

}

class TestInitBehavior : public ICozmoBehavior
{
public:

  TestInitBehavior(const Json::Value& config)
    :ICozmoBehavior(config)
    {
    }

  bool _inited = false;
  int _numUpdates = 0;
  bool _stopped = false;

  bool _stopAction = false;

  virtual bool CarryingObjectHandledInternally() const override {return true;}

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    return true;
  }

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override {
    _inited = true;
    WaitForLambdaAction* action = new WaitForLambdaAction([this](Robot& r){ return _stopAction; });
    DelegateIfInControl(action);
  }
  
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override {
    _numUpdates++;
    return Status::Running;
  }
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override {
    _stopped = true;
  }

};

TEST(BehaviorInterface, DelegateIfInControlInsideInit)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestInitBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework(1, &context);
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  auto& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  IncrementBaseStationTimerTicks();

  EXPECT_FALSE(robot.GetActionList().IsEmpty()) << "action should be started by Init";

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  b._stopAction = true;

  DoBehaviorInterfaceTicks(robot, b, behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
}
