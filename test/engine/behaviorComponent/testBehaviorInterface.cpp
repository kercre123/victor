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

#include "clad/types/behaviorComponent/behaviorTypes.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/cozmoAPI/comms/uiMessageHandler.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"

using namespace Anki;
using namespace Anki::Vector;

static const BehaviorClass emptyClass = BEHAVIOR_CLASS(Wait);
static const BehaviorID emptyID = BEHAVIOR_ID(Wait);
static const BehaviorID emptyID2 = BEHAVIOR_ID(Wait_TestInjectable);


TEST(BehaviorInterface, Create)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.InitBehaviorOperationModifiers();
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b.IsActivated() );
  EXPECT_TRUE( b.WantsToBeActivated());
  EXPECT_FALSE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, Init)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, bc);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.InitBehaviorOperationModifiers();
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b._inited );
  b.WantsToBeActivated();
  b.OnActivated();
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}

TEST(BehaviorInterface, InitWithInterface)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);

  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.InitBehaviorOperationModifiers();
  b.OnEnteredActivatableScope();

  EXPECT_FALSE( b._inited );
  b.WantsToBeActivated();
  b.OnActivated();
  EXPECT_TRUE( b._inited );
  EXPECT_EQ( b._numUpdates, 0 );
  EXPECT_FALSE( b._stopped );
}


TEST(BehaviorInterface, Run)
{
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, bc);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  
  BaseStationTimer::getInstance()->UpdateTime(0);

  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.InitBehaviorOperationModifiers();
  b.OnEnteredActivatableScope();
  
  b.WantsToBeActivated();
  b.OnActivated();


  BaseStationTimer::getInstance()->UpdateTime( BaseStationTimer::getInstance()->GetTickCount() + 1);

  b.OnDeactivated();

  EXPECT_TRUE( b._inited );
  EXPECT_TRUE( b._stopped );
}


TEST(BehaviorInterface, HandleMessages)
{
  TestBehaviorFramework testBehaviorFramework;
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
  b2.InitBehaviorOperationModifiers();
  b2.OnEnteredActivatableScope();
  b2.WantsToBeActivated();
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
    DependencyManagedEntity<AIComponentID> dependentComps;
    testBehaviorFramework.GetBehaviorComponent().UpdateDependent(dependentComps);
  }
  
  EXPECT_EQ(b._alwaysHandleCalls, 1);
  EXPECT_EQ(b._handleWhileRunningCalls, 1);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);

  EXPECT_EQ(b2._alwaysHandleCalls, 1);
  EXPECT_EQ(b2._handleWhileRunningCalls, 1);
  EXPECT_EQ(b2._handleWhileNotRunningCalls,  0);

  IncrementBaseStationTimerTicks();
  b.OnDeactivated();

  robot.Broadcast( MessageEngineToGame( Ping() ) );

  // Tick the behavior component to send message to behavior
  {
    DependencyManagedEntity<AIComponentID> dependentComps;
    testBehaviorFramework.GetBehaviorComponent().UpdateDependent(dependentComps);
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
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, container);
  }
  
  BehaviorExternalInterface& behaviorExternalInterface = testBehaviorFramework.GetBehaviorExternalInterface();
  
  Robot& robot = testBehaviorFramework.GetRobot();
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  TestBehavior b(empty);
  b.Init(behaviorExternalInterface);
  b.InitBehaviorOperationModifiers();
  b.OnEnteredActivatableScope();
  b.WantsToBeActivated();
  b.OnActivated();

  
  b.Update();
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + 1);
  b.Update();
  BaseStationTimer::getInstance()->UpdateTime(BaseStationTimer::getInstance()->GetTickCount() + 1);

  bool done = false;

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
  
  WaitForLambdaAction* action = new WaitForLambdaAction([&done](Robot& r){ return done; });
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);

  DoBehaviorInterfaceTicks(robot, b);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  DoBehaviorInterfaceTicks(robot, b, 3);

  done = true;

  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlSimple)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();
  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  bool done = false;
  EXPECT_TRUE( b.CallDelegateIfInControl(robot, done) );

  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  done = true;

  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());

  EXPECT_EQ(b._alwaysHandleCalls, 0);
  EXPECT_EQ(b._handleWhileRunningCalls, 0);
  EXPECT_EQ(b._handleWhileNotRunningCalls,  0);
}

TEST(BehaviorInterface, DelegateIfInControlFailures)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
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
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
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
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* container = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, container);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();

  DoBehaviorComponentTicks(robot, b, testBehaviorFramework.GetBehaviorComponent(), 3);

  b.OnDeactivated();

  bool done1 = false;
  bool callbackCalled1 = false;
  bool ret = b.CallDelegateIfInControlExternalCallback2(robot, done1,
                                                [&callbackCalled1](ActionResult res) {
                                                  callbackCalled1 = true;
                                                });

  EXPECT_FALSE(ret) << "should fail to start acting since the behavior isn't running";

  b.OnLeftActivatableScope();
  b.OnEnteredActivatableScope();
  
  b.WantsToBeActivated();
  b.OnActivated();

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

  b.OnDeactivated();
  
  DependencyManagedEntity<RobotComponentID> robotComps;
  robot.GetActionList().UpdateDependent(robotComps);
  robot.GetActionList().UpdateDependent(robotComps);
  robot.GetActionList().UpdateDependent(robotComps);

  EXPECT_TRUE(robot.GetActionList().IsEmpty()) << "action should have been canceled by stop";

  EXPECT_FALSE(callbackCalled1);
  EXPECT_FALSE(callbackCalled2) << "callback shouldn't happen if behavior isn't running";
}

TEST(BehaviorInterface, StopActingWithoutCallback)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
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

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual bool WantsToBeActivatedBehavior() const override {
    return true;
  }

  virtual void OnBehaviorActivated() override {
    _inited = true;
    WaitForLambdaAction* action = new WaitForLambdaAction([this](Robot& r){ return _stopAction; });
    DelegateIfInControl(action);
  }
  
  virtual void BehaviorUpdate() override {
    if(!IsActivated()){
      return;
    }
    _numUpdates++;
  }
  virtual void OnBehaviorDeactivated() override {
    _stopped = true;
  }

};

TEST(BehaviorInterface, DelegateIfInControlInsideInit)
{
  BaseStationTimer::getInstance()->UpdateTime(0);
  
  Json::Value empty = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
  TestInitBehavior b(empty);
  
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&b, nullptr, true, bc);
  }
  
  Robot& robot = testBehaviorFramework.GetRobot();
  
  IncrementBaseStationTimerTicks();

  EXPECT_FALSE(robot.GetActionList().IsEmpty()) << "action should be started by Init";

  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_FALSE(robot.GetActionList().IsEmpty());

  b._stopAction = true;

  DoBehaviorInterfaceTicks(robot, b, 3);

  EXPECT_TRUE(robot.GetActionList().IsEmpty());
}

class TestBehavior_RespondsUserIntents : public ICozmoBehavior
{
public:
  TestBehavior_RespondsUserIntents(const Json::Value& config)
    :ICozmoBehavior(config)
  {
    _type = config["responseType"].asString();
  
    if( _type == "user intent tags" ) {
      AddWaitForUserIntent( USER_INTENT(test_user_intent_1) );
      AddWaitForUserIntent( USER_INTENT(test_name) );
    }
    else if( _type == "user intent lambdas" ) {
      AddWaitForUserIntent( USER_INTENT(test_name), [](const UserIntent& intent){
        return intent.Get_test_name().name == "Victor";
      });
    }
    else if( _type == "user intent intent Victor" ) {
      UserIntent intent;
      intent.Set_test_name( UserIntent_Test_Name{"Victor"} );
      AddWaitForUserIntent( std::move(intent) );
    }
    else if( _type == "user intent intent empty" ) {
      UserIntent intent;
      intent.Set_test_name( UserIntent_Test_Name{""} );
      AddWaitForUserIntent( std::move(intent) );
    }
    else if( _type == "trigger" ) {
      SetRespondToTriggerWord( true );
    }
    // otherwise don't do anything
  }
  
  std::string _type;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual bool WantsToBeActivatedBehavior() const override {
    return true;
  }

  virtual void OnBehaviorActivated() override {}

};

TEST(BehaviorInterface, BehaviorRespondsToUserIntents)
{
  // test all the ways we can make this behavior wait for user intents
  TestBehaviorFramework testBehaviorFramework;
  RobotDataLoader::BehaviorIDJsonMap emptyBehaviorMap;
  Json::Value emptyConfig = ICozmoBehavior::CreateDefaultBehaviorConfig(BehaviorClass::Wait , BehaviorID::Anonymous);
  TestBehavior emptyBase(emptyConfig);
  {
    BehaviorContainer* bc = new BehaviorContainer(emptyBehaviorMap);
    testBehaviorFramework.InitializeStandardBehaviorComponent(&emptyBase, nullptr, true, bc);
  }
  
  BehaviorExternalInterface& bei = testBehaviorFramework.GetBehaviorExternalInterface();
  auto& uic = bei.GetAIComponent().GetComponent<BehaviorComponent>().GetComponent<UserIntentComponent>();
  
  for( int i=0; i<=11; ++i ) {
    bool expectingCallback = false;
    Json::Value config = ICozmoBehavior::CreateDefaultBehaviorConfig(emptyClass, emptyID);
    
    if( i == 0 ) { // behavior isn't listening for anything
      
    }
    else if( i >= 1 && i <= 3 ) { // 1-3 are behavior waiting for tags (one tag, the other tag, a wrong tag)
      config["responseType"] = "user intent tags";
    }
    else if( i >= 4 && i <= 6 ) { // 4-6 are lambdas (lambda with correct data, lambda with wrong data, a wrong tag)
      config["responseType"] = "user intent lambdas";
    }
    else if( i >= 7 && i <= 9 ) { // 7-9 are intents (intent with correct data, intent with wrong/empty data, wrong tag)
      config["responseType"] = "user intent intent Victor";
    }
    else if( i == 10 || i == 11 ) { // 10-11 are intents with null string (intent with null string, intent with nonnull string)
      config["responseType"] = "user intent intent empty";
    }
    else if( i == 12 ) { // 12 is trigger
      config["responseType"] = "trigger";
    }
    TestBehavior_RespondsUserIntents b(config);
    
    UserIntent passedIntent;
    
    if( i==1 || i==2 || i==4 || i==7 || i==10 ) {
      expectingCallback = true;
    }
    
    b.Init( bei );
    b.InitBehaviorOperationModifiers();
    b.OnEnteredActivatableScope();
    
    EXPECT_FALSE( uic.IsAnyUserIntentPending() );
    EXPECT_FALSE( uic.IsTriggerWordPending() );

    EXPECT_TRUE( b.WantsToBeActivatedBehavior() ); // it's the ICozmoBehavior we're testing here, not b
    
    switch( i ) {
      case 0: // behavior isnt listening to intents, so it should always be true
      {
        passedIntent.Set_test_name( UserIntent_Test_Name{""} );
        EXPECT_TRUE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( std::move(passedIntent) , UserIntentSource::Voice);
        EXPECT_TRUE( b.WantsToBeActivatedInternal() );
        uic.DropUserIntent( USER_INTENT(test_name) );
      }
        break;
      case 1: // behavior is waiting for test_user_intent_1; also, it should activate the intent
      {
        passedIntent.Set_test_user_intent_1({});
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( passedIntent.GetTag() , UserIntentSource::Voice);
        EXPECT_TRUE( b.WantsToBeActivated() );
        b.OnActivated();
        EXPECT_FALSE( uic.IsUserIntentPending(USER_INTENT(test_user_intent_1)) );
        EXPECT_FALSE( uic.IsAnyUserIntentPending() );
        EXPECT_TRUE( uic.IsUserIntentActive(USER_INTENT(test_user_intent_1)) );
        b.OnDeactivated();
        EXPECT_FALSE( uic.IsUserIntentPending(USER_INTENT(test_user_intent_1)) );
        EXPECT_FALSE( uic.IsAnyUserIntentPending() );
        EXPECT_FALSE( uic.IsUserIntentActive(USER_INTENT(test_user_intent_1)) );
      }
        break;
      case 2: // behavior is waiting for test_name; also, it should activate the intent
      case 4: // behavior is waiting for test_name with "Victor"
      case 7: // behavior is waiting for test_name with "Victor"
      {
        passedIntent.Set_test_name( UserIntent_Test_Name{"Victor"} );
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( std::move(passedIntent) , UserIntentSource::Voice);
        EXPECT_TRUE( b.WantsToBeActivated() );
        b.OnActivated();
        EXPECT_FALSE( uic.IsUserIntentPending(USER_INTENT(test_name)) );
        EXPECT_FALSE( uic.IsAnyUserIntentPending() );
        EXPECT_TRUE( uic.IsUserIntentActive(USER_INTENT(test_name)) );
        b.OnDeactivated();
        EXPECT_FALSE( uic.IsUserIntentPending(USER_INTENT(test_name)) );
        EXPECT_FALSE( uic.IsAnyUserIntentPending() );
        EXPECT_FALSE( uic.IsUserIntentActive(USER_INTENT(test_name)) );

      }
        break;
      case 3: // passing a different tag. should not want to start
      case 6: // passing a different tag. should not want to start
      case 9: // passing a different tag. should not want to start
      {
        passedIntent.Set_test_user_intent_2({});
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( passedIntent.GetTag() , UserIntentSource::Voice);
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DropUserIntent( USER_INTENT(test_user_intent_2) );
      }
        break;
      case 5: // behavior is waiting for test_name with "Victor" but we give it Cozmo
      case 8: // behavior is waiting for test_name with "Victor" but we give it Cozmo
      case 11: // behavior is waiting for test_name with "" but we give it Cozmo
      {
        passedIntent.Set_test_name( UserIntent_Test_Name{"Cozmo"} );
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( std::move(passedIntent) , UserIntentSource::Voice);
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DropUserIntent( USER_INTENT(test_name) );
      }
        break;
      case 10: // behavior is waiting for test_name with "".
      {
        passedIntent.Set_test_name( UserIntent_Test_Name{""} );
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.DevSetUserIntentPending( std::move(passedIntent) , UserIntentSource::Voice);
        EXPECT_TRUE( b.WantsToBeActivated() );
        b.OnActivated();
        EXPECT_FALSE( uic.IsUserIntentPending(USER_INTENT(test_name)) );
        EXPECT_FALSE( uic.IsAnyUserIntentPending() );
      }
        break;
      case 12: // behavior is waiting for a trigger
      {
        EXPECT_FALSE( b.WantsToBeActivated() );
        uic.SetTriggerWordPending();
        EXPECT_TRUE( b.WantsToBeActivated() );
        b.OnActivated();
        EXPECT_FALSE( uic.IsTriggerWordPending() );
      }
        break;
    }
  }
  
}

