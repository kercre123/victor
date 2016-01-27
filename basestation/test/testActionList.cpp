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
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"
#include <limits.h>


using namespace Anki::Cozmo;

// Simple action that can be set to complete
class TestAction : public IAction
{
  public:
    TestAction(std::string name, RobotActionType type);
    virtual const std::string& GetName() const override { return _name; }
    virtual RobotActionType GetType() const override { return _type; }
    Robot* GetRobot() { return _robot; }
    int _numRetries = 0;
    bool _complete = false;
  protected:
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;
    std::string _name;
    RobotActionType _type;
};

TestAction::TestAction(std::string name, RobotActionType type)
{
  _name = name;
  _type = type;
}

ActionResult TestAction::Init() { return ActionResult::SUCCESS; }

ActionResult TestAction::CheckIfDone()
{
  if(_numRetries > 0)
  {
    _numRetries--;
    return ActionResult::FAILURE_RETRY;
  }
  return (_complete ? ActionResult::SUCCESS : ActionResult::RUNNING);
}

// Simple Interruptable action that can be set to complete
class TestInterruptAction : public IAction
{
public:
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

// Simple Interruptable action that can be set to complete
class TestActionWithinAction : public IAction
{
public:
  TestActionWithinAction(std::string name, RobotActionType type);
  virtual const std::string& GetName() const override { return _name; }
  virtual RobotActionType GetType() const override { return _type; }
  Robot* GetRobot() { return _robot; }
  IActionRunner* GetAction() { return _compoundAction; }
  bool _complete;
protected:
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  virtual bool InterruptInternal() override { return true; }
  std::string _name;
  RobotActionType _type;
private:
  IActionRunner* _compoundAction;
};

TestActionWithinAction::TestActionWithinAction(std::string name, RobotActionType type)
{
  _name = name;
  _complete = false;
  _type = type;
}

ActionResult TestActionWithinAction::Init()
{
  CompoundActionSequential* compound = new CompoundActionSequential();
  _compoundAction = compound;
  EXPECT_TRUE(RegisterSubAction(_compoundAction));
  
  compound->AddAction(new TestAction("Test1", RobotActionType::WAIT));
  compound->AddAction(new TestAction("Test2", RobotActionType::WAIT));
  
  return ActionResult::SUCCESS;
}

ActionResult TestActionWithinAction::CheckIfDone()
{
  bool result = true;
  for(auto action : ((ICompoundAction*)_compoundAction)->GetActions())
  {
    result &= ((TestAction*)action.second)->_complete;
  }
  return (result ? ActionResult::SUCCESS : ActionResult::RUNNING);
}


extern Anki::Util::Data::DataPlatform* dataPlatform;
RobotInterface::MessageHandlerStub  msgHandler;

// Tests queueing a single and letting it complete normally
TEST(QueueAction, SingleActionFinishes)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction = new TestAction("Test", RobotActionType::WAIT);
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

// BROKEN because cancelling doesn't work properly
// Tets queueing a single action and cancelling it works
//TEST(QueueAction, SingleActionCancel)
//{
//  Robot r(0, &msgHandler, nullptr, dataPlatform);
//  TestAction* testAction = new TestAction("Test", RobotActionType::WAIT);
//  testAction->_complete = true;
//  
//  EXPECT_NE(testAction->GetRobot(), &r);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction);
//  
//  EXPECT_EQ(testAction->GetRobot(), &r);
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().Cancel(0, RobotActionType::WAIT);
//  
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
//}

// Tests queueing three actions and letting them complete normally
TEST(QueueAction, ThreeActionsFinish)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
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

// BROKEN because cancelling
// Tests queueing three actions, cancelling the second and letting the rest complete normally
//TEST(QueueAction, ThreeActionsCancelSecond)
//{
//  Robot r(0, &msgHandler, nullptr, dataPlatform);
//  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
//  testAction1->_complete = true;
//  TestAction* testAction2 = new TestAction("Test2", RobotActionType::TURN_IN_PLACE);
//  TestAction* testAction3 = new TestAction("Test3", RobotActionType::TRACK_OBJECT);
//  
//  EXPECT_NE(testAction1->GetRobot(), &r);
//  EXPECT_NE(testAction2->GetRobot(), &r);
//  EXPECT_NE(testAction3->GetRobot(), &r);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
//  
//  r.GetActionList().Cancel(0, RobotActionType::TURN_IN_PLACE);
//  
//  EXPECT_EQ(testAction1->GetRobot(), &r);
//  EXPECT_EQ(testAction2->GetRobot(), &r);
//  EXPECT_EQ(testAction3->GetRobot(), &r);
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
//  
//  r.GetActionList().Update();
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  testAction3->_complete = true;
//  r.GetActionList().Update();
//  
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
//}

// BROKEN because cancelling
// Tests queueing three actions and cancelling all of them
//TEST(QueueAction, ThreeActionsCancelAll)
//{
//  Robot r(0, &msgHandler, nullptr, dataPlatform);
//  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
//  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
//  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
//  
//  EXPECT_NE(testAction1->GetRobot(), &r);
//  EXPECT_NE(testAction2->GetRobot(), &r);
//  EXPECT_NE(testAction3->GetRobot(), &r);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
//  r.GetActionList().Cancel(0, RobotActionType::WAIT);
//  
//  EXPECT_EQ(testAction1->GetRobot(), &r);
//  EXPECT_EQ(testAction2->GetRobot(), &r);
//  EXPECT_EQ(testAction3->GetRobot(), &r);
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
//}

// Tests queueing two actions where the first one in interruptable so queueing the second action
// interrupts the first. The second action completes and the interrupted action resumes and completes.
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

// Tests queueing a sequential compound action made of two actions and both actions complete at the same time
TEST(QueueAction, CompoundActionSequentialSingle)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  r.SetLiftAngle(0);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  CompoundActionSequential* compoundAction = new CompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  // Can't check testAction1 and testAction2's GetRobot() as they are set internally during update()
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

// Tests queueing a sequential compound action made of two actions and they complete at different times
TEST(QueueAction, CompoundActionMultipleUpdates)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  CompoundActionSequential* compoundAction = new CompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

// Tests queueing a sequential compound action that initially has two actions and a third action is added to
// the compound action after an update
TEST(QueueAction, CompoundActionAddAfterQueued)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  CompoundActionSequential* compoundAction = new CompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  compoundAction->AddAction(testAction3);
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  // Not set yet since it hasn't been the current action during the UpdateInternal() of compoundAction
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r); // Has been set since it was added using AddAction()
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

// Tests queueing an empty sequential compound action and adding three additional actions to it at various times
TEST(QueueAction, CompoundActionAddAfterQueued2)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  CompoundActionSequential* compoundAction = new CompoundActionSequential({});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  compoundAction->AddAction(testAction1);
  
  r.GetActionList().Update();
  
  compoundAction->AddAction(testAction2);
  compoundAction->AddAction(testAction3);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
}

// Tests queueing a parallel compound action with two actions that complete immediately
TEST(QueueAction, ParallelActionImmediateComplete)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  CompoundActionParallel* compoundAction = new CompoundActionParallel({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

// Tests queueing a parallel compound action with two actions where one completes before the other
TEST(QueueAction, ParallelActionOneCompleteBefore)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  CompoundActionParallel* compoundAction = new CompoundActionParallel({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

// Currently broken because cancelling doesn't remove the action from the queue
// Tests queueing actions at the NOW position
//TEST(QueueAction, QueueNow)
//{
//  Robot r(0, &msgHandler, nullptr, dataPlatform);
//  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
//  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
//  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction1);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction2);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction3);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().Print();
//}

// Tests queueing actions at the NEXT position
TEST(QueueAction, QueueNext)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);

  r.GetActionList().QueueAction(0, QueueActionPosition::NEXT, testAction1);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);

  r.GetActionList().QueueAction(0, QueueActionPosition::NEXT, testAction2);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);

  r.GetActionList().QueueAction(0, QueueActionPosition::NEXT, testAction3);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);

  testAction1->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
}

// Tests queueing actions at the AT_END position
TEST(QueueAction, QueueAtEnd)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  testAction1->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  testAction2->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
}

// BROKEN because cancelling
//Tests queueing actions at the NOW_AND_CLEAR_REMAINING position
//TEST(QueueAction, QueueNowAndClearRemaining)
//{
//  Robot r(0, &msgHandler, nullptr, dataPlatform);
//  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
//  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
//  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
//  testAction3->_complete = true;
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
//  
//  r.GetActionList().QueueAction(0, QueueActionPosition::NOW_AND_CLEAR_REMAINING, testAction3);
//  
//  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
//  
//  r.GetActionList().Update();
//  
//  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
//  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
//}

// Tests queueing an action that has sub-actions (ie. action within an action)
TEST(QueueAction, QueueActionWithinAction)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestActionWithinAction* testActionWithinAction = new TestActionWithinAction("TestActionWithinAction", RobotActionType::UNKNOWN);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testActionWithinAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(testActionWithinAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  // Sets the subActions of testActionWithinAction to be complete so that testActionWithinAction can complete
  auto subActions = ((ICompoundAction*)testActionWithinAction->GetAction())->GetActions();
  for(auto action : subActions)
  {
    // Set during Init() when RegisterSubActions() is called
    EXPECT_EQ(((TestAction*)action.second)->GetRobot(), &r);
    ((TestAction*)action.second)->_complete = true;
  }
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}

TEST(QueueAction, ActionFailureRetry)
{
  Robot r(0, &msgHandler, nullptr, dataPlatform);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  CompoundActionSequential* compoundAction = new CompoundActionSequential({testAction1, testAction2});
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction2->SetNumRetries(3);
  testAction2->_numRetries = 1;
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  // Both actions are set to complete but testAction2 is going to retry once
  // so check both actions are still in the compound action
  auto actions = compoundAction->GetActions();
  EXPECT_EQ(actions.size(), 2);
  EXPECT_EQ(actions.front().second->GetName(), "Test1");
  EXPECT_EQ(actions.back().second->GetName(), "Test2");
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
}
