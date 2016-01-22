/**
 * File: testActionList.cpp
 *
 * Author: Al Chaussee
 * Created: 1/21/2016
 *
 * Description: Unit tests for queueing actions
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=QueueAction*
 **/

#include "gtest/gtest.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"
#include <limits.h>


using namespace Anki::Cozmo;

class TestAction : public IAction
{
  public:
    TestAction(std::string name);
    TestAction(std::string name, RobotActionType type);
    virtual const std::string& GetName() const override { return _name; }
    virtual RobotActionType GetType() const override { return _type; }
    Robot* GetRobot() { return _robot; }
    bool _complete;
  protected:
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;
    std::string _name;
    RobotActionType _type;
};

TestAction::TestAction(std::string name)
{
  _name = name;
  _complete = false;
  _type = RobotActionType::WAIT;
}

TestAction::TestAction(std::string name, RobotActionType type)
{
  _name = name;
  _complete = false;
  _type = type;
}

ActionResult TestAction::Init() { return ActionResult::SUCCESS; }

ActionResult TestAction::CheckIfDone()
{
  return (_complete) ? ActionResult::SUCCESS : ActionResult::RUNNING;
}

class TestInterruptAction : public IAction
{
public:
  TestInterruptAction(std::string name);
  TestInterruptAction(std::string name, RobotActionType type);
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return _type; }
  Robot* GetRobot() { return _robot; }
  bool _complete;
protected:
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  virtual bool InterruptInternal() override { return true; }
  std::string _name;
  RobotActionType _type;
};

TestInterruptAction::TestInterruptAction(std::string name)
{
  _name = name;
  _complete = false;
  _type = RobotActionType::WAIT;
}

TestInterruptAction::TestInterruptAction(std::string name, RobotActionType type)
{
  _name = name;
  _complete = false;
  _type = type;
}

ActionResult TestInterruptAction::Init() { return ActionResult::SUCCESS; }

ActionResult TestInterruptAction::CheckIfDone()
{
  return (_complete) ? ActionResult::SUCCESS : ActionResult::RUNNING;
}


extern Anki::Util::Data::DataPlatform* dataPlatform;
RobotInterface::MessageHandlerStub  msgHandler;

TEST(QueueAction, SingleActionFinishes)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction = new TestAction("Test");
  testAction->_complete = true;
  
  EXPECT_NE(testAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

TEST(QueueAction, SingleActionCancel)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction = new TestAction("Test");
  testAction->_complete = true;
  
  EXPECT_NE(testAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Cancel(0, RobotActionType::WAIT);
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
}

TEST(QueueAction, ThreeActionsFinish)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1");
  testAction1->_complete = true;
  TestAction* testAction2 = new TestAction("Test2");
  TestAction* testAction3 = new TestAction("Test3");
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  testAction2->_complete = true;
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  testAction3->_complete = true;
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

TEST(QueueAction, ThreeActionsCancelSecond)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::TURN_IN_PLACE);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::TRACK_OBJECT);
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
  // I feel like cancel should remove it from the queue otherwise we waste an update cycle cancelling the action
  // instead of starting the next action
  r.GetActionList().Cancel(0, RobotActionType::TURN_IN_PLACE);
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

TEST(QueueAction, ThreeActionsCancelAll)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
  r.GetActionList().Cancel(0, RobotActionType::WAIT);
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
}

TEST(QueueAction, InterruptAction)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction = new TestAction("Test1", RobotActionType::WAIT);
  TestInterruptAction* testInterruptAction = new TestInterruptAction("Test2", RobotActionType::TURN_IN_PLACE);
  
  EXPECT_NE(testAction->GetRobot(), &r);
  EXPECT_NE(testInterruptAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testInterruptAction);
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW_AND_RESUME, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_EQ(testInterruptAction->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  // Interrupt test2 for test1 which is now current
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  // Test1 completes and Test2 is current
  testAction->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  // Test2 completes
  testInterruptAction->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}