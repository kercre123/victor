#include "gtest/gtest.h"
#include <set>
#include <vector>

#define private public
#define protected public

#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/common/basestation/platformPathManager.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"

using namespace std;
using namespace Anki::Planning;


GTEST_TEST(TestPlanner, PlanOnceEmptyEnv)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(
                PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/test_mprim.json").c_str()));


  xythetaPlanner planner(env);

  State_c start(0.0, 1.0, 0.57);
  State_c goal(-10.0, 3.0, -1.5);

  ASSERT_TRUE(planner.SetStart(start));
  ASSERT_TRUE(planner.SetGoal(goal));

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());
}

GTEST_TEST(TestPlanner, PlanAroundBox)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(
                PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/test_mprim.json").c_str()));


  env.AddObstacle(new Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));

  xythetaPlanner planner(env);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  ASSERT_TRUE(planner.SetStart(start));
  ASSERT_TRUE(planner.SetGoal(goal));

  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());
}

GTEST_TEST(TestPlanner, ReplanEasy)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(
                PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/test_mprim.json").c_str()));

  xythetaPlanner planner(env);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  ASSERT_TRUE(planner.SetStart(start));
  ASSERT_TRUE(planner.SetGoal(goal));

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());

  env.AddObstacle(new Anki::RotatedRectangle(50.0, -100.0, 80.0, -100.0, 20.0));

  EXPECT_TRUE(planner.PlanIsSafe()) << "new obstacle should not interfere with plan";

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());
}


GTEST_TEST(TestPlanner, ReplanHard)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(
                PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/test_mprim.json").c_str()));

  xythetaPlanner planner(env);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  ASSERT_TRUE(planner.SetStart(start));
  ASSERT_TRUE(planner.SetGoal(goal));

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());

  env.AddObstacle(new Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));

  EXPECT_FALSE(planner.PlanIsSafe()) << "new obstacle should block plan!";

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(planner.PlanIsSafe());
}


