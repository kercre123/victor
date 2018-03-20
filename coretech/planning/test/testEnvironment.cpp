#include "util/helpers/includeGTest.h"
#include "util/logging/logging.h"

#include <set>
#include <vector>


#define private public
#define protected public

#include "coretech/common/engine/math/rotatedRect.h"
#include "coretech/planning/engine/xythetaEnvironment.h"
#include "json/json.h"
#include "util/helpers/quoteMacro.h"
#include "util/jsonWriter/jsonWriter.h"

using namespace std;
using namespace Anki::Planning;

#ifndef TEST_DATA_PATH
#error No TEST_DATA_PATH defined. You may need to re-run cmake.
#endif
#define TEST_PRIM_FILE "/planning/matlab/test_mprim.json"


GTEST_TEST(TestEnvironment, StateIDPacking)
{
  vector<GraphState> states;
  states.push_back(GraphState(0,0,0));
  states.push_back(GraphState(0,0,1));
  states.push_back(GraphState(0,0,15));
  states.push_back(GraphState(-34, 12, 7));
  states.push_back(GraphState(-1034, -221, 14));
  states.push_back(GraphState(1097, -208, 3));
  states.push_back(GraphState(1234, 4321, 4));

  for(size_t i=0; i<states.size(); ++i) {
    StateID id = states[i].GetStateID();
    EXPECT_EQ(id.s.x, states[i].x);
    EXPECT_EQ(id.s.y, states[i].y);
    EXPECT_EQ(id.s.theta, states[i].theta);
    GraphState newState(id);
    EXPECT_EQ(states[i], newState);
  }
}

GTEST_TEST(TestEnvironment, State2c)
{
  xythetaEnvironment env;

  // just read the prims so we have a valid environment (resolution, etc).
  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  ASSERT_FLOAT_EQ(1.0 / GraphState::resolution_mm_, GraphState::oneOverResolution_);
  ASSERT_FLOAT_EQ(1.0 / GraphState::radiansPerAngle_, GraphState::oneOverRadiansPerAngle_);

  GraphState s(0, 0, 0);
  State_c c = env.State2State_c(s);

  EXPECT_FLOAT_EQ(c.x_mm, 0.0);
  EXPECT_FLOAT_EQ(c.y_mm, 0.0);
  EXPECT_FLOAT_EQ(c.theta, 0.0);
  
  GraphState s2(c);
  EXPECT_EQ(s, s2);

  s.x = 234;
  s.y = 103;
  s.theta = 6;

  s2 = GraphState(env.State2State_c(s));
  EXPECT_EQ(s, s2);

}

GTEST_TEST(TestEnvironment, LoadPrimFile)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  ASSERT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  ASSERT_FALSE(env.allMotionPrimitives_.empty());
  for(size_t i=0; i<env.allMotionPrimitives_.size(); ++i) {
    ASSERT_FALSE(env.allMotionPrimitives_[i].empty());
  }
}

GTEST_TEST(TestEnvironment, DumpAndInit)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  // TODO:(bn) open something saved in the test dir isntead, so we
  // know not to change or remove it
  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  EXPECT_FALSE(env.allMotionPrimitives_.empty());
  for(size_t i=0; i<env.allMotionPrimitives_.size(); ++i) {
    EXPECT_FALSE(env.allMotionPrimitives_[i].empty());
  }

  env.AddObstacleAllThetas(Anki::RotatedRectangle(50.0, -10.0, 80.0, -10.0, 20.0));
  env.AddObstacleAllThetas(Anki::RotatedRectangle(200.0, -10.0, 230.0, -10.0, 20.0));

  ASSERT_EQ(env.GetNumObstacles(), 2);

  // uncomment to see the output in a file
  // Anki::Util::JsonWriter tmpWriter("/tmp/env.json");
  // env.Dump(tmpWriter);
  // tmpWriter.Close();

  std::stringstream jsonSS;
  Anki::Util::JsonWriter writer(jsonSS);
  env.Dump(writer);
  writer.Close();

  Json::Reader jsonReader;
  Json::Value config;
  ASSERT_TRUE( jsonReader.parse(jsonSS.str(), config) ) << "json parsing error";

  xythetaEnvironment env2;
  EXPECT_TRUE(env2.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));
  ASSERT_TRUE(env2.Import(config)) << "json import error";

  // check that the environments match
  ASSERT_EQ(env.GetNumObstacles(), env2.GetNumObstacles());
  ASSERT_FLOAT_EQ(env.GetResolution_mm(), env2.GetResolution_mm());
  ASSERT_EQ(env.allMotionPrimitives_.size(), env2.allMotionPrimitives_.size());
}


GTEST_TEST(TestEnvironment, SuccessorsFromZero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  GraphState curr(0,0,0);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done(env));
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.s.x, 1);
  EXPECT_EQ(it.Front().stateID.s.y, 0);
  EXPECT_EQ(it.Front().stateID.s.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.s.x, 5);
  EXPECT_EQ(it.Front().stateID.s.y, 0);
  EXPECT_EQ(it.Front().stateID.s.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.s.x, 5);
  EXPECT_EQ(it.Front().stateID.s.y, 1);
  EXPECT_EQ(it.Front().stateID.s.theta, 1);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.s.x, 5);
  EXPECT_EQ(it.Front().stateID.s.y, -1);
  EXPECT_EQ(it.Front().stateID.s.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.s.x, 0);
  EXPECT_EQ(it.Front().stateID.s.y, 0);
  EXPECT_EQ(it.Front().stateID.s.theta, 1);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.s.x, 0);
  EXPECT_EQ(it.Front().stateID.s.y, 0);
  EXPECT_EQ(it.Front().stateID.s.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.s.x, -1);
  EXPECT_EQ(it.Front().stateID.s.y, 0);
  EXPECT_EQ(it.Front().stateID.s.theta, 0);
  it.Next(env);

  EXPECT_TRUE(it.Done(env));
}

GTEST_TEST(TestEnvironment, SuccessorsFromNonzero)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  GraphState curr(-14,107,15);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0);

  EXPECT_FALSE(it.Done(env));
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 0);
  EXPECT_EQ(it.Front().stateID.s.x, -12);
  EXPECT_EQ(it.Front().stateID.s.y, 106);
  EXPECT_EQ(it.Front().stateID.s.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 1);
  EXPECT_EQ(it.Front().stateID.s.x, -10);
  EXPECT_EQ(it.Front().stateID.s.y, 105);
  EXPECT_EQ(it.Front().stateID.s.theta, 15);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 2);
  EXPECT_EQ(it.Front().stateID.s.x, -11);
  EXPECT_EQ(it.Front().stateID.s.y, 106);
  EXPECT_EQ(it.Front().stateID.s.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 3);
  EXPECT_EQ(it.Front().stateID.s.x, -11);
  EXPECT_EQ(it.Front().stateID.s.y, 105);
  EXPECT_EQ(it.Front().stateID.s.theta, 14);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 4);
  EXPECT_EQ(it.Front().stateID.s.x, -14);
  EXPECT_EQ(it.Front().stateID.s.y, 107);
  EXPECT_EQ(it.Front().stateID.s.theta, 0);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 5);
  EXPECT_EQ(it.Front().stateID.s.x, -14);
  EXPECT_EQ(it.Front().stateID.s.y, 107);
  EXPECT_EQ(it.Front().stateID.s.theta, 14);
  it.Next(env);

  EXPECT_FALSE(it.Done(env));
  EXPECT_EQ(it.Front().actionID, 6);
  EXPECT_EQ(it.Front().stateID.s.x, -16);
  EXPECT_EQ(it.Front().stateID.s.y, 108);
  EXPECT_EQ(it.Front().stateID.s.theta, 15);
  it.Next(env);

  EXPECT_TRUE(it.Done(env));
}

GTEST_TEST(TestEnvironment, ReverseSuccessorsMatch)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  GraphState curr(-14,107,15);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 0.0, false);

  // now go through each one, then from there, apply the reverse successors and make sure they match up
  ASSERT_FALSE(it.Done(env));
  it.Next(env);

  int numActions = 0;

  while( ! it.Done(env) ) {
    numActions++;
    StateID nextID = it.Front().stateID;

    SuccessorIterator rIt = env.GetSuccessors(nextID, 0.0, true);
    ASSERT_FALSE(rIt.Done(env));
    rIt.Next(env);
    int numMatches = 0;

    while( ! rIt.Done(env) ) {
      if( rIt.Front().stateID == curr.GetStateID() ) {
        numMatches++;
        EXPECT_EQ(rIt.Front().actionID, it.Front().actionID);
        EXPECT_FLOAT_EQ(rIt.Front().g, it.Front().g);
        EXPECT_FLOAT_EQ(rIt.Front().penalty, it.Front().penalty);
      }
      rIt.Next(env);
    }

    EXPECT_EQ(numMatches, 1) << "going forward then backwards should have exactly 1 match";

    it.Next(env);
  }

  EXPECT_GT(numActions, 2);
}

GTEST_TEST(TestEnvironment, ReverseSuccessorsMatch_WithObstacle)
{
  // Assuming this is running from root/build......
  xythetaEnvironment env;

  EXPECT_TRUE(env.ReadMotionPrimitives((std::string(QUOTE(TEST_DATA_PATH)) + std::string(TEST_PRIM_FILE)).c_str()));

  env.AddObstacleAllThetas(Anki::RotatedRectangle(-20.0, 100.0, 20.0, 100.0, 20.0), 1.0f);

  env.PrepareForPlanning();

  State_c curr_c(-14,107,15);
  GraphState curr(curr_c);

  SuccessorIterator it = env.GetSuccessors(curr.GetStateID(), 5.0, false);

  // now go through each one, then from there, apply the reverse successors and make sure they match up
  ASSERT_FALSE(it.Done(env));
  it.Next(env);

  int numActions = 0;

  while( ! it.Done(env) ) {
    numActions++;
    StateID nextID = it.Front().stateID;

    SuccessorIterator rIt = env.GetSuccessors(nextID, 5.0, true);
    ASSERT_FALSE(rIt.Done(env));
    rIt.Next(env);
    int numMatches = 0;

    while( ! rIt.Done(env) ) {
      if( rIt.Front().stateID == curr.GetStateID() ) {
        numMatches++;
        EXPECT_EQ(rIt.Front().actionID, it.Front().actionID);
        EXPECT_FLOAT_EQ(rIt.Front().g, it.Front().g)
          << "g value incorrect " << ((int)it.Front().actionID) << ": "
          << env.GetActionType(it.Front().actionID).GetName();
        EXPECT_FLOAT_EQ(rIt.Front().penalty, it.Front().penalty);
        EXPECT_GT(rIt.Front().penalty, 0.0f)
          << "action penalty fail " << ((int)it.Front().actionID) << ": "
          << env.GetActionType(it.Front().actionID).GetName();
      }
      rIt.Next(env);
    }

    EXPECT_EQ(numMatches, 1) << "going forward then backwards should have exactly 1 match";

    it.Next(env);
  }

  EXPECT_GT(numActions, 2);
}

