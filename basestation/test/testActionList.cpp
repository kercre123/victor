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

#include "anki/common/types.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/actionWatcher.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "gtest/gtest.h"
#include "util/helpers/templateHelpers.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <vector>


using namespace Anki::Cozmo;

std::vector<std::string> actionsDestroyed;
uint8_t track1 = (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::LIFT_TRACK;
uint8_t track2 = (u8)AnimTrackFlag::BACKPACK_LIGHTS_TRACK | (u8)AnimTrackFlag::AUDIO_TRACK;
uint8_t track3 = (u8)AnimTrackFlag::FACE_IMAGE_TRACK | (u8)AnimTrackFlag::EVENT_TRACK;

// Simple action that can be set to complete
class TestAction : public IAction
{
  public:
    TestAction(Robot& robot, std::string name, RobotActionType type, u8 tracks = 0);
    virtual ~TestAction() { actionsDestroyed.push_back(GetName()); }
    int _numRetries = 0;
    bool _complete = false;
  protected:
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;
};

TestAction::TestAction(Robot& robot, std::string name, RobotActionType type, u8 tracks)
: IAction(robot,
          name,
          type,
          tracks)
{

}

ActionResult TestAction::Init() { return ActionResult::SUCCESS; }

ActionResult TestAction::CheckIfDone()
{
  if(_numRetries > 0)
  {
    _numRetries--;
    return ActionResult::RETRY;
  }
  return (_complete ? ActionResult::SUCCESS : ActionResult::RUNNING);
}

// Simple Interruptable action that can be set to complete
class TestInterruptAction : public IAction
{
public:
  TestInterruptAction(Robot& robot, std::string name, RobotActionType type, u8 tracks = 0);
  virtual ~TestInterruptAction() { actionsDestroyed.push_back(GetName()); }
  bool _complete;
protected:
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  virtual bool InterruptInternal() override { return true; }
};


TestInterruptAction::TestInterruptAction(Robot& robot, std::string name, RobotActionType type, u8 tracks)
: IAction(robot,
          name,
          type,
          tracks)
{
  _complete = false;
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
  TestCompoundActionSequential(Robot& robot, std::initializer_list<IActionRunner*> actions, std::string name);
  virtual ~TestCompoundActionSequential() { actionsDestroyed.push_back(_name); }
  virtual std::list<std::shared_ptr<IActionRunner>>& GetActions() { return _actions; }
private:
  std::string _name;
};

TestCompoundActionSequential::TestCompoundActionSequential(Robot& robot,
                                                           std::initializer_list<IActionRunner*> actions,
                                                           std::string name)
: CompoundActionSequential(robot, actions)
{
  _name = name;
  SetName(name);
}

// Simple parallel action with a function to expose its internal list of actions
class TestCompoundActionParallel : public CompoundActionParallel
{
public:
  TestCompoundActionParallel(Robot& robot, std::initializer_list<IActionRunner*> actions, std::string name);
  virtual ~TestCompoundActionParallel() { actionsDestroyed.push_back(GetName()); }
  virtual std::list<std::shared_ptr<IActionRunner>>& GetActions() { return _actions; }
private:
  std::string _name;
};

TestCompoundActionParallel::TestCompoundActionParallel(Robot& robot,
                                                       std::initializer_list<IActionRunner*> actions,
                                                       std::string name)
: CompoundActionParallel(robot, actions)
{
  _name = name;
  SetName(name);
}

// Simple Interruptable action that can be set to complete
class TestActionWithinAction : public IAction
{
public:
  TestActionWithinAction(Robot& robot, std::string name, RobotActionType type);
  virtual ~TestActionWithinAction() { actionsDestroyed.push_back(GetName()); }
  TestCompoundActionSequential* GetAction() { return &_compoundAction; }
  bool _complete;
protected:
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
  virtual bool InterruptInternal() override { return true; }
private:
  TestCompoundActionSequential _compoundAction;
};

TestActionWithinAction::TestActionWithinAction(Robot& robot, std::string name, RobotActionType type)
: IAction(robot,
          name,
          type,
          track1 | track2)
, _compoundAction(robot, {}, "Comp1")
{
  _complete = false;
}

ActionResult TestActionWithinAction::Init()
{
  _compoundAction.AddAction(new TestAction(_robot, "Test1", RobotActionType::WAIT, track1));
  _compoundAction.AddAction(new TestAction(_robot, "Test2", RobotActionType::WAIT, track2));
  _compoundAction.ShouldSuppressTrackLocking(true);
  
  _compoundAction.Update();
  
  return ActionResult::SUCCESS;
}

ActionResult TestActionWithinAction::CheckIfDone()
{
  bool result = true;
  for(auto& action : _compoundAction.GetActions())
  {
    result &= ((TestAction*)action.get())->_complete;
  }
  return (result ? ActionResult::SUCCESS : ActionResult::RUNNING);
}

// Simple action that can be set to complete
class TestActionThatCancels : public IAction
{
public:
  TestActionThatCancels(Robot& robot, std::string name, RobotActionType type, u8 tracks = 0);
  virtual ~TestActionThatCancels()
  {
    actionsDestroyed.push_back(GetName());
    if(_clearInsteadOfCancel)
    {
      _robot.GetActionList().Clear();
    }
    else
    {
      for(auto tag : _tagsToCancelOnDestroy)
      {
        _robot.GetActionList().Cancel(tag);
      }
    }
  }
  int _numRetries = 0;
  bool _complete = false;
  std::vector<u32> _tagsToCancelOnDestroy;
  bool _clearInsteadOfCancel = false;
protected:
  virtual ActionResult Init() override;
  virtual ActionResult CheckIfDone() override;
};

TestActionThatCancels::TestActionThatCancels(Robot& robot, std::string name, RobotActionType type, u8 tracks)
: IAction(robot,
          name,
          type,
          tracks)
{

}

ActionResult TestActionThatCancels::Init() { return ActionResult::SUCCESS; }

ActionResult TestActionThatCancels::CheckIfDone()
{
  if(_numRetries > 0)
  {
    _numRetries--;
    return ActionResult::RETRY;
  }
  return (_complete ? ActionResult::SUCCESS : ActionResult::RUNNING);
}


void CheckActionDestroyed(std::vector<std::string> actualNames)
{
  ASSERT_EQ(actionsDestroyed.size(), actualNames.size());
  
  for(int i = 0; i < actionsDestroyed.size(); i++)
  {
    EXPECT_TRUE(actionsDestroyed[i] == actualNames[i]);
  }
  actionsDestroyed.clear();
}

void CheckTracksLocked(Robot& r, u8 track)
{
  EXPECT_TRUE(r.GetMoveComponent().AreAllTracksLocked(track));
  EXPECT_FALSE(r.GetMoveComponent().AreAllTracksLocked(~track));
}

void CheckTracksUnlocked(Robot& r, u8 track)
{
  EXPECT_FALSE(r.GetMoveComponent().AreAllTracksLocked(track));
}

extern Anki::Cozmo::CozmoContext* cozmoContext;

// Tests queueing a single and letting it complete normally
TEST(QueueAction, SingleActionFinishes)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction(r, "Test", RobotActionType::WAIT, track1);
  
  EXPECT_EQ(&(testAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  CheckTracksLocked(r, track1);
  
  testAction->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test"});
  CheckTracksUnlocked(r, track1);
}

// Tests queueing a single action and cancelling it works
TEST(QueueAction, SingleActionCancel)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction(r, "Test", RobotActionType::WAIT, track1);
  
  EXPECT_EQ(&(testAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  r.GetActionList().Cancel(RobotActionType::WAIT);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test"));
  CheckActionDestroyed({"Test"});
  CheckTracksUnlocked(r, track1);
}

// Tests queueing three actions and letting them complete normally
TEST(QueueAction, ThreeActionsFinish)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(testAction3->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  
  r.GetActionList().Update();
  
  CheckTracksUnlocked(r, track1);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

// Tests queueing three actions, cancelling the second and letting the rest complete normally
TEST(QueueAction, ThreeActionsCancelSecond)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::TURN_IN_PLACE, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::TRACK_OBJECT, track3);
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(testAction3->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
  r.GetActionList().Cancel(RobotActionType::TURN_IN_PLACE);
  CheckTracksUnlocked(r, track2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test2", "Test1", "Test3"});
}

// Tests queueing three actions and cancelling all of them
TEST(QueueAction, ThreeActionsCancelAll)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(testAction3->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  r.GetActionList().Cancel(RobotActionType::WAIT);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckTracksUnlocked(r, track1);
  CheckTracksUnlocked(r, track2);
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}


// Tests queueing two actions where the first one, which hasn't started running, is interruptable so
// queueing the second action doesn't interrupt the first but instead adds it to the front of the queue.
TEST(QueueAction, InterruptActionNotRunning)
{
  Robot r(0, cozmoContext);
  TestAction* testAction = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestInterruptAction* testInterruptAction = new TestInterruptAction(r, "Test2", RobotActionType::TURN_IN_PLACE,
                                                                     track2);
  
  EXPECT_EQ(&(testAction->GetRobot()), &r);
  EXPECT_EQ(&(testInterruptAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testInterruptAction);
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_RESUME, testAction);
  
  // Interrupt test2 for test1 which is now current
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  // Test1 completes and Test2 is current
  testAction->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  // Test2 completes
  testInterruptAction->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
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
  TestAction* testAction = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestInterruptAction* testInterruptAction = new TestInterruptAction(r, "Test2", RobotActionType::TURN_IN_PLACE,
                                                                     track2);
  
  EXPECT_EQ(&(testAction->GetRobot()), &r);
  EXPECT_EQ(&(testInterruptAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testInterruptAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  // Move it out of queue to currentAction
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_RESUME, testAction);
  CheckTracksUnlocked(r, track2);
  
  EXPECT_EQ(&(testAction->GetRobot()), &r);
  EXPECT_EQ(&(testInterruptAction->GetRobot()), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testInterruptAction->_complete = true;
  testAction->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2"});
}

// Tests queueing a sequential compound action made of two actions and both actions complete at the same time
TEST(QueueAction, CompoundActionSequentialSingle)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential(r, {testAction1, testAction2}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2", "Comp1"});
}

// Tests queueing a sequential compound action made of two actions and they complete at different times
TEST(QueueAction, CompoundActionMultipleUpdates)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential(r, {testAction1, testAction2}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  CheckTracksLocked(r, track2);
  
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2", "Comp1"});
}

// Tests queueing a sequential compound action that initially has two actions and a third action is added to
// the compound action after an update
TEST(QueueAction, CompoundActionAddAfterQueued)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential(r, {testAction1, testAction2}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(testAction3->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);

  u32 tag1 = testAction1->GetTag();
  u32 tag2 = testAction2->GetTag();
  u32 tag3 = testAction3->GetTag();
  u32 tagCompound = compoundAction->GetTag();
  
  // Make sure the action watcher callacks are tracking everything
  std::set<u32> completedTags;
  auto& watcher = r.GetActionList().GetActionWatcher();
  watcher.RegisterActionEndedCallbackForAllActions(
    [&completedTags](const ExternalInterface::RobotCompletedAction& completion) {
      const bool inserted = completedTags.insert(completion.idTag).second;
      EXPECT_TRUE(inserted) << "set already contained tag " << completion.idTag;
    });
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  compoundAction->AddAction(testAction3);
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  CheckTracksUnlocked(r, track3);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2 | track3);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);

  EXPECT_TRUE(completedTags.find(tag1) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tag2) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tag3) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tagCompound) != completedTags.end());
  
  CheckActionDestroyed({"Test1", "Test2", "Test3", "Comp1"});
}

// Tests queueing an empty sequential compound action and adding three additional actions to it at various times
TEST(QueueAction, CompoundActionAddAfterQueued2)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential(r, {}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(testAction3->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);

  u32 tag1 = testAction1->GetTag();
  u32 tag2 = testAction2->GetTag();
  u32 tag3 = testAction3->GetTag();
  u32 tagCompound = compoundAction->GetTag();
  
  // Make sure the action watcher callacks are tracking everything
  std::set<u32> completedTags;
  auto& watcher = r.GetActionList().GetActionWatcher();
  watcher.RegisterActionEndedCallbackForAllActions(
    [&completedTags](const ExternalInterface::RobotCompletedAction& completion) {
      const bool inserted = completedTags.insert(completion.idTag).second;
      EXPECT_TRUE(inserted) << "set already contained tag " << completion.idTag;
    });
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  compoundAction->AddAction(testAction1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  compoundAction->AddAction(testAction2);
  compoundAction->AddAction(testAction3);
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  CheckTracksUnlocked(r, track3);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction3->_complete = true;
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2 | track3);
  
  r.GetActionList().Update();

  EXPECT_TRUE(completedTags.find(tag1) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tag2) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tag3) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tagCompound) != completedTags.end());
  
  CheckActionDestroyed({"Test1", "Test2", "Test3", "Comp1"});
}

// Tests queueing a parallel compound action with two actions that complete immediately
TEST(QueueAction, ParallelActionImmediateComplete)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel(r, {testAction1, testAction2}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);

  u32 tag1 = testAction1->GetTag();
  u32 tag2 = testAction2->GetTag();
  u32 tagCompound = compoundAction->GetTag();
  
  // Make sure the action watcher callacks are tracking everything
  std::set<u32> completedTags;
  auto& watcher = r.GetActionList().GetActionWatcher();
  watcher.RegisterActionEndedCallbackForAllActions(
    [&completedTags](const ExternalInterface::RobotCompletedAction& completion) {
      const bool inserted = completedTags.insert(completion.idTag).second;
      EXPECT_TRUE(inserted) << "set already contained tag " << completion.idTag;
    });
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1 | track2);
  
  testAction1->_complete = true;
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);

  EXPECT_TRUE(completedTags.find(tag1) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tag2) != completedTags.end());
  EXPECT_TRUE(completedTags.find(tagCompound) != completedTags.end());

  CheckActionDestroyed({"Test1", "Test2", "Comp1"});
}

// Tests queueing a parallel compound action with two actions where one completes before the other
TEST(QueueAction, ParallelActionOneCompleteBefore)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  testAction1->_complete = true;
  TestCompoundActionParallel* compoundAction = new TestCompoundActionParallel(r, {testAction1, testAction2}, "Comp1");
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2", "Comp1"});
}

// Tests queueing actions at the NOW position
TEST(QueueAction, QueueNow)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  CheckActionDestroyed({"Test3", "Test2", "Test1"});
}

// Tests queueing actions at the NEXT position
TEST(QueueAction, QueueNext)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction1);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction2);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);

  r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction3);

  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);

  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  CheckActionDestroyed({"Test1", "Test3", "Test2"});
}

// Tests queueing actions at the AT_END position
TEST(QueueAction, QueueAtEnd)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track2);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

//Tests queueing actions at the NOW_AND_CLEAR_REMAINING position
TEST(QueueAction, QueueNowAndClearRemaining)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_CLEAR_REMAINING, testAction3);
  CheckTracksUnlocked(r, track1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track3);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

// Tests queueing actions at the IN_PARALLEL position
TEST(QueueAction, QueueInParallel)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction1);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 1);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 2);
  
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction3);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(3), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 3);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1 | track2 | track3);
  
  testAction1->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 1);
  EXPECT_EQ(r.GetActionList().GetQueueLength(3), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 2);
  
  testAction2->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track2);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(3), 1);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 1);
  
  testAction3->_complete = true;
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track3);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test2"));
  EXPECT_FALSE(r.GetActionList().IsCurrAction("Test3"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(1), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(2), 0);
  EXPECT_EQ(r.GetActionList().GetQueueLength(3), 0);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 0);
  
  CheckActionDestroyed({"Test1", "Test2", "Test3"});
}

// Tests queueing an action that has sub-actions (ie. action within an action)
TEST(QueueAction, QueueActionWithinAction)
{
  Robot r(0, cozmoContext);
  TestActionWithinAction* testActionWithinAction = new TestActionWithinAction(r, "TestActionWithinAction", RobotActionType::UNKNOWN);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testActionWithinAction);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(&(testActionWithinAction->GetRobot()), &r);
  
  r.GetActionList().Update();
  CheckTracksLocked(r, track1);
  
  // Sets the subActions of testActionWithinAction to be complete so that testActionWithinAction can complete
  auto& subActions = testActionWithinAction->GetAction()->GetActions();
  for(auto& action : subActions)
  {
    // Set during Init() when RegisterSubActions() is called
    EXPECT_EQ(&(((TestAction*)action.get())->GetRobot()), &r);
    ((TestAction*)action.get())->_complete = true;
  }
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2);
  
  EXPECT_FALSE(r.GetActionList().IsCurrAction("TestActionWithinAction"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  // TestActionWithinAction has a compound action with 2 actions for a total of 4 actions
  // In this case the parent action TestActionWithinAction succeeds before the sub actions officially succeed
  // (TestActionWithinAction is checked if it is done before the sub actions) so it completes which destroys
  // the sub action and its actions, then itself
  CheckActionDestroyed({"TestActionWithinAction", "Comp1", "Test1", "Test2"});
}

// Tests queueing a compound action where on of its actions has a FAILURE_RETRY
TEST(QueueAction, ActionFailureRetry)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track2);
  TestCompoundActionSequential* compoundAction = new TestCompoundActionSequential(r, {testAction1, testAction2}, "Comp1");

  testAction1->_complete = true;
  testAction2->_complete = true;
  testAction2->SetNumRetries(3);
  testAction2->_numRetries = 2;
  
  EXPECT_EQ(&(testAction1->GetRobot()), &r);
  EXPECT_EQ(&(testAction2->GetRobot()), &r);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  EXPECT_EQ(&(compoundAction->GetRobot()), &r);
  
  r.GetActionList().Update();
  // testAction1 completed and unlocked its tracks and testAction2 had to retry so it also unlocked its tracks
  CheckTracksUnlocked(r, track1 | track2);
  
  // Both actions are set to complete but testAction2 is going to retry once
  // so it is still left
  auto& actions = compoundAction->GetActions();
  EXPECT_EQ(actions.size(), 1);
  EXPECT_EQ(actions.front()->GetName(), "Test2");
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  // Make sure the action that is being retried relocks its tracks
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1);
  CheckTracksLocked(r, track2);
  
  r.GetActionList().Update();
  CheckTracksUnlocked(r, track1 | track2);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 0);
  CheckActionDestroyed({"Test1", "Test2", "Comp1"});
}

// Tests queueing duplicate actions
TEST(QueueAction, QueueDuplicate)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction2);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::AT_END, testAction1), Anki::RESULT_FAIL);
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::NEXT, testAction1), Anki::RESULT_FAIL);
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction1), Anki::RESULT_FAIL);
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_RESUME, testAction1), Anki::RESULT_FAIL);
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::NOW_AND_CLEAR_REMAINING, testAction1),
            Anki::RESULT_FAIL);
  EXPECT_EQ(r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, testAction1), Anki::RESULT_FAIL);
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test1"));
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);
  EXPECT_EQ(r.GetActionList().GetNumQueues(), 1);
}

TEST(QueueAction, CancelFromDestructor)
{
  Robot r(0, cozmoContext);
  
  TestAction* action = new TestAction(r, "TestAction", RobotActionType::WAIT);
  TestAction* action2 = new TestAction(r, "TestAction2", RobotActionType::WAIT);
  TestActionThatCancels* actionThatCancels = new TestActionThatCancels(r, "TestActionThatCancels", RobotActionType::WAIT);
  actionThatCancels->_tagsToCancelOnDestroy.push_back(action->GetTag());
  actionThatCancels->_tagsToCancelOnDestroy.push_back(action2->GetTag());
  
  r.GetActionList().QueueAction(QueueActionPosition::NEXT, action);
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, actionThatCancels);
  r.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, action2);
  
  r.GetActionList().Clear();
  
  EXPECT_TRUE(r.GetActionList().IsEmpty());
}

TEST(QueueAction, CancelAlreadyCancelledFromDestructor)
{
  Robot r(0, cozmoContext);
  
  // Cancel by tag
  TestActionThatCancels* actionThatCancels = new TestActionThatCancels(r, "TestActionThatCancels", RobotActionType::WAIT);
  actionThatCancels->_tagsToCancelOnDestroy.push_back(actionThatCancels->GetTag());
  
  r.GetActionList().QueueAction(QueueActionPosition::NEXT, actionThatCancels);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Cancel(actionThatCancels->GetTag());
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsEmpty());
  
  // Cancel by type
  TestActionThatCancels* actionThatCancels2 = new TestActionThatCancels(r, "TestActionThatCancels", RobotActionType::WAIT);
  actionThatCancels2->_tagsToCancelOnDestroy.push_back(actionThatCancels2->GetTag());
  
  r.GetActionList().QueueAction(QueueActionPosition::NEXT, actionThatCancels2);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 1);
  
  r.GetActionList().Cancel(actionThatCancels2->GetType());
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsEmpty());
  
  // Cancel by clearing
  TestActionThatCancels* actionThatCancels3 = new TestActionThatCancels(r, "TestActionThatCancels", RobotActionType::WAIT);
  actionThatCancels3->_tagsToCancelOnDestroy.push_back(actionThatCancels3->GetTag());
  actionThatCancels3->_clearInsteadOfCancel = true;
  TestAction* action = new TestAction(r, "Test1", RobotActionType::WAIT);
  
  r.GetActionList().QueueAction(QueueActionPosition::NEXT, actionThatCancels3);
  r.GetActionList().QueueAction(QueueActionPosition::NEXT, action);
  
  EXPECT_EQ(r.GetActionList().GetQueueLength(0), 2);

  r.GetActionList().Clear();
  
  EXPECT_TRUE(r.GetActionList().IsEmpty());
}


TEST(QueueAction, ComplexNestedAction)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT, track1);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT, track1);
  TestAction* testAction3 = new TestAction(r, "Test3", RobotActionType::WAIT, track3);
  TestAction* testAction4 = new TestAction(r, "Test4", RobotActionType::WAIT, track1);
  TestAction* testAction5 = new TestAction(r, "Test5", RobotActionType::WAIT, track2);
  TestCompoundActionSequential* compoundAction1 = new TestCompoundActionSequential(r, {}, "Comp1");
  TestCompoundActionSequential* compoundAction2 = new TestCompoundActionSequential(r, {}, "Comp2");
  TestCompoundActionParallel* compoundAction3 = new TestCompoundActionParallel(r, {}, "Comp3");
  TestCompoundActionParallel* compoundAction4 = new TestCompoundActionParallel(r, {}, "Comp4");

  compoundAction4->AddAction(testAction5);
  compoundAction2->AddAction(testAction4);
  compoundAction2->AddAction(compoundAction4);
  compoundAction2->AddAction(testAction3);
  compoundAction3->AddAction(compoundAction2);
  compoundAction3->AddAction(testAction2);
  compoundAction1->AddAction(testAction1);
  compoundAction1->AddAction(compoundAction3);
  
  std::set<u32> allTags = {testAction1->GetTag(),
                           testAction2->GetTag(),
                           testAction3->GetTag(),
                           testAction4->GetTag(),
                           testAction5->GetTag(),
                           compoundAction1->GetTag(),
                           compoundAction2->GetTag(),
                           compoundAction3->GetTag(),
                           compoundAction4->GetTag()};
  std::set<u32> completedTags;
  auto& watcher = r.GetActionList().GetActionWatcher();
  watcher.RegisterActionEndedCallbackForAllActions(
    [&completedTags](const ExternalInterface::RobotCompletedAction& completion) {
      const bool inserted = completedTags.insert(completion.idTag).second;
      EXPECT_TRUE(inserted) << "set already contained tag " << completion.idTag;
    });

  r.GetActionList().QueueAction(QueueActionPosition::AT_END, compoundAction1);

  r.GetActionList().Update();

  testAction1->_complete = true;
  r.GetActionList().Update();

  testAction2->_complete = true;
  r.GetActionList().Update();

  testAction3->_complete = true;
  r.GetActionList().Update();

  testAction4->_complete = true;
  testAction5->_complete = true;
  r.GetActionList().Update();

  CheckTracksUnlocked(r, track1 | track2 | track3);

  EXPECT_EQ(allTags, completedTags);

  // TODO: should test this, but I'm too lazy to figure out which order they are being destroyed in
  // CheckActionDestroyed({"Test1", "Test2", "Test3", "Test4", "Test5", "Comp1", "Comp2", "Comp3", "Comp4"});

}


// Tests setting two unique tags
TEST(ActionTag, UniqueUnityTags)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  
  testAction1->SetTag(20000);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction1->GetTag(), 20000);
  
  testAction2->SetTag(20001);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction2->GetTag(), 20001);
  
  delete testAction1;
  delete testAction2;
}

// Tests to make sure two set tags can't be the same
TEST(ActionTag, ConflictingUnityTags)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  u32 origTag2 = testAction2->GetTag();
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  
  EXPECT_TRUE(testAction1->SetTag(100));
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction1->GetTag(), 100);
  
  EXPECT_FALSE(testAction2->SetTag(100));
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction2->GetTag(), origTag2); // Tag will not change because of conflict
  
  delete testAction1;
  delete testAction2;
}

// Tests to make sure tags can't be set to already existing auto-generated tags
TEST(ActionTag, ConflictingNormalTags)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  u32 origTag2 = testAction2->GetTag();
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  
  EXPECT_FALSE(testAction1->SetTag(origTag2));
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction1->GetTag(), origTag1); // Tag will not change because of conflict
  
  EXPECT_FALSE(testAction2->SetTag(origTag1));
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  EXPECT_EQ(testAction2->GetTag(), origTag2); // Tag will not change because of conflict
  
  actionsDestroyed.clear();
  // Trying to queue an action with a bad tag should delete the action
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction1);
  r.GetActionList().QueueAction(QueueActionPosition::NOW, testAction2);
  CheckActionDestroyed({"Test1", "Test2"});
}

// Tests when an auto-generated tag conflicts with an existing set tag
TEST(ActionTag, ConflictAutoGeneratedTag)
{
  Robot r(0, cozmoContext);
  TestAction* testAction1 = new TestAction(r, "Test1", RobotActionType::WAIT);
  u32 origTag1 = testAction1->GetTag();
  
  EXPECT_TRUE(testAction1->SetTag(origTag1+1));
  
  EXPECT_EQ(testAction1->GetTag(), origTag1+1);
  
  // First auto-generated tag will conflict with the set tag of testAction1 so testAction2's tag will be 2 greater
  // than testAction1's instead of 1 greater
  TestAction* testAction2 = new TestAction(r, "Test2", RobotActionType::WAIT);
  u32 origTag2 = testAction2->GetTag();
  EXPECT_EQ(testAction2->GetTag(), origTag2);
  EXPECT_EQ(origTag2, origTag1+2);
  EXPECT_NE(testAction1->GetTag(), testAction2->GetTag());
  
  delete testAction1;
  delete testAction2;
}

TEST(ActionWatcher, AddAndRemoveCallbacks)
{
  Robot robot(0, cozmoContext);

  using ActionTag = uint32_t;
  
  struct CallbackResult {
    ActionTag tag;
    RobotActionType type;
    ActionResult result;
  };

  // map callback int id to result
  std::map< int, CallbackResult > callbackResults;

  ActionEndedCallback cb1 = [&callbackResults](const ExternalInterface::RobotCompletedAction& completion) {
    callbackResults[1] = {completion.idTag, completion.actionType, completion.result};
  };

  ActionEndedCallback cb2 = [&callbackResults](const ExternalInterface::RobotCompletedAction& completion) {
    callbackResults[2] = {completion.idTag, completion.actionType, completion.result};
  };

  auto& watcher = robot.GetActionList().GetActionWatcher();
  
  ActionEndedCallbackID cbID1 = watcher.RegisterActionEndedCallbackForAllActions( cb1 );

  robot.GetActionList().Update();
  robot.GetActionList().Update();

  EXPECT_TRUE( callbackResults.empty() );
  
  TestAction* testAction1 = new TestAction(robot, "Test1", RobotActionType::WAIT, track1);
  auto action1Tag = testAction1->GetTag();
  
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, testAction1);  
  robot.GetActionList().Update();

  EXPECT_TRUE( callbackResults.empty() );
  
  testAction1->_complete = true;

  EXPECT_TRUE( callbackResults.empty() );

  robot.GetActionList().Update();

  EXPECT_FALSE( callbackResults.empty() );
  EXPECT_FALSE( callbackResults.find(1) == callbackResults.end() );
  EXPECT_EQ( callbackResults.size(), 1 );
  EXPECT_EQ( callbackResults[1].tag, action1Tag );
  EXPECT_EQ( callbackResults[1].type, RobotActionType::WAIT );
  EXPECT_EQ( callbackResults[1].result, ActionResult::SUCCESS );

  // add another watcher callback
  ActionEndedCallbackID cbID2 = watcher.RegisterActionEndedCallbackForAllActions( cb2 );
  EXPECT_NE(cbID1, cbID2) << "ids should be unique";
  callbackResults.clear();

  TestAction* testAction2 = new TestAction(robot, "Test2", RobotActionType::WAIT, track1);
  auto action2Tag = testAction2->GetTag();
  
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, testAction2);
  robot.GetActionList().Update();
  testAction2->_complete = true;
  robot.GetActionList().Update();

  EXPECT_FALSE( callbackResults.empty() );
  EXPECT_FALSE( callbackResults.find(1) == callbackResults.end() );
  EXPECT_FALSE( callbackResults.find(2) == callbackResults.end() );
  EXPECT_EQ( callbackResults.size(), 2 );
  EXPECT_EQ( callbackResults[1].tag, action2Tag );
  EXPECT_EQ( callbackResults[1].type, RobotActionType::WAIT );
  EXPECT_EQ( callbackResults[1].result, ActionResult::SUCCESS );
  EXPECT_EQ( callbackResults[2].tag, action2Tag );
  EXPECT_EQ( callbackResults[2].type, RobotActionType::WAIT );
  EXPECT_EQ( callbackResults[2].result, ActionResult::SUCCESS );

  // Remove one callback
  EXPECT_TRUE( watcher.UnregisterCallback(cbID1) );
  EXPECT_FALSE( watcher.UnregisterCallback(cbID1) ) << "already removed";
  callbackResults.clear();

  TestAction* testAction3 = new TestAction(robot, "Test3", RobotActionType::WAIT, track1);
  auto action3Tag = testAction3->GetTag();
  
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, testAction3);
  robot.GetActionList().Update();
  testAction3->_complete = true;
  robot.GetActionList().Update();

  EXPECT_FALSE( callbackResults.empty() );
  EXPECT_TRUE( callbackResults.find(1) == callbackResults.end() );
  EXPECT_FALSE( callbackResults.find(2) == callbackResults.end() );
  EXPECT_EQ( callbackResults.size(), 1 );
  EXPECT_EQ( callbackResults[2].tag, action3Tag );
  EXPECT_EQ( callbackResults[2].type, RobotActionType::WAIT );
  EXPECT_EQ( callbackResults[2].result, ActionResult::SUCCESS );

  // Remove the final callback
  EXPECT_TRUE( watcher.UnregisterCallback(cbID2) );
  EXPECT_FALSE( watcher.UnregisterCallback(cbID2) ) << "already removed";
  callbackResults.clear();

  TestAction* testAction4 = new TestAction(robot, "Test4", RobotActionType::WAIT, track1);  
  robot.GetActionList().QueueAction(QueueActionPosition::NOW, testAction4);
  robot.GetActionList().Update();
  testAction4->_complete = true;
  robot.GetActionList().Update();

  EXPECT_TRUE( callbackResults.empty() );  
}
