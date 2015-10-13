#include "anki/common/constantsAndMacros.h"
#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include "util/logging/logging.h"
#include <set>
#include <vector>

#define private public
#define protected public

#include "anki/common/basestation/math/rotatedRect.h"
#include "anki/planning/basestation/xythetaEnvironment.h"
#include "anki/planning/basestation/xythetaPlanner.h"
#include "anki/planning/basestation/xythetaPlannerContext.h"
#include "util/jsonWriter/jsonWriter.h"
#include "json/json.h"

// hack just for unit testing, don't do this outside of tests
#include "xythetaPlanner_internal.h"


using namespace std;
using namespace Anki::Planning;

#ifndef TEST_DATA_PATH
#error No TEST_DATA_PATH defined. You may need to re-run cmake.
#endif
#define TEST_PRIM_FILE "/planning/matlab/test_mprim.json"


GTEST_TEST(TestPlanner, PlanOnceEmptyEnv)
{
  // Assuming this is running from root/build......
  
  xythetaPlannerContext context;

  // TEMP:  // TODO:(bn) read context params from new json, instead of loading a different file manually here

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));


  xythetaPlanner planner(context);

  State_c start(0.0, 1.0, 0.57);
  State_c goal(-10.0, 3.0, -1.5);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));
}

GTEST_TEST(TestPlanner, PlanAroundBox)
{
  // Assuming this is running from root/build......
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));


  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));

  xythetaPlanner planner(context);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));
}

GTEST_TEST(TestPlanner, PlanAroundBoxDumpAndImportContext)
{
  // Assuming this is running from root/build......
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));


  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));

  xythetaPlanner planner(context);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  std::stringstream jsonSS;
  Anki::Util::JsonWriter writer(jsonSS);
  context.Dump(writer);
  writer.Close();

  Json::Reader jsonReader;
  Json::Value config;
  ASSERT_TRUE( jsonReader.parse(jsonSS.str(), config) ) << "json parsing error";

  xythetaPlannerContext context2;
  EXPECT_TRUE(context2.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));
  ASSERT_TRUE( context2.Import( config ) );

  xythetaPlanner planner2(context2);

  ASSERT_TRUE(planner2.GoalIsValid());
  ASSERT_TRUE(planner2.StartIsValid());

  ASSERT_TRUE(planner2.Replan());
  EXPECT_TRUE(context2.env.PlanIsSafe(planner2.GetPlan(), 0));

  // now check that the plans match. I'm not doing floating point equality here, so they may be a bit
  // different, but not much
  
  float tol = planner.GetFinalCost() * 0.05f;
  if( tol < 0.1f ) {
    tol = 0.1f;
  }
  EXPECT_NEAR( planner.GetFinalCost(), planner2.GetFinalCost(), tol );

  ASSERT_EQ(planner.GetPlan().actions_.size(), planner2.GetPlan().actions_.size());
  for(int a=0; a < planner.GetPlan().actions_.size(); ++a) {
    ASSERT_EQ( planner.GetPlan().actions_[a], planner2.GetPlan().actions_[a] )
      << "action # "<<a<<" mismatches";
    ASSERT_NEAR( planner.GetPlan().penalties_[a], planner2.GetPlan().penalties_[a], tol )
      << "penalty # "<<a<<" mismatches";
  }

}

GTEST_TEST(TestPlanner, PlanAroundBox_soft)
{
  // Assuming this is running from root/build......
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));


  // first add it with a fatal cost (the default)
  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));

  xythetaPlanner planner(context);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  bool hasTurn = false;
  for(const auto& action : planner.GetPlan().actions_) {
    if(context.env.GetRawMotionPrimitive(0, action).endStateOffset.theta != 0) {
      hasTurn = true;
      break;
    }
  }
  ASSERT_TRUE(hasTurn) << "plan with fatal obstacle should turn";

  Cost fatalCost = planner.GetFinalCost();

  context.env.ClearObstacles();
  // now add it with a high cost
  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0), 50.0);

  context.forceReplanFromScratch = true;
  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  hasTurn = false;
  for(const auto& action : planner.GetPlan().actions_) {
    if(context.env.GetRawMotionPrimitive(0, action).endStateOffset.theta != 0) {
      hasTurn = true;
      break;
    }
  }
  ASSERT_TRUE(hasTurn) << "plan with high obstacle cost should turn";

  Cost highCost = planner.GetFinalCost();

  EXPECT_FLOAT_EQ(highCost, fatalCost) << "cost should be the same with fatal or high cost obstacle";

  context.env.ClearObstacles();
  // now add it with a very low cost
  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0), 1e-4);

  context.forceReplanFromScratch = true;
  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  // context.env.PrintPlan(planner.GetPlan());
  for(const auto& action : planner.GetPlan().actions_) {
    ASSERT_EQ(context.env.GetRawMotionPrimitive(0, action).endStateOffset.theta,0)
      <<"with low cost, should drive straight through obstacle, but plan has a turn!";
  }

  Cost lowCost = planner.GetFinalCost();

  EXPECT_LT(lowCost, highCost) << "should be cheaper to drive through obstacle than around it";

  context.env.ClearObstacles();
  // this time leave the world empty

  context.forceReplanFromScratch = true;
  ASSERT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  for(const auto& action : planner.GetPlan().actions_) {
    ASSERT_EQ(context.env.GetRawMotionPrimitive(0, action).endStateOffset.theta,0)
      <<"with no obstacle, should drive straight, but plan has a turn!";
  }

  Cost emptyCost = planner.GetFinalCost();
  
  EXPECT_LT(emptyCost, lowCost) << "no obstacle should be cheaper than any obstacle";
}


GTEST_TEST(TestPlanner, ReplanEasy)
{
  // Assuming this is running from root/build......
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  xythetaPlanner planner(context);

  State_c start(0, 0, 0);
  State_c goal(200, 0, 0);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  context.env.AddObstacle(Anki::RotatedRectangle(50.0, -100.0, 80.0, -100.0, 20.0));

  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0)) << "new obstacle should not interfere with plan";

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));
}


// some paremeter changes or something broke this test
GTEST_TEST(TestPlanner, ReplanHard)
{
  // Assuming this is running from root/build......
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  xythetaPlanner planner(context);

  State_c start(0, 0, 0);
  State_c goal(800, 0, 0);

  context.start = start;
  context.goal = goal;
  
  ASSERT_TRUE(planner.GoalIsValid());
  ASSERT_TRUE(planner.StartIsValid());

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  context.env.PrintPlan(planner.GetPlan());

  context.env.AddObstacle(Anki::RotatedRectangle(200.0, -10.0, 230.0, -10.0, 20.0));

  EXPECT_FALSE(context.env.PlanIsSafe(planner.GetPlan(), 0)) << "new obstacle should block plan!";

  State_c newRobotPos(137.9, -1.35, 0.0736);
  ASSERT_FALSE(context.env.IsInCollision(newRobotPos)) << "position "<<newRobotPos<<" should be safe";
  ASSERT_FALSE(context.env.IsInCollision(context.env.State_c2State(newRobotPos)));

  State_c lastSafeState;
  xythetaPlan oldPlan;

  float distFromPlan = 9999.0;
  int currentPlanIdx = static_cast<int>(context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), newRobotPos, distFromPlan));
  ASSERT_EQ(currentPlanIdx, 2)
    << "should be at action #2 in the plan (plan should start with 3 long actions) d="<<distFromPlan;

  EXPECT_LT(distFromPlan, 15.0) << "too far away from plan";

  ASSERT_FALSE(context.env.PlanIsSafe(planner.GetPlan(), 1000.0, currentPlanIdx, lastSafeState, oldPlan));

  std::cout<<"safe section of old plan:\n";
  context.env.PrintPlan(oldPlan);

  ASSERT_GE(oldPlan.Size(), 1) << "should re-use at least one action from the old plan";

  StateID currID = oldPlan.start_.GetStateID();
  for(const auto& action : oldPlan.actions_) {
    ASSERT_LT(context.env.ApplyAction(action, currID, false), 100.0) << "action penalty too high!";
  }

  ASSERT_EQ(currID, context.env.State_c2State(lastSafeState).GetStateID()) << "end of validOldPlan should match lastSafeState!";
  
  // replan from last safe state

  context.start = lastSafeState;

  ASSERT_TRUE(planner.StartIsValid());
  ASSERT_TRUE(planner.GoalIsValid()) << "goal should still be valid";

  EXPECT_TRUE(planner.Replan());
  EXPECT_TRUE(context.env.PlanIsSafe(planner.GetPlan(), 0));

  std::cout<<"final plan:\n";
  context.env.PrintPlan(planner.GetPlan());

}


GTEST_TEST(TestPlanner, ClosestSegmentToPose_straight)
{
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  xythetaPlanner planner(context);

  planner._impl->_plan.start_ = State(0, 0, 0);

  ASSERT_EQ(context.env.GetRawMotionPrimitive(0, 0).endStateOffset.x, 1) << "invalid action";
  ASSERT_EQ(context.env.GetRawMotionPrimitive(0, 0).endStateOffset.y, 0) << "invalid action";

  for(int i=0; i<10; ++i) {
    planner._impl->_plan.Push(0, 0.0);
  }

  // context.env.PrintPlan(planner._impl->_plan);

  // plan now goes form (0,0) to (10,0), any point in between should work

  for(float distAlong = 0.0f; distAlong < 12.0 * context.env.GetResolution_mm(); distAlong += 0.7356 * context.env.GetResolution_mm()) {
    size_t expected = (size_t)floor(distAlong / context.env.GetResolution_mm());
    if(expected >= 10)
      expected = 9;

    float distFromPlan = 9999.0;

    State_c pose(distAlong, 0.0, 0.0);
    ASSERT_EQ(context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), pose, distFromPlan), expected) 
      <<"closest path segment doesn't match expectation for state "<<pose;

    if(expected < 9) {
      EXPECT_LT(distFromPlan, 1.0) << "too far away from plan";
    }

    pose.y_mm = 7.36;
    ASSERT_EQ(context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), pose, distFromPlan), expected) 
      <<"closest path segment doesn't match expectation for state "<<pose;

    if(expected < 9) {
      EXPECT_LT(distFromPlan, 8.0) << "too far away from plan";
    }

    pose.y_mm = -0.3;
    ASSERT_EQ(context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), pose, distFromPlan), expected) 
      <<"closest path segment doesn't match expectation for state "<<pose;

    if(expected < 9) {
      EXPECT_LT(distFromPlan, 1.0) << "too far away from plan";
    }
  }
}



GTEST_TEST(TestPlanner, ClosestSegmentToPose_wiggle)
{
  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  xythetaPlanner planner(context);

  // bunch of random actions, no turn in place
  planner._impl->_plan.start_ = State(0, 0, 6);
  planner._impl->_plan.Push(0, 0.0);
  planner._impl->_plan.Push(2, 0.0);
  planner._impl->_plan.Push(2, 0.0);
  planner._impl->_plan.Push(0, 0.0);
  planner._impl->_plan.Push(1, 0.0);
  planner._impl->_plan.Push(2, 0.0);
  planner._impl->_plan.Push(2, 0.0);
  planner._impl->_plan.Push(0, 0.0);
  planner._impl->_plan.Push(1, 0.0);
  planner._impl->_plan.Push(3, 0.0);
  planner._impl->_plan.Push(3, 0.0);
  planner._impl->_plan.Push(0, 0.0);

  // go through each intermediate point, perturb it a bit, and make sure it returns correctly
  State curr = State(0,0,6);

  size_t planSize = planner._impl->_plan.Size();
  for(size_t planIdx = 0; planIdx < planSize; ++planIdx) {
    const MotionPrimitive& prim(context.env.GetRawMotionPrimitive(curr.theta, planner._impl->_plan.actions_[planIdx]));

    float distFromPlan = 9999.0;

    ASSERT_EQ(planIdx, context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), context.env.State2State_c(curr), distFromPlan))
      << "initial state wrong";
    EXPECT_LT(distFromPlan, 15.0) << "too far away from plan";

    ASSERT_FALSE(prim.intermediatePositions.empty());

    // check everything except the last one (which overlaps with the next segment)
    for(size_t intermediateIdx = 0; intermediateIdx < prim.intermediatePositions.size() - 1; intermediateIdx++) {
      State_c pose(prim.intermediatePositions[intermediateIdx].position.x_mm + context.env.GetX_mm(curr.x),
                   prim.intermediatePositions[intermediateIdx].position.y_mm + context.env.GetX_mm(curr.y),
                   prim.intermediatePositions[intermediateIdx].position.theta);

      ASSERT_EQ(planIdx, context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), pose, distFromPlan))
        << "exact intermediate state "<<intermediateIdx<<" wrong";

      EXPECT_LT(distFromPlan, 15.0) << "too far away from plan";

      pose.x_mm += 0.003;
      pose.y_mm -= 0.006;
      ASSERT_EQ(planIdx, context.env.FindClosestPlanSegmentToPose(planner.GetPlan(), pose, distFromPlan))
        << "offset intermediate state "<<intermediateIdx<<" wrong";
      EXPECT_LT(distFromPlan, 15.0) << "too far away from plan";

    }

    StateID currID(curr);
    context.env.ApplyAction(planner._impl->_plan.actions_[planIdx], currID, false);
    curr = State(currID);
  }
}


GTEST_TEST(TestPlanner, CorrectlyRoundStateNearBox)
{

  xythetaPlannerContext context;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(context.env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  xythetaPlanner planner(context);

  context.env.AddObstacle(Anki::RotatedRectangle(200.0, -10.0, 230.0, -10.0, 20.0));

  context.start = State_c(0, 0, 0.0);
  EXPECT_TRUE(planner.StartIsValid()) << "set start at origin should pass";

  context.start = State_c(210.0, -1.7, 2.34);
  EXPECT_FALSE(planner.StartIsValid()) << "set start in box should fail";

  context.start = State_c(198.7, 0, 0.0);
  EXPECT_TRUE(planner.StartIsValid()) << "set start near box should pass";
  
}
