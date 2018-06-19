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
  VariableSnapshotComponent::kVariableSnapshotFilename = "unitTestFile";

  // cache the name of our save directory
  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);
  auto platform = robot->GetContextDataPlatform();
  std::string saveFolder = platform->pathToResource( Anki::Util::Data::Scope::Persistent, VariableSnapshotComponent::kVariableSnapshotFolder );
  saveFolder = Anki::Util::FileUtils::AddTrailingFileSeparator( saveFolder );

  // make sure our folder structure exists
  if(Anki::Util::FileUtils::DirectoryDoesNotExist( saveFolder )) {
    Anki::Util::FileUtils::CreateDirectory( saveFolder, false, true );
    PRINT_CH_DEBUG( "DataLoader", "VariableSnapshot", "Creating variable snapshot directory: %s", saveFolder.c_str() );
  }
  
  // read in our data
  std::string pathToVariableSnapshotFile = ( saveFolder + VariableSnapshotComponent::kVariableSnapshotFilename + ".json" );
};

// removes all test information from storage
void RemoveTestData()
{
  using namespace Anki::Cozmo;
  VariableSnapshotComponent::kVariableSnapshotFilename = "unitTestFile";

  // cache the name of our save directory
  auto robot = std::make_unique<Robot>(kRobotId, cozmoContext);
  auto platform = robot->GetContextDataPlatform();
  std::string saveFolder = platform->pathToResource( Anki::Util::Data::Scope::Persistent, VariableSnapshotComponent::kVariableSnapshotFolder );
  saveFolder = Anki::Util::FileUtils::AddTrailingFileSeparator( saveFolder );

   // read in our data
  std::string pathToVariableSnapshotFile = ( saveFolder + VariableSnapshotComponent::kVariableSnapshotFilename + ".json" );

  Json::Value emptyJson;
  platform->writeAsJson(Anki::Util::Data::Scope::Persistent, pathToVariableSnapshotFile, emptyJson);
};

// tests that the save functionality works when the robot is shut down (i.e. destructed)
TEST(VariableSnapshotComponent, SaveOnShutdown)
{
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

    variableSnapshotComp.InitVariableSnapshot<bool>(VariableSnapshotId::UnitTestBool0,
                                                    testBoolPtr0,
                                                    VariableSnapshotEncoder::SerializeBool,
                                                    VariableSnapshotEncoder::DeserializeBool);

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

    variableSnapshotComp.InitVariableSnapshot<bool>(VariableSnapshotId::UnitTestBool0,
                                                    testBoolPtr1,
                                                    VariableSnapshotEncoder::SerializeBool,
                                                    VariableSnapshotEncoder::DeserializeBool);

    // check that the data is the same
    EXPECT_EQ(*testBoolPtr1, initBool0);

    // the robot now automatically saves data as it destructs
  }
  RemoveTestData();
};

// tests that data persists when changed
TEST(VariableSnapshotComponent, BasicFunctionalityTest)
{

  using namespace Anki::Cozmo;
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

    variableSnapshotComp.InitVariableSnapshot<int>(VariableSnapshotId::UnitTestInt0,
                                                   testIntPtr0,
                                                   VariableSnapshotEncoder::SerializeInt,
                                                   VariableSnapshotEncoder::DeserializeInt);

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

    variableSnapshotComp.InitVariableSnapshot<int>(VariableSnapshotId::UnitTestInt0,
                                                   testIntPtr1,
                                                   VariableSnapshotEncoder::SerializeInt,
                                                   VariableSnapshotEncoder::DeserializeInt);

    // check that the data is the same
    EXPECT_EQ(*testIntPtr1, initInt0+1);
    RemoveTestData();
  }
};

// confirm that multiple initializations fail (for now - this behavior should be added later)
TEST(VariableSnapshotComponent, MultipleInitsFail)
{

  using namespace Anki::Cozmo;
  InitializeTests();

  int initInt0 = 20;
  Anki::Util::_errG = false;

  // make a robot
  auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

  // get and load data
  auto& variableSnapshotComp = robot0->GetVariableSnapshotComponent();

  // identify data to be stored
  std::shared_ptr<int> testIntPtr0 = std::make_shared<int>(initInt0);
  std::shared_ptr<int> testIntPtr1 = std::make_shared<int>(initInt0-1);

  variableSnapshotComp.InitVariableSnapshot<int>(VariableSnapshotId::UnitTestInt0,
                                                 testIntPtr0,
                                                 VariableSnapshotEncoder::SerializeInt,
                                                 VariableSnapshotEncoder::DeserializeInt);
  Anki::Util::_errG = false;
  variableSnapshotComp.InitVariableSnapshot<int>(VariableSnapshotId::UnitTestInt0,
                                                 testIntPtr1,
                                                 VariableSnapshotEncoder::SerializeInt,
                                                 VariableSnapshotEncoder::DeserializeInt);

  // should error here
  EXPECT_TRUE( Anki::Util::_errG );

};
