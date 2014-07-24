/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <webots/Supervisor.hpp>

#include "anki/common/basestation/platformPathManager.h"
#include "anki/common/basestation/general.h"
#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/basestation/tcpComms.h"
#include "json/json.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include <fstream>

webots::Supervisor basestationController;

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  // Get configuration JSON
  Json::Value config;
  const std::string subPath(std::string("basestation/config") + std::string(AnkiUtil::kP_CONFIG_JSON_PATH));
  const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, subPath);
  
  Json::Reader reader;
  std::ifstream jsonFile(jsonFilename);
  reader.parse(jsonFile, config);
  jsonFile.close();
  
  
  // Get basestation mode
  BasestationMode bm;
  Json::Value bmValue = JsonTools::GetValueOptional(config, "basestation_mode", (u8&)bm);
  assert(bm <= BM_PLAYBACK_SESSION);
  

  // Connect to robot.
  // UI-layer should handle this (whether the connection is TCP or BTLE).
  // Start basestation only when connections have been established.
  TCPComms robotComms;
  if (bm != BM_PLAYBACK_SESSION) {
    
    // ====== TCP ======
    while (basestationController.step(BS_TIME_STEP) != -1) {
      robotComms.Update();
      
      // If not already connected to a robot, connect to the
      // first one that becomes available.
      // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
      if (robotComms.GetNumConnectedRobots() == 0) {
        std::vector<int> advertisingRobotIDs;
        if (robotComms.GetAdvertisingRobotIDs(advertisingRobotIDs) > 0) {
          for(auto robotID : advertisingRobotIDs) {
            printf("RobotComms connecting to robot %d.\n", robotID);
            if (robotComms.ConnectToRobotByID(robotID)) {
              printf("Connected to robot %d\n", robotID);
              
              // Add connected_robot ID to config
              config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);
              
              break;
            } else {
              printf("Failed to connect to robot %d\n", robotID);
              return BS_END_INIT_ERROR;
            }
          }
        }
      }
    
      if (robotComms.GetNumConnectedRobots() > 0) {
        break;
      }
    }
  } // if (bm != BM_PLAYBACK_SESSION)
  else {
    // HACK!!!
    // How do we properly determine what robots to instantiate when playing a log?
    // Just hard-coding robotID for now.

    config[AnkiUtil::kP_CONNECTED_ROBOTS].append(1);
  }
  
  
  // We have connected robots. Start the basestation loop!
  BasestationMain bs;
  BasestationStatus status = bs.Init(&robotComms, config, bm);
  if (status != BS_OK) {
    PRINT_NAMED_ERROR("Basestation.Init.Fail","status %d\n", status);
  }
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
    // Read messages from all robots
    robotComms.Update();
    
    status = bs.Update(SEC_TO_NANOS(basestationController.getTime()));
    if (status != BS_OK) {
      PRINT_NAMED_WARNING("Basestation.Update.NotOK","status %d\n", status);
    }
    
  } // while still stepping
  
  return 0;
}

