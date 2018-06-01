/**
 * File: testDataPersistence.cpp
 *
 * Author: Hamzah Khan
 * Created: 6/19/18
 *
 * Description: Various tests for data persistence component
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

// Access internals for tests
#define private public
#define protected public

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/components/persistence/persistentSerializationFactory.h"
#include "engine/components/persistence/dataPersistenceComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "gtest/gtest.h"

#include "clad/types/persistenceIds.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;

const std::string emptyJson = R"json({})json";

// TODO: write hardcoded json file
TEST(DataPersistenceComponent, SaveOnShutdown) {

  using namespace Anki::Cozmo;

  const int kRobotId = 0;

  int initInt0 = 20;
  bool initBool0 = false;

  // scope so that robot 1 is destroyed before robot 2 is made
  {
    // make a robot
    auto robot0 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& dataPersistenceComp = robot0->GetDataPersistenceComponent();

    // identify data to be stored
    std::shared_ptr<int> testIntPtr0 = std::make_shared<int>(initInt0);
    std::shared_ptr<bool> testBoolPtr0 = std::make_shared<bool>(initBool0);

    dataPersistenceComp.InitPersistentVariable<int>(PersistenceId::UnitTestInt0,
                                                    testIntPtr0,
                                                    PersistentSerializationFactory::SerializeInt,
                                                    PersistentSerializationFactory::DeserializeInt);
    dataPersistenceComp.InitPersistentVariable<bool>(PersistenceId::UnitTestBool0,
                                                     testBoolPtr0,
                                                     PersistentSerializationFactory::SerializeBool,
                                                     PersistentSerializationFactory::DeserializeBool);

    // save the data
    dataPersistenceComp.SavePersistentVariables();

    // the robot now shuts down
  }

  // make another robot
  {
    // make a robot
    auto robot1 = std::make_unique<Robot>(kRobotId, cozmoContext);

    // get and load data
    auto& dataPersistenceComp = robot1->GetDataPersistenceComponent();

    // identify data to be stored
    std::shared_ptr<int> testIntPtr1 = std::make_shared<int>(0);
    std::shared_ptr<bool> testBoolPtr1 = std::make_shared<bool>(true);

    dataPersistenceComp.InitPersistentVariable<int>(PersistenceId::UnitTestInt0,
                                                    testIntPtr1,
                                                    PersistentSerializationFactory::SerializeInt,
                                                    PersistentSerializationFactory::DeserializeInt);
    dataPersistenceComp.InitPersistentVariable<bool>(PersistenceId::UnitTestBool0,
                                                     testBoolPtr1,
                                                     PersistentSerializationFactory::SerializeBool,
                                                     PersistentSerializationFactory::DeserializeBool);

    // check that the data is the same
    EXPECT_EQ(*testIntPtr1, initInt0);
    EXPECT_EQ(*testBoolPtr1, initBool0);
    dataPersistenceComp.SavePersistentVariables();
  }


// TODO: List of tests to add
/* 
 * Does version update (SHA and OS) result in emptied data?
 * Reset data by id functionality test
 * DPC direct test (initialize DC object and alter directly)
 */
};