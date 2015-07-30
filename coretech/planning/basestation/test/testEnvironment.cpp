#include "util/helpers/includeGTest.h"
#include <set>
#include <vector>

#define private public
#define protected public

#include "anki/common/basestation/platformPathManager.h"
#include "anki/planning/basestation/xythetaEnvironment.h"

using namespace std;
using namespace Anki::Planning;

#define TEST_PRIM_FILE "coretech/planning/matlab/test_mprim.json"

GTEST_TEST(TestEnvironment, StateIDPacking)
{
  vector<State> states;
  states.push_back(State(0,0,0));
  states.push_back(State(0,0,1));
  states.push_back(State(0,0,15));
  states.push_back(State(-34, 12, 7));
  states.push_back(State(-1034, -221, 14));
  states.push_back(State(1097, -208, 3));
  states.push_back(State(1234, 4321, 4));

  for(size_t i=0; i<states.size(); ++i) {
    StateID id = states[i].GetStateID();
    EXPECT_EQ(id.x, states[i].x);
    EXPECT_EQ(id.y, states[i].y);
    EXPECT_EQ(id.theta, states[i].theta);
    State newState(id);
    EXPECT_EQ(states[i], newState);
  }
}

GTEST_TEST(TestEnvironment, State2c)
{
  xythetaEnvironment env;

  // just read the prims so we havea  valid environment (resolution, etc).
  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, TEST_PRIM_FILE).c_str()));

  ASSERT_FLOAT_EQ(1.0 / env.resolution_mm_, env.oneOverResolution_);
  ASSERT_FLOAT_EQ(1.0 / env.radiansPerAngle_, env.oneOverRadiansPerAngle_);

  State s(0, 0, 0);
  State_c c = env.State2State_c(s);

  EXPECT_FLOAT_EQ(c.x_mm, 0.0);
  EXPECT_FLOAT_EQ(c.y_mm, 0.0);
  EXPECT_FLOAT_EQ(c.theta, 0.0);
  
  State s2 = env.State_c2State(c);
  EXPECT_EQ(s, s2);

  s.x = 234;
  s.y = 103;
  s.theta = 6;

  s2 = env.State_c2State(env.State2State_c(s));
  EXPECT_EQ(s, s2);

}

GTEST_TEST(TestEnvironment, LoadPrimFile)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, TEST_PRIM_FILE).c_str()));

  EXPECT_FALSE(env.allMotionPrimitives_.empty());
  for(size_t i=0; i<env.allMotionPrimitives_.size(); ++i) {
    EXPECT_FALSE(env.allMotionPrimitives_[i].empty());
  }
}


GTEST_TEST(TestEnvironment, SuccessorsFromZero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, TEST_PRIM_FILE).c_str()));

  State curr(0,0,0);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done(env));
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.x, 1);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.x, 5);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.x, 5);
  EXPECT_EQ(it.Front().stateID.y, 1);
  EXPECT_EQ(it.Front().stateID.theta, 1);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.x, 5);
  EXPECT_EQ(it.Front().stateID.y, -1);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.x, 0);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 1);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.x, 0);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.x, -1);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next(env);

  EXPECT_TRUE(it.Done(env));
}

GTEST_TEST(TestEnvironment, SuccessorsFromNonzero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, TEST_PRIM_FILE).c_str()));

  State curr(-14,107,15);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done(env));
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.x, -12);
  EXPECT_EQ(it.Front().stateID.y, 106);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.x, -10);
  EXPECT_EQ(it.Front().stateID.y, 105);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.x, -11);
  EXPECT_EQ(it.Front().stateID.y, 106);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.x, -11);
  EXPECT_EQ(it.Front().stateID.y, 105);
  EXPECT_EQ(it.Front().stateID.theta, 14);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.x, -14);
  EXPECT_EQ(it.Front().stateID.y, 107);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.x, -14);
  EXPECT_EQ(it.Front().stateID.y, 107);
  EXPECT_EQ(it.Front().stateID.theta, 14);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.x, -16);
  EXPECT_EQ(it.Front().stateID.y, 108);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next(env);

  EXPECT_TRUE(it.Done(env));
}
