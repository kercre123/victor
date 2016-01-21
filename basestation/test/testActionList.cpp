/**
 * File: testProgressionSystem
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
#include "anki/cozmo/basestation/actionContainers.h"
#include "anki/cozmo/basestation/actionInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include <limits.h>


using namespace Anki::Cozmo;

class TestAction : public IAction
{
  public:
    TestAction();
  
    virtual const std::string& GetName() const override { return name; }
    virtual RobotActionType GetType() const override { return RobotActionType::WAIT; }
    Robot* GetRobot() { return _robot; }
  protected:
    virtual ActionResult Init() override;
    virtual ActionResult CheckIfDone() override;
    std::string name = "Test";
};

TestAction::TestAction() {}

ActionResult TestAction::Init() { return ActionResult::SUCCESS; }

ActionResult TestAction::CheckIfDone() { return ActionResult::SUCCESS; }

TEST(QueueAction, QueueSingleAction)
{
  TestAction* testAction = new TestAction();
  
  EXPECT_EQ(testAction->GetRobot(), nullptr);
  
  Robot r(0, nullptr, nullptr, nullptr);
  r.GetActionList().QueueAction(0, QueueActionPosition::NOW, testAction);
  
  EXPECT_EQ(testAction->GetRobot(), &r);
  
  r.GetActionList().Update();
  
  EXPECT_TRUE(r.GetActionList().IsCurrAction("Test"));
}