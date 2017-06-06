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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqNone.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/game/comms/uiMessageHandler.h"
#include "gtest/gtest.h"


using namespace Anki;
using namespace Anki::Cozmo;

namespace {
const float kNotRunningScore = 0.25f;
const float kRunningScore = 0.5f;
}


class TestBehavior : public IBehavior
{
public:

  TestBehavior(Robot& robot, const Json::Value& config)
    : IBehavior(robot, config)
    {
      if(robot.HasExternalInterface()) {
        SubscribeToTags({EngineToGameTag::Ping});
      }
    }

  bool _inited = false;
  int _numUpdates = 0;
  bool _stopped = false;
  virtual bool CarryingObjectHandledInternally() const override {return true;}

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override {
    return true;
  }

  virtual Result InitInternal(Robot& robot) override {
    _inited = true;
    return RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot) override {
    _numUpdates++;
    return Status::Running;
  }
  virtual void   StopInternal(Robot& robot) override {
    _stopped = true;
  }

  int _alwaysHandleCalls = 0;
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override {
    _alwaysHandleCalls++;
  }

  int _handleWhileRunningCalls = 0;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override { 
    _handleWhileRunningCalls++;
  }

  int _handleWhileNotRunningCalls = 0;
  virtual void HandleWhileNotRunning(const EngineToGameEvent& event, const Robot& robot) override {
    _handleWhileNotRunningCalls++;
  }

  int _calledVoidFunc = 0;
  void Foo() {
    _calledVoidFunc++;
  }

  int _calledRobotFunc = 0;
  void Bar(Robot& robot) {
    _calledRobotFunc++;
  }

  bool CallStartActing(Robot& robot, bool& actionCompleteRef);

  void CallIncreaseScoreWhileActing(float extraScore) { IncreaseScoreWhileActing(extraScore); }

  bool CallStartActingExternalCallback1(Robot& robot,
                                        bool& actionCompleteRef,
                                        IBehavior::RobotCompletedActionCallback callback);

  bool CallStartActingExternalCallback2(Robot& robot,
                                        bool& actionCompleteRef,
                                        IBehavior::ActionResultCallback callback);

  bool CallStartActingInternalCallbackVoid(Robot& robot,
                                           bool& actionCompleteRef);
  bool CallStartActingInternalCallbackRobot(Robot& robot,
                                            bool& actionCompleteRef);

  bool CallStopActing() { return StopActing(); }
  bool CallStopActing(bool val) { return StopActing(val); }

protected:

  virtual float EvaluateRunningScoreInternal(const Robot& robot) const override {
    return kRunningScore;
  }
  virtual float EvaluateScoreInternal(const Robot& robot) const override {
    return kNotRunningScore;
  }

};


TEST(BehaviorInterface, Create)
{
  CozmoContext context{};
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);
  b.ReadFromScoredJson(empty);

  EXPECT_FALSE( b.IsRunning() );
  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );
  BehaviorPreReqNone noPreReqs;
  EXPECT_TRUE( b.IsRunnable(noPreReqs));
  EXPECT_FALSE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, Init)
{
  CozmoContext context{};
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);
  b.ReadFromScoredJson(empty);

  EXPECT_FALSE( b._inited );
  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );
  b.Init();
  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kRunningScore );
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, InitWithInterface)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  EXPECT_FALSE( b._inited );
  b.Init();
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}


TEST(BehaviorInterface, Run)
{
  CozmoContext context{};
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);
  b.ReadFromScoredJson(empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );

  b.Init();
  for(int i=0; i<5; i++) {
    EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kRunningScore );
    BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 0.01 * i ) );
    b.Update();
  }

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.Stop();

  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );

  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 5 );
  EXPECT_TRUE( b._stopped );
}

void TickAndCheckScore( Robot& robot, IBehavior& behavior, int num, float expectedScore )
{
  auto startTime = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
  const float dt = 0.01f;
  
  for( int i=0; i<num; ++i ) {
    BaseStationTimer::getInstance()->UpdateTime( startTime + Util::SecToNanoSec( dt * i ) );
    robot.GetActionList().Update();
    behavior.Update();
    EXPECT_FLOAT_EQ( expectedScore, behavior.EvaluateScore(robot) ) << "i=" << i;
  }
}


TEST(BehaviorInterface, ScoreWhileRunning)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);
  b.ReadFromScoredJson(empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );

  {
    SCOPED_TRACE("");
    b.Init();
    TickAndCheckScore(robot, b, 5, kRunningScore);
  }

  // this should have no effect, since we aren't acting
  b.CallIncreaseScoreWhileActing(0.1f);

  {
    SCOPED_TRACE("");
    TickAndCheckScore(robot, b, 5, kRunningScore);
  }

  bool done = false;
  {
    SCOPED_TRACE("");
    EXPECT_TRUE( b.CallStartActing(robot, done) );
    TickAndCheckScore(robot, b, 5, kRunningScore);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileActing(0.1f);
    TickAndCheckScore(robot, b, 5, kRunningScore + 0.1f);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileActing(1.0f);
    TickAndCheckScore(robot, b, 5, kRunningScore + 0.1f + 1.0f);
  }

  {
    SCOPED_TRACE("");
    done = true;
    // now the behavior is not acting so the score should revert back
    TickAndCheckScore(robot, b, 5, kRunningScore);
  }

  {
    SCOPED_TRACE("");
    b.CallIncreaseScoreWhileActing(0.999f);
    TickAndCheckScore(robot, b, 5, kRunningScore);
  }

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.Stop();

  EXPECT_FLOAT_EQ( b.EvaluateScore(robot), kNotRunningScore );

  EXPECT_TRUE( b._inited );
  EXPECT_TRUE( b._stopped );
}

TEST(BehaviorInterface, HandleMessages)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.Init();

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  using namespace ExternalInterface;

  robot.Broadcast( MessageEngineToGame( Ping() ) );
  
  EXPECT_EQ(b._alwaysHandleCalls, 1);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );
  b.Stop();

  robot.Broadcast( MessageEngineToGame( Ping() ) );
  
  EXPECT_EQ(b._alwaysHandleCalls, 2);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  1);
};

void DoTicks(Robot& robot, IBehavior& behavior, int num=1)
{
  for(int i=0; i<num; i++) {
    robot.GetActionList().Update();
    behavior.Update();
  }
}

TEST(BehaviorInterface, OutsideAction)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);
  b.Init();
  
  b.Update();
  b.Update();

  bool done = false;

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  WaitForLambdaAction* action = new WaitForLambdaAction(robot, [&done](Robot& r){ return done; });
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);

  DoTicks(robot, b);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  DoTicks(robot, b, 3);

  done = true;

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

bool TestBehavior::CallStartActing(Robot& robot, bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return StartActing(action);
}

bool TestBehavior::CallStartActingExternalCallback1(Robot& robot,
                                                    bool& actionCompleteRef,
                                                    IBehavior::RobotCompletedActionCallback callback)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return StartActing(action, callback);
}

bool TestBehavior::CallStartActingExternalCallback2(Robot& robot,
                                                    bool& actionCompleteRef,
                                                    IBehavior::ActionResultCallback callback)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return StartActing(action, callback);
}

bool TestBehavior::CallStartActingInternalCallbackVoid(Robot& robot,
                                                       bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return StartActing(action, &TestBehavior::Foo);
}

bool TestBehavior::CallStartActingInternalCallbackRobot(Robot& robot,
                                                        bool& actionCompleteRef)
{
  WaitForLambdaAction* action =
    new WaitForLambdaAction(robot, [&actionCompleteRef](Robot& r){ return actionCompleteRef; });

  return StartActing(action, &TestBehavior::Bar);
}

TEST(BehaviorInterface, StartActingSimple)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);
  
  b.Init();

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  bool done = false;
  EXPECT_TRUE( b.CallStartActing(robot, done) );

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, StartActingFailures)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);
  
  b.Init();

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_FALSE( b.CallStopActing() );
  
  bool done = false;
  EXPECT_TRUE( b.CallStartActing(robot, done) );
  EXPECT_FALSE( b.CallStartActing(robot, done) );

  DoTicks(robot, b, 3);

  EXPECT_FALSE( b.CallStartActing(robot, done) );

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  // action hasn't updated yet, so it's done. Should still fail to start a new action
  EXPECT_FALSE( b.CallStartActing(robot, done) );

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  done = false;
  EXPECT_TRUE( b.CallStartActing(robot, done) );

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  EXPECT_TRUE( b.CallStopActing() );
  // same tick, should be able to start a new one
  bool done2 = false;
  EXPECT_TRUE( b.CallStartActing(robot, done2) );

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done2 = true;

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, StartActingCallbacks)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);
  
  BaseStationTimer::getInstance()->UpdateTime(0);

  b.Init();

  bool done = false;
  bool callbackCalled = false;
  bool ret = b.CallStartActingExternalCallback1(robot, done,
                                                [&callbackCalled](const ExternalInterface::RobotCompletedAction& res) {
                                                  callbackCalled = true;
                                                });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoTicks(robot, b, 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  callbackCalled = false;
  ret = b.CallStartActingExternalCallback2(robot, done,
                                           [&callbackCalled](ActionResult res) {
                                             callbackCalled = true;
                                           });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);
  EXPECT_FALSE(callbackCalled);
  done = true;
  DoTicks(robot, b, 3);
  EXPECT_TRUE(callbackCalled);

  done = false;
  ret = b.CallStartActingInternalCallbackVoid(robot, done);
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);
  EXPECT_EQ(b._calledVoidFunc, 0);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoTicks(robot, b, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);

  done = false;
  ret = b.CallStartActingInternalCallbackRobot(robot, done);
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 0);
  done = true;
  DoTicks(robot, b, 3);
  EXPECT_EQ(b._calledVoidFunc, 1);
  EXPECT_EQ(b._calledRobotFunc, 1);
}

TEST(BehaviorInterface, StartActingWhenNotRunning)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime(0);

  b.Init();

  DoTicks(robot, b, 3);

  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );

  b.Stop();

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallStartActingExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });

  EXPECT_FALSE(ret) << "should fail to start acting since the behavior isn't running";

  BaseStationTimer::getInstance()->UpdateTime(0);
  
  b.Init();

  DoTicks(robot, b, 3);

  bool done2 = false;
  bool callbackCalled2 = false;
  ret = b.CallStartActingExternalCallback2(robot, done2,
                                           [&callbackCalled2](ActionResult res) {
                                             callbackCalled2 = true;
                                           });

  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);

  EXPECT_FALSE(callbackCalled1);
  EXPECT_FALSE(callbackCalled2);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());
  
  BaseStationTimer::getInstance()->UpdateTime( Util::SecToNanoSec( 2.0 ) );  

  b.Stop();
  
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
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestBehavior b(robot, empty);

  BaseStationTimer::getInstance()->UpdateTime( 0 );
  
  b.Init();

  DoTicks(robot, b, 3);

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallStartActingExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallStopActing(false);

  DoTicks(robot, b, 3);
  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should be canceled";
  EXPECT_FALSE(callbackCalled1) << "StartActing callback should not have run";

  ///////////////////////////////////////////////////////////////////////////////
  // now do it with true

  callbackCalled1 = false;
  
  ret = b.CallStartActingExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });
  EXPECT_TRUE(ret);

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  // stop acting but don't allow the callback
  b.CallStopActing(true);

  DoTicks(robot, b, 3);
  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should be canceled";
  EXPECT_TRUE(callbackCalled1) << "Should have gotten callback this time";

}

class TestInitBehavior : public IBehavior
{
public:

  TestInitBehavior(Robot& robot, const Json::Value& config)
    :IBehavior(robot, config)
    {
    }

  bool _inited = false;
  int _numUpdates = 0;
  bool _stopped = false;

  bool _stopAction = false;

  virtual bool CarryingObjectHandledInternally() const override {return true;}

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override {
    return true;
  }

  virtual Result InitInternal(Robot& robot) override {
    _inited = true;
    WaitForLambdaAction* action = new WaitForLambdaAction(robot, [this](Robot& r){ return _stopAction; });
    StartActing(action);

    return RESULT_OK;
  }
  
  virtual Status UpdateInternal(Robot& robot) override {
    _numUpdates++;
    return Status::Running;
  }
  virtual void   StopInternal(Robot& robot) override {
    _stopped = true;
  }

};

TEST(BehaviorInterface, StartActingInsideInit)
{
  UiMessageHandler handler(0, nullptr);
  CozmoContext context(nullptr, &handler);
  Robot robot(0, &context);
  Json::Value empty = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);

  TestInitBehavior b(robot, empty);

  b.Init();

  EXPECT_FALSE(robot.GetActionList().IsEmpty()) << "action should be started by Init";

  DoTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  b._stopAction = true;

  DoTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
}
