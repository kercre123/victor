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
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"
#include <limits.h>


using namespace Anki::Cozmo;

int numActionsDestroyed = 0;

// Simple action that can be set to complete
class TestAction : public IAction
{
  public:
    TestAction(std::string name, RobotActionType type);
    virtual ~TestAction() { numActionsDestroyed++; }
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
  virtual ~TestInterruptAction() { numActionsDestroyed++; }
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

// Simple compound action with a function to expose its internal list of actions
class TestCompoundActionSequential : public CompoundActionSequential
{
public:
  TestCompoundActionSequential(std::initializer_list<IActionRunner*> actions);
  virtual ~TestCompoundActionSequential() { numActionsDestroyed++; }
  virtual std::list<std::pair<bool, IActionRunner*>> GetActions() { return _actions; }
};

TestCompoundActionSequential::TestCompoundActionSequential(std::initializer_list<IActionRunner*> actions)
: CompoundActionSequential(actions)
{
  
}


// Simple parallel action with a function to expose its internal list of actions
class TestCompoundActionParallel : public CompoundActionParallel
{
public:
  TestCompoundActionParallel(std::initializer_list<IActionRunner*> actions);
  virtual ~TestCompoundActionParallel() { numActionsDestroyed++; }
  virtual std::list<std::pair<bool, IActionRunner*>> GetActions() { return _actions; }
};

TestCompoundActionParallel::TestCompoundActionParallel(std::initializer_list<IActionRunner*> actions)
: CompoundActionParallel(actions)
{
  
}

// Simple Interruptable action that can be set to complete
class TestActionWithinAction : public IAction
{
public:
  TestActionWithinAction(std::string name, RobotActionType type);
  virtual ~TestActionWithinAction() { numActionsDestroyed++; }
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
  TestCompoundActionSequential* compound = new TestCompoundActionSequential({});
  _compoundAction = compound;
  EXPECT_TRUE(RegisterSubAction(_compoundAction));
  
  compound->AddAction(new TestAction("Test1", RobotActionType::WAIT));
  compound->AddAction(new TestAction("Test2", RobotActionType::WAIT));
  
  return ActionResult::SUCCESS;
}

ActionResult TestActionWithinAction::CheckIfDone()
{
  bool result = true;
  for(auto action : ((TestCompoundActionSequential*)_compoundAction)->GetActions())
  {
    result &= ((TestAction*)action.second)->_complete;
  }
  return (result ? ActionResult::SUCCESS : ActionResult::RUNNING);
}


extern Anki::Cozmo::CozmoContext* cozmoContext;

// Tests queueing a single and letting it complete normally
TEST(QueueAction, SingleActionFinishes)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test", RobotActionType::WAIT);
  testAction->_complete = true;
  
  EXPECT_NE(testAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 1);
}

// Tets queueing a single action and cancelling it works
TEST(QueueAction, SingleActionCancel)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test", RobotActionType::WAIT);
  testAction->_complete = true;
  
  EXPECT_NE(testAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Cancel(RobotActionType::WAIT);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(numActionsDestroyed, 1);
}

// Tests queueing three actions and letting them complete normally
TEST(QueueAction, ThreeActionsFinish)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
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
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing three actions, cancelling the second and letting the rest complete normally
TEST(QueueAction, ThreeActionsCancelSecond)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::TURN_IN_PLACE);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::TRACK_OBJECT);
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
  r.GetActionList().Cancel(RobotActionType::TURN_IN_PLACE);
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction3->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing three actions and cancelling all of them
TEST(QueueAction, ThreeActionsCancelAll)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  r.GetActionList().Cancel(RobotActionType::WAIT);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}


// Tests queueing two actions where the first one, which hasn't started running, is interruptable so
// queueing the second action interrupts the first. The second action completes and the interrupted
// action resumes and completes.
TEST(QueueAction, InterruptActionNotRunning)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test1", RobotActionType::WAIT);
  TestInterruptAction* testInterruptAction = new TestInterruptAction("Test2", RobotActionType::TURN_IN_PLACE);
  
  EXPECT_NE(testAction->GetRobot(), &r);
  EXPECT_NE(testInterruptAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testInterruptAction);
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_RESUME, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_EQ(testInterruptAction->GetRobot(), &r);
  
  // Interrupt test2 for test1 which is now current
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
  EXPECT_EQ(numActionsDestroyed, 2);
}

// Tests queueing two actions where the first one ,which has started running, is interruptable so queueing
// the second action interrupts it. The second action completes and the interrupted action
// resumes and completes.
TEST(QueueAction, InterruptActionRunning)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test1", RobotActionType::WAIT);
  TestInterruptAction* testInterruptAction = new TestInterruptAction("Test2", RobotActionType::TURN_IN_PLACE);
  
  EXPECT_NE(testAction->GetRobot(), &r);
  EXPECT_NE(testInterruptAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testInterruptAction);
  
  EXPECT_EQ(testInterruptAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  // Move it out of queue to currentAction
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_RESUME, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_EQ(testInterruptAction->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  
  testInterruptAction->_complete = true;
  testAction->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 2);
}

// Tests queueing a sequential compound action made of two actions and both actions complete at the same time
TEST(QueueAction, CompoundActionSequentialSingle)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  // Can't check testAction1 and testAction2's GetRobot() as they are set internally during update()
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing a sequential compound action made of two actions and they complete at different times
TEST(QueueAction, CompoundActionMultipleUpdates)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing a sequential compound action that initially has two actions and a third action is added to
// the compound action after an update
TEST(QueueAction, CompoundActionAddAfterQueued)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  compoundAction->AddAction(testAction3);
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
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
  EXPECT_EQ(numActionsDestroyed, 4);
}

// Tests queueing an empty sequential compound action and adding three additional actions to it at various times
TEST(QueueAction, CompoundActionAddAfterQueued2)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(testAction3->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
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
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  r.GetActionList().Update();
  
  EXPECT_EQ(numActionsDestroyed, 4);
}

// Tests queueing a parallel compound action with two actions that complete immediately
TEST(QueueAction, ParallelActionImmediateComplete)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing a parallel compound action with two actions where one completes before the other
TEST(QueueAction, ParallelActionOneCompleteBefore)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel({testAction1, testAction2});
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(testAction1->GetRobot(), &r);
  EXPECT_EQ(testAction2->GetRobot(), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing actions at the NOW position
TEST(QueueAction, QueueNow)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  EXPECT_EQ(numActionsDestroyed, 0);
}

// Tests queueing actions at the NEXT position
TEST(QueueAction, QueueNext)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction1);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction2);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction3);

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
  EXPECT_EQ(numActionsDestroyed, 2);
}

// Tests queueing actions at the AT_END position
TEST(QueueAction, QueueAtEnd)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
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
  EXPECT_EQ(numActionsDestroyed, 2);
}

//Tests queueing actions at the NOW_AND_CLEAR_REMAINING position
TEST(QueueAction, QueueNowAndClearRemaining)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  testAction3->_complete = true;
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_CLEAR_REMAINING, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing actions at the IN_PARALLEL position
TEST(QueueAction, QueueInParallel)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 3);
  
  testAction1->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 3);
  
  testAction2->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 3);
  
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 0);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 3);
  
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests queueing an action that has sub-actions (ie. action within an action)
TEST(QueueAction, QueueActionWithinAction)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestActionWithinAction* testActionWithinAction = new TestActionWithinAction("TestActionWithinAction", RobotActionType::UNKNOWN);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testActionWithinAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(testActionWithinAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  // Sets the subActions of testActionWithinAction to be complete so that testActionWithinAction can complete
  auto subActions = ((TestCompoundActionSequential*)testActionWithinAction->GetAction())->GetActions();
  for(auto action : subActions)
  {
    // Set during Init() when RegisterSubActions() is called
    EXPECT_EQ(((TestAction*)action.second)->GetRobot(), &r);
    ((TestAction*)action.second)->_complete = true;
  }
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_EQ(numActionsDestroyed, 4); //TestActionWithinAction has a compound action with 2 actions for a total of 4 actions
}

// Tests queueing a compound action where on of its actions has a FAILURE_RETRY
TEST(QueueAction, ActionFailureRetry)
{
  numActionsDestroyed = 0;
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2});
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction2->SetNumRetries(3);
  testAction2->_numRetries = 1;
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
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
  EXPECT_EQ(numActionsDestroyed, 3);
}

// Tests setting two unique tags
TEST(ActionTag, UniqueUnityTags)
{
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  u32 origTag2 = testAction2->GetTag();
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), testAction1->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), testAction2->GetUnityTag());
  
  testAction1->SetTag(20000);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), origTag1);
  EXPECT_EQ(testAction1->GetUnityTag(), 20000);
  
  testAction2->SetTag(20001);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), origTag2);
  EXPECT_EQ(testAction2->GetUnityTag(), 20001);
}

// Tests to make sure two unity tags can't be the same
TEST(ActionTag, ConflictingUnityTags)
{
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  u32 origTag2 = testAction2->GetTag();
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), testAction1->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), testAction2->GetUnityTag());
  
  testAction1->SetTag(100);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), origTag1);
  EXPECT_EQ(testAction1->GetUnityTag(), 100);
  
  testAction2->SetTag(100);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), origTag2);
  EXPECT_EQ(testAction2->GetUnityTag(), origTag2); // Unity tag will not change because of tag conflict
}

// Tests to make sure tags can't be set to already existing auto-generated tags
TEST(ActionTag, ConflictingNormalTags)
{
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  u32 origTag2 = testAction2->GetTag();
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), testAction1->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), testAction2->GetUnityTag());
  
  testAction1->SetTag(2);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction1->GetTag(), origTag1);
  EXPECT_EQ(testAction1->GetUnityTag(), origTag1); // Unity tag will not change because of tag conflict
  
  testAction2->SetTag(1);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
  EXPECT_EQ(testAction2->GetTag(), origTag2);
  EXPECT_EQ(testAction2->GetUnityTag(), origTag2); // Unity tag will not change because of tag conflict
}

// Tests when an auto-generated tag conflicts with an existing unity tag
TEST(ActionTag, ConflictAutoGeneratedTag)
{
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  EXPECT_EQ(testAction1->GetTag(), testAction1->GetUnityTag());
  
  testAction1->SetTag(origTag1+1);
  EXPECT_EQ(testAction1->GetTag(), origTag1);
  EXPECT_EQ(testAction1->GetUnityTag(), origTag1+1);
  
  // First auto-generated tag will conflict with the unity tag of testAction1 so testAction2's tag will be 2 greater
  // than testAction1's instead of 1 greater
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  u32 origTag2 = testAction2->GetTag();
  EXPECT_EQ(testAction2->GetTag(), origTag2);
  EXPECT_EQ(testAction2->GetTag(), testAction2->GetUnityTag());
  EXPECT_EQ(origTag2, origTag1+2);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_NE(testAction1->GetUnityTag(), testAction2->GetUnityTag());
}
