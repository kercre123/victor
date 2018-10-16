/**
 * File: testVariableSnapshotComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 6/19/18
 *
 * Description: Various tests for variable snapshot component
 * 
 * Note: All instances of robot that are created must be in their own block scope to avoid
 *       different instances of robots overwriting each others' saves.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// Access internals for tests
#define private public
#define protected public

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/components/variableSnapshot/variableSnapshotEncoder.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "gtest/gtest.h"
#include "test/engine/callWithoutError.h"

#include "clad/types/variableSnapshotIds.h"

extern Anki::Vector::CozmoContext* cozmoContext;

const std::string emptyJson = R"json({})json";
const int kRobotId = 0;

// removes all test information from storage before tests - ignores robot versioning information
void RemoveTestDataPrior(std::unique_ptr<Anki::Vector::Robot>& robot)
{
  using namespace Anki::Vector;

  std::string pathToVariableSnapshotFile;

  // clear data in json map
  robot->GetDataAccessorComponent()._variableSnapshotJsonMap->clear();
};

// removes all test information from storage before tests - ignores robot versioning information
void RemoveTestDataAfter()
{
  using namespace Anki::Vector;

  // cache the name of our save directory
  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);
  
  // clear data in data and json maps
  robot->GetDataAccessorComponent()._variableSnapshotJsonMap->clear();
  robot->GetVariableSnapshotComponent()._variableSnapshotDataMap.clear();
};

// tests that the save functionality works when the robot is shut down (i.e. destructed)
TEST(VariableSnapshotComponent, SaveOnShutdown)
{
  
  using namespace Anki::Vector;

  const bool kFalseBool = false;

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);
    RemoveTestDataPrior(robot0);

    // get and load data
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> defaultFalsePtr = std::make_shared<bool>(kFalseBool);

    const bool check1 = variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, defaultFalsePtr);
    EXPECT_FALSE( check1 );

    // the robot now shuts down and automatically saves the data
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> defaultTruePtr = std::make_shared<bool>(!kFalseBool);

    const bool check2 = variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, defaultTruePtr);
    EXPECT_TRUE( check2 );

    // check that the data is the same - since the data already exists, the false data should
    // have been loaded in and overwritten the default true value of the pointer
    EXPECT_EQ(*defaultTruePtr, kFalseBool);

    // the robot now automatically saves data as it destructs
  }

  RemoveTestDataAfter();
};

// tests that data persists when changed
TEST(VariableSnapshotComponent, BasicFunctionalityTest)
{

  using namespace Anki::Vector;

  int initInt0 = 20;

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);
    RemoveTestDataPrior(robot0);    

    // get and load data
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<int> testIntPtr0 = std::make_shared<int>(initInt0);

    variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, testIntPtr0);

    ++(*testIntPtr0);

    // save the data
    variableSnapshotComp.SaveVariableSnapshots();
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<int> testIntPtr1 = std::make_shared<int>(0);

    variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, testIntPtr1);

    // check that the data is the same
    EXPECT_EQ(*testIntPtr1, initInt0+1);
  }
  RemoveTestDataAfter();
};

// changing version info leads to data reset
TEST(VariableSnapshotComponent, VersioningInfoDataResetOSBuildVersion)
{
  using namespace Anki::Vector;

  bool falseBool = false;
  std::string wrongVersion = "wrongVersion";

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);
    RemoveTestDataPrior(robot0);

    // get and load data
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    // alter version number serialization
    auto wrongVersionSeralizeFn = [wrongVersion](Json::Value& outJson) { 
      outJson[VariableSnapshotEncoder::kVariableSnapshotKey] = wrongVersion;
      outJson[VariableSnapshotEncoder::kVariableSnapshotIdKey] = VariableSnapshotIdToString(VariableSnapshotId::_RobotOSBuildVersion);
      return true;
    };

    variableSnapshotComp._variableSnapshotDataMap.at(VariableSnapshotId::_RobotOSBuildVersion) = wrongVersionSeralizeFn;

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(falseBool);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr0);

    variableSnapshotComp.SaveVariableSnapshots();
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(!falseBool);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr1);

    // check that the data is the same
    EXPECT_TRUE(*testBoolPtr1);

    // the robot now automatically saves data as it destructs
  }
  RemoveTestDataAfter();
};

// changing version robot build sha leads to data reset
TEST(VariableSnapshotComponent, VersioningInfoDataResetRobotBuildSha)
{
  using namespace Anki::Vector;

  bool falseBool = false;
  std::string wrongSha = "wrongSha";

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);
    RemoveTestDataPrior(robot0);

    // get and load data
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    // alter version number serialization
    auto wrongShaSeralizeFn = [wrongSha](Json::Value& outJson) { 
      outJson[VariableSnapshotEncoder::kVariableSnapshotKey] = wrongSha;
      outJson[VariableSnapshotEncoder::kVariableSnapshotIdKey] = VariableSnapshotIdToString(VariableSnapshotId::_RobotBuildSha);
      return true;
    };

    variableSnapshotComp._variableSnapshotDataMap.at(VariableSnapshotId::_RobotBuildSha) = wrongShaSeralizeFn;

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(falseBool);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr0);

    variableSnapshotComp.SaveVariableSnapshots();
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(!falseBool);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr1);

    // check that the data is the same
    EXPECT_TRUE(*testBoolPtr1);

    // the robot now automatically saves data as it destructs
  }
  RemoveTestDataAfter();
};

// test that passing in a nullptr results in an error
TEST(VariableSnapshotComponent, NullPointerError)
{
  using namespace Anki::Vector;

  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    RemoveTestDataPrior(robot0);
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    const bool err = CallWithoutError( [&](){
      variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, nullptr);
    });
    EXPECT_TRUE( err );
  }

  RemoveTestDataAfter();
};

