/**
 * File: testVariableSnapshotComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 6/19/18
 *
 * Description: Various tests for variable snapshot component
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

#include "clad/types/variableSnapshotIds.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;

const std::string emptyJson = R"json({})json";
const int kRobotId = 0;

// pass in a json string and this function will write it to the file used by the system
// if an empty string is passed in, the function will delete the json file
void InitializeTests()
{
  using namespace Anki::Cozmo;
  // VariableSnapshotComponent::kVariableSnapshotFilename = "unitTestFile";
};

// removes all test information from storage
void RemoveTestData()
{
  using namespace Anki::Cozmo;
  // VariableSnapshotComponent::kVariableSnapshotFilename = "unitTestFile";

  // cache the name of our save directory
  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);
  auto platform = robot->GetContextDataPlatform();

   // read in our data
  std::string pathToVariableSnapshotFile = VariableSnapshotComponent::GetSavePath(platform,
                                                                                  VariableSnapshotComponent::kVariableSnapshotFolder,
                                                                                  VariableSnapshotComponent::kVariableSnapshotFilename);

  Json::Value emptyJson;
  platform->writeAsJson(Anki::Util::Data::Scope::Persistent, pathToVariableSnapshotFile, emptyJson);
};

// tests that the save functionality works when the robot is shut down (i.e. destructed)
TEST(VariableSnapshotComponent, SaveOnShutdown)
{
  RemoveTestData();
  InitializeTests();

  using namespace Anki::Cozmo;

  bool initBool0 = false;

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(initBool0);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr0);

    // the robot now shuts down and automatically saves the data
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(!initBool0);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr1);

    // check that the data is the same
    EXPECT_EQ(*testBoolPtr1, initBool0);

    // the robot now automatically saves data as it destructs
  }
};

// tests that data persists when changed
TEST(VariableSnapshotComponent, BasicFunctionalityTest)
{

  using namespace Anki::Cozmo;
  RemoveTestData();
  InitializeTests();

  int initInt0 = 20;

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

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
    RemoveTestData();
  }
};

// confirm that multiple initializations fail (for now - this behavior should be added later)
TEST(VariableSnapshotComponent, MultipleInitsFail)
{

  using namespace Anki::Cozmo;
  RemoveTestData();
  InitializeTests();

  int initInt0 = 20;
  auto errGState = Anki::Util::_errG;

  // make a robot
  auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

  // get and load data
  auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

  // identify data to be stored
  std::shared_ptr<int> testIntPtr0 = std::make_shared<int>(initInt0);
  std::shared_ptr<int> testIntPtr1 = std::make_shared<int>(initInt0-1);

  variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, testIntPtr0);
  Anki::Util::_errG = false;
  variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, testIntPtr1);

  // should error here
  EXPECT_TRUE( Anki::Util::_errG );

  // set _errG back to its initial value
  Anki::Util::_errG = errGState;
};

// changing version info leads to data reset
TEST(VariableSnapshotComponent, VersioningInfoDataResetOSBuildVersion)
{
  InitializeTests();

  using namespace Anki::Cozmo;

  bool initBool0 = false;
  std::string wrongVersion = "wrongVersion";

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

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
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(initBool0);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr0);

    variableSnapshotComp.SaveVariableSnapshots();
    RemoveTestData();
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& variableSnapshotComp = robot1->GetVariableSnapshotComponent();

    // identify data to be stored
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(!initBool0);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr1);

    // check that the data is the same
    EXPECT_TRUE(*testBoolPtr1);

    // the robot now automatically saves data as it destructs
  }
  RemoveTestData();
};

// changing version robot build sha leads to data reset
TEST(VariableSnapshotComponent, VersioningInfoDataResetRobotBuildSha)
{
  RemoveTestData();
  InitializeTests();

  using namespace Anki::Cozmo;

  bool initBool0 = false;
  std::string wrongSha = "wrongSha";

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

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
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(initBool0);

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
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(!initBool0);

    variableSnapshotComp.InitVariable<bool>(VariableSnapshotId::UnitTestBool0, testBoolPtr1);

    // check that the data is the same
    EXPECT_TRUE(*testBoolPtr1);

    // the robot now automatically saves data as it destructs
  }
  RemoveTestData();
};

// test that passing in a nullptr results in an error
TEST(VariableSnapshotComponent, NullPointerError)
{
  using namespace Anki::Cozmo;
  RemoveTestData();
  InitializeTests();

  auto errGState = Anki::Util::_errG;

  // make a robot
  auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

  // get and load data
  auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

  Anki::Util::_errG = false;
  variableSnapshotComp.InitVariable<int>(VariableSnapshotId::UnitTestInt0, nullptr);
  
  // should error here
  EXPECT_TRUE( Anki::Util::_errG );

  // set _errG back to its initial value
  Anki::Util::_errG = errGState;
};

