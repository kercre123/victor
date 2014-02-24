#include "gtest/gtest.h"
#include <set>
#include <vector>

#define private public
#define protected public

#include "anki/common/basestation/general.h"
#include "anki/common/basestation/platformPathManager.h"
#include "anki/planning/basestation/xythetaEnvironment.h"

using namespace std;
using namespace Anki::Planning;


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

GTEST_TEST(TestEnvironment, LoadPrimFile)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/unicycle_backonlystraight_mprim.json").c_str()));

  EXPECT_FALSE(env.allMotionPrimitives_.empty());
  for(size_t i=0; i<env.allMotionPrimitives_.size(); ++i) {
    EXPECT_FALSE(env.allMotionPrimitives_[i].empty());
  }
}


GTEST_TEST(TestEnvironment, SuccessorsFromZero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/unicycle_backonlystraight_mprim.json").c_str()));

  State curr(0,0,0);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done());
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.x, 1);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.x, 8);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.x, -1);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.x, 8);
  EXPECT_EQ(it.Front().stateID.y, 1);
  EXPECT_EQ(it.Front().stateID.theta, 1);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.x, 8);
  EXPECT_EQ(it.Front().stateID.y, -1);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.x, 0);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 1);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.x, 0);
  EXPECT_EQ(it.Front().stateID.y, 0);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next();

  EXPECT_TRUE(it.Done());
}

GTEST_TEST(TestEnvironment, SuccessorsFromNonzero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives(PREPEND_SCOPED_PATH(Test, "coretech/planning/matlab/unicycle_backonlystraight_mprim.json").c_str()));

  State curr(-14,107,15);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done());
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.x, -12);
  EXPECT_EQ(it.Front().stateID.y, 106);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.x, -8);
  EXPECT_EQ(it.Front().stateID.y, 104);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.x, -16);
  EXPECT_EQ(it.Front().stateID.y, 108);
  EXPECT_EQ(it.Front().stateID.theta, 15);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.x, -9);
  EXPECT_EQ(it.Front().stateID.y, 103);
  EXPECT_EQ(it.Front().stateID.theta, 14);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.x, -7);
  EXPECT_EQ(it.Front().stateID.y, 105);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.x, -14);
  EXPECT_EQ(it.Front().stateID.y, 107);
  EXPECT_EQ(it.Front().stateID.theta, 14);
  it.Next();

  EXPECT_FALSE(it.Done());
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.x, -14);
  EXPECT_EQ(it.Front().stateID.y, 107);
  EXPECT_EQ(it.Front().stateID.theta, 0);
  it.Next();

  EXPECT_TRUE(it.Done());
}
