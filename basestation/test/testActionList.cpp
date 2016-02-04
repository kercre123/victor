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
#include <vector>


using namespace Anki::Cozmo;

std::vector<std::string> actionsDestroyed;

// Simple action that can be set to complete
class TestAction : public IAction
{
  public:
    TestAction(std::string name, RobotActionType type);
    virtual ~TestAction() { actionsDestroyed.push_back(_name); }
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
  virtual ~TestInterruptAction() { actionsDestroyed.push_back(_name); }
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
  TestCompoundActionSequential(std::initializer_list<IActionRunner*> actions, std::string name);
  virtual ~TestCompoundActionSequential() { actionsDestroyed.push_back(_name); }
  virtual std::list<std::pair<bool, IActionRunner*>> GetActions() { return _actions; }
private:
  std::string _name;
};

TestCompoundActionSequential::TestCompoundActionSequential(std::initializer_list<IActionRunner*> actions,
                                                           std::string name)
: CompoundActionSequential(actions)
{
  _name = name;
}


// Simple parallel action with a function to expose its internal list of actions
class TestCompoundActionParallel : public CompoundActionParallel
{
public:
  TestCompoundActionParallel(std::initializer_list<IActionRunner*> actions, std::string name);
  virtual ~TestCompoundActionParallel() { actionsDestroyed.push_back(_name); }
  virtual std::list<std::pair<bool, IActionRunner*>> GetActions() { return _actions; }
private:
  std::string name;
};

TestCompoundActionParallel::TestCompoundActionParallel(std::initializer_list<IActionRunner*> actions,
                                                       std::string name)
: CompoundActionParallel(actions)
{
  _name = name;
}

// Simple Interruptable action that can be set to complete
class TestActionWithinAction : public IAction
{
public:
  TestActionWithinAction(std::string name, RobotActionType type);
  virtual ~TestActionWithinAction() { actionsDestroyed.push_back(_name); }
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
  TestCompoundActionSequential* compound = new TestCompoundActionSequential({}, "Comp1");
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

void CheckActionDestroyed(std::vector<std::string> actualNames)
{
  ASSERT_TRUE(actionsDestroyed.size() == actualNames.size());
  for(int i = 0; i < actionsDestroyed.size(); i++)
  {
    PRINT_NAMED_INFO("", "%s %s", actionsDestroyed[i].c_str(), actualNames[i].c_str());
    EXPECT_TRUE(actionsDestroyed[i] == actualNames[i]);
  }
  actionsDestroyed.clear();
}


extern Anki::Cozmo::CozmoContext* cozmoContext;

// Tests queueing a single and letting it complete normally
TEST(QueueAction, SingleActionFinishes)
{
  Robot r(0, cozmoContext);
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
  CheckActionDestroyed({"Test"});
}

// Tets queueing a single action and cancelling it works
TEST(QueueAction, SingleActionCancel)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test", RobotActionType::WAIT);
  testAction->_complete = true;
  
  EXPECT_NE(testAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Cancel(0, RobotActionType::WAIT);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  CheckActionDestroyed({"Test"});
}

// Tests queueing three actions and letting them complete normally
TEST(QueueAction, ThreeActionsFinish)
{
  Robot r(0, cozmoContext);
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
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

// Tests queueing three actions, cancelling the second and letting the rest complete normally
TEST(QueueAction, ThreeActionsCancelSecond)
{
  Robot r(0, cozmoContext);
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
  
  r.GetActionList().Cancel(0, RobotActionType::TURN_IN_PLACE);
  
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
  CheckActionDestroyed({"Test2", "Test1", "Test3"});
}

// Tests queueing three actions and cancelling all of them
TEST(QueueAction, ThreeActionsCancelAll)
{
  Robot r(0, cozmoContext);
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
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}


// Tests queueing two actions where the first one, which hasn't started running, is interruptable so
// queueing the second action interrupts the first. The second action completes and the interrupted
// action resumes and completes.
TEST(QueueAction, InterruptActionNotRunning)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test1", RobotActionType::WAIT);
  TestInterruptAction* testInterruptAction = new TestInterruptAction("Test2", RobotActionType::TURN_IN_PLACE);
  
  EXPECT_NE(testAction->GetRobot(), &r);
  EXPECT_NE(testInterruptAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testInterruptAction);
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW_AND_RESUME, testAction);
  
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
  CheckActionDestroyed({"Test1", "Test2"});
}

// Tests queueing two actions where the first one ,which has started running, is interruptable so queueing
// the second action interrupts it. The second action completes and the interrupted action
// resumes and completes.
TEST(QueueAction, InterruptActionRunning)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction("Test1", RobotActionType::WAIT);
  TestInterruptAction* testInterruptAction = new TestInterruptAction("Test2", RobotActionType::TURN_IN_PLACE);
  
  EXPECT_NE(testAction->GetRobot(), &r);
  EXPECT_NE(testInterruptAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testInterruptAction);
  
  EXPECT_EQ(testInterruptAction->GetRobot(), &r);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  // Move it out of queue to currentAction
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW_AND_RESUME, testAction);
  
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
  CheckActionDestroyed({"Test1", "Test2"});
}

// Tests queueing a sequential compound action made of two actions and both actions complete at the same time
TEST(QueueAction, CompoundActionSequentialSingle)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2}, "Comp1");
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  // Can't check testAction1 and testAction2's GetRobot() as they are set internally during update()
  
  r.GetActionList().Update();
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Comp1", "Test1", "Test2"});
}

// Tests queueing a sequential compound action made of two actions and they complete at different times
TEST(QueueAction, CompoundActionMultipleUpdates)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2}, "Comp1");
  
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
  CheckActionDestroyed({"Comp1", "Test1", "Test2"});
}

// Tests queueing a sequential compound action that initially has two actions and a third action is added to
// the compound action after an update
TEST(QueueAction, CompoundActionAddAfterQueued)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2}, "Comp1");
  
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
  CheckActionDestroyed({"Comp1", "Test1", "Test2", "Test3"});
}

// Tests queueing an empty sequential compound action and adding three additional actions to it at various times
TEST(QueueAction, CompoundActionAddAfterQueued2)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({}, "Comp1");
  
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
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  r.GetActionList().Update();
  CheckActionDestroyed({"Comp1", "Test1", "Test2", "Test3"});
}

// Tests queueing a parallel compound action with two actions that complete immediately
TEST(QueueAction, ParallelActionImmediateComplete)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  testAction2->_complete = true;
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel({testAction1, testAction2}, "Comp1");
  
  EXPECT_NE(testAction1->GetRobot(), &r);
  EXPECT_NE(testAction2->GetRobot(), &r);
  EXPECT_NE(compoundAction->GetRobot(), &r);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(compoundAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Comp1", "Test1", "Test2"});
}

// Tests queueing a parallel compound action with two actions where one completes before the other
TEST(QueueAction, ParallelActionOneCompleteBefore)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel({testAction1, testAction2}, "Comp1");
  
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
  CheckActionDestroyed({"Comp1", "Test1", "Test2"});
}

// Tests queueing actions at the NOW position
TEST(QueueAction, QueueNow)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  r.GetActionList().Update();
  r.GetActionList().Update();
  r.GetActionList().Update();
  
  CheckActionDestroyed({"Test3", "Test2", "Test1"});
}

// Tests queueing actions at the NEXT position
TEST(QueueAction, QueueNext)
{
  Robot r(0, cozmoContext);
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
  
  testAction2->_complete = true;
  
  r.GetActionList().Update();
  
  CheckActionDestroyed({"Test1", "Test3", "Test2"});
}

// Tests queueing actions at the AT_END position
TEST(QueueAction, QueueAtEnd)
{
  Robot r(0, cozmoContext);
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
  
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

//Tests queueing actions at the NOW_AND_CLEAR_REMAINING position
TEST(QueueAction, QueueNowAndClearRemaining)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestAction* testAction3 = new TestAction("Test3", RobotActionType::WAIT);
  testAction3->_complete = true;
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW_AND_CLEAR_REMAINING, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

// Tests queueing an action that has sub-actions (ie. action within an action)
TEST(QueueAction, QueueActionWithinAction)
{
  Robot r(0, cozmoContext);
  TestActionWithinAction* testActionWithinAction = new TestActionWithinAction("TestActionWithinAction", RobotActionType::UNKNOWN);
  
  r.GetActionList().QueueAction(0, QueueActionPosition::AT_END, testActionWithinAction);
  
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
  //TestActionWithinAction has a compound action with 2 actions for a total of 4 actions
  CheckActionDestroyed({"Comp1", "Test1", "Test2", "TestActionWithinAction"});
}

TEST(QueueAction, ActionFailureRetry)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction("Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction("Test2", RobotActionType::WAIT);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential({testAction1, testAction2}, "Comp1");
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
  CheckActionDestroyed({"Comp1", "Test1", "Test2"});
}
