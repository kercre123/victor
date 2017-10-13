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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Cozmo;


TEST(BehaviorInterface, Create)
{
  CozmoContext context{};
  Robot robot(0, &context);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.ReadFromScoredJson(empty);

  EXPECT_FALSE( b.IsRunning() );
  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore);
  EXPECT_TRUE( b.WantsToBeActivated(*behaviorExternalInterface));
  EXPECT_FALSE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, Init)
{
  CozmoContext context{};
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.ReadFromScoredJson(empty);

  EXPECT_FALSE( b._inited );
  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore );
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);
  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kRunningScore );
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, InitWithInterface)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);

  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b._inited );
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}


TEST(BehaviorInterface, Run)
{
  CozmoContext context{};
  Robot robot(0, &context);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.ReadFromScoredJson(empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore );
  
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);
  for(int i=0; i<5; i++) {
    EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kRunningScore );
    BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 0.01 * i ) );
    b.Update(*behaviorExternalInterface);
  }

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.OnDeactivated(*behaviorExternalInterface);

  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore );

  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 5 );
  EXPECT_TRUE( b._stopped );
}

void TickAndCheckScore( Robot& robot, ICozmoBehavior& behavior, BehaviorExternalInterface& behaviorExternalInterface, int num, float expectedScore )
{
  auto startTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  const float dt = 0.01f;
  
  for( int i=0; i<num; ++i ) {
    BaseStationTimer::getInstance()->UpdateTime( startTime + Util::SecToNanoSec( dt * i ) );
    robot.GetActionList().Update();
    behavior.Update(behaviorExternalInterface);
    EXPECT_FLOAT_EQ( expectedScore, behavior.EvaluateScore(behaviorExternalInterface) ) << "i=" << i;
  }
}


TEST(BehaviorInterface, ScoreWhileRunning)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.ReadFromScoredJson(empty);
  b.OnEnteredActivatableScope();


  BaseStationTimer::getInstance()->UpdateTime(0);

  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore );

  {
    SCOPED_TRACE("");
    b.WantsToBeActivated(*behaviorExternalInterface);
    b.OnActivated(*behaviorExternalInterface);
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore);
  }

  // this should have no effect, since we aren't acting
  b.CallIncreaseScoreWhileControlDelegated(0.1f);

  {
    SCOPED_TRACE("");
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore);
  }

  bool done = false;
  {
    SCOPED_TRACE("");
    EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileControlDelegated(0.1f);
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore + 0.1f);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileControlDelegated(1.0f);
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore + 0.1f + 1.0f);
  }

  {
    SCOPED_TRACE("");
    done = true;
    // now the behavior is not acting so the score should revert back
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileControlDelegated(0.999f);
    TickAndCheckScore(robot, b, *behaviorExternalInterface, 5, TestBehavior::kRunningScore);
  }

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.OnDeactivated(*behaviorExternalInterface);

  EXPECT_FLOAT_EQ( b.EvaluateScore(*behaviorExternalInterface), TestBehavior::kNotRunningScore );

  EXPECT_TRUE( b._inited );
  EXPECT_TRUE( b._stopped );
}

TEST(BehaviorInterface, HandleMessages)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( Ping() ) );
  
  EXPECT_EQ(b._alwaysHandleCalls, 1);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );
  b.OnDeactivated(*behaviorExternalInterface);

  robot.Broadcast( MessageEngineToGame( Ping() ) );
  
  EXPECT_EQ(b._alwaysHandleCalls, 2);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  1);
};

void DoTicks(Robot& robot, ICozmoBehavior& behavior, BehaviorExternalInterface& behaviorExternalInterface, int num=1)
{
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
    behavior.Update(behaviorExternalInterface);
  }
}

TEST(BehaviorInterface, OutsideAction)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  BaseStationTimer::getInstance()->UpdateTime(0);
  
  b.Update(*behaviorExternalInterface);
  b.Update(*behaviorExternalInterface);

  bool done = false;

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  WaitForLambdaAction* action = new WaitForLambdaAction(robot, [&done](Robot& r){ return done; });
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);

  DoTicks(robot, b, *behaviorExternalInterface);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  done = true;

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

bool TestBehavior::CallDelegateIfInControl(Robot& robot, bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action);
}

bool TestBehavior::CallDelegateIfInControlExternalCallback1(Robot& robot,
                                                    bool& actionCompleteRef,
                                                    ICozmoBehavior::RobotCompletedActionCallback callback)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, callback);
}

bool TestBehavior::CallDelegateIfInControlExternalCallback2(Robot& robot,
                                                    bool& actionCompleteRef,
                                                    ICozmoBehavior::ActionResultCallback callback)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, callback);
}

bool TestBehavior::CallDelegateIfInControlInternalCallbackVoid(Robot& robot,
                                                       bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, &TestBehavior::Foo);
}

bool TestBehavior::CallDelegateIfInControlInternalCallbackRobot(Robot& robot,
                                                        bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return DelegateIfInControl(action, &TestBehavior::Bar);
}

TEST(BehaviorInterface, DelegateIfInControlSimple)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  BaseStationTimer::getInstance()->UpdateTime(0);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  bool done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlFailures)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);

  b.OnActivated(*behaviorExternalInterface);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_FALSE( b.CallCancelDelegates() );
  
  bool done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );
  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  // action hasn't updated yet, so it's done. Should still fail to start a new action
  EXPECT_FALSE( b.CallDelegateIfInControl(robot, done) );

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  EXPECT_TRUE( b.CallCancelDelegates() );
  // same tick, should be able to start a new one
  bool done2 = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done2) );

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done2 = true;

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlCallbacks)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);

  b.OnActivated(*behaviorExternalInterface);

  bool done = false;
  bool callbackCalled = false;
  bool ret = b.CallDelegateIfInControlExternalCallback1(robot, done,
                                                [&callbackCalled](const ExternalInterface::RobotCompletedAction& res) {
                                                  callbackCalled = true;
                                                });
  EXPECT_TRUE(ret);

  DoTicks(robot, b,*behaviorExternalInterface, 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  callbackCalled = false;
  ret = b.CallDelegateIfInControlExternalCallback2(robot, done,
                                           [&callbackCalled](ActionResult res) {
                                             callbackCalled = true;
                                           });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  ret = b.CallDelegateIfInControlInternalCallbackVoid(robot, done);
  EXPECT_TRUE(ret);

  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_EQ(b._calledVoidFunc, 0);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);

  done = false;
  ret = b.CallDelegateIfInControlInternalCallbackRobot(robot, done);
  EXPECT_TRUE(ret);

  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoTicks(robot, b, *behaviorExternalInterface, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 1);
}

TEST(BehaviorInterface, DelegateIfInControlWhenNotRunning)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);

  b.OnActivated(*behaviorExternalInterface);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.OnDeactivated(*behaviorExternalInterface);

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });

  EXPECT_FALSE(ret) << "should fail to start acting since the behavior isn't running";

  BaseStationTimer::getInstance()->UpdateTime(0);
  
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  bool done2 = false;
  bool callbackCalled2 = false;
  ret = b.CallDelegateIfInControlExternalCallback2(robot, done2,
                                           [&callbackCalled2](ActionResult res) {
                                             callbackCalled2 = true;
                                           });

  EXPECT_TRUE(ret);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(callbackCalled1);
  EXPECT_FALSE(callbackCalled2);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );  

  b.OnDeactivated(*behaviorExternalInterface);
  
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
  Robot robot(0, &context);
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  TestBehavior b(empty);
  b.Init(*behaviorExternalInterface);

  BaseStationTimer::getInstance()->UpdateTime( 0 );
  
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallCancelDelegates(false);

  DoTicks(robot, b, *behaviorExternalInterface, 3);
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

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallCancelDelegates(true);

  DoTicks(robot, b, *behaviorExternalInterface, 3);
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

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override {
    _inited = true;
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    WaitForLambdaAction* action = new WaitForLambdaAction(robot, [this](Robot& r){ return _stopAction; });
    DelegateIfInControl(action);

    return RESULT_OK;
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
  Robot robot(0, &context);
  
  DelegationComponent delegationComp;
  StateChangeComponent stateChangeComp;
  BehaviorExternalInterface* behaviorExternalInterface = new BehaviorExternalInterface();
  behaviorExternalInterface->Init(robot,
                                  robot.GetAIComponent(),
                                  robot.GetBehaviorManager().GetBehaviorContainer(),
                                  robot.GetBlockWorld(),
                                  robot.GetFaceWorld(),
                                  stateChangeComp);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait, BehaviorID::Wait);

  TestInitBehavior b(empty);
  b.Init(*behaviorExternalInterface);
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated(*behaviorExternalInterface);
  b.OnActivated(*behaviorExternalInterface);

  EXPECT_FALSE(robot.GetActionList().IsEmpty()) << "action should be started by Init";

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  b._stopAction = true;

  DoTicks(robot, b, *behaviorExternalInterface, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
}
