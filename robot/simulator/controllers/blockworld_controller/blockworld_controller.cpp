/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/common/basestation/platformPathManager.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/cozmo/basestation/basestation.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/basestation/multiClientComms.h"
#include "json/json.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include <fstream>



#define USE_WEBOTS_TIMESTEP 1
#if (USE_WEBOTS_TIMESTEP)
#include <webots/Supervisor.hpp>
webots::Supervisor basestationController;
#else
class BSTimer {
public:
  BSTimer() {_time = 0;}
  
  // TODO: This needs to wait until actual time has elapsed
  int step(int ms) {_time += ms; return 0;}
  int getTime() {return _time;}
private:
  int _time;
};
BSTimer basestationController;
#endif


// Set to 1 if you want to use BLE to communicate with robot.
// Set to 0 if you want to use TCP to communicate with robot.
#define USE_BLE_ROBOT_COMMS 0
#if (USE_BLE_ROBOT_COMMS)
#include "anki/cozmo/basestation/bleRobotManager.h"
#include "anki/cozmo/basestation/bleComms.h"

// Set this UUID to the desired robot you want to connect to
#define COZMO_BLE_UUID (0xbeefffff00010001)
#endif

#define ROBOT_ADVERTISING_HOST_IP "127.0.0.1"
#define VIZ_HOST_IP               "127.0.0.1"

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  // Start with a step so that we can attach to the process here for debugging
  basestationController.step(BS_TIME_STEP);
  
  // Get configuration JSON
  Json::Value config;
  const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, std::string(AnkiUtil::kP_CONFIG_JSON_FILE));
  
  Json::Reader reader;
  std::ifstream jsonFile(jsonFilename);
  reader.parse(jsonFile, config);
  jsonFile.close();
  
  if(!config.isMember("AdvertisingHostIP")) {
    config["AdvertisingHostIP"] = ROBOT_ADVERTISING_HOST_IP;
  }
  if(!config.isMember("VizHostIP")) {
    config["VizHostIP"] = VIZ_HOST_IP;
  }
  
  
  // Get basestation mode
  int bmInt;
  Json::Value bmValue = JsonTools::GetValueOptional(config, "basestation_mode", bmInt);
  BasestationMode bm = (BasestationMode)bmInt;
  assert(bm <= BM_PLAYBACK_SESSION);
  
  // Connect to robot and UI device.
  // UI-layer should handle this (whether the connection is TCP or BTLE).
  // Start basestation only when connections have been established.
  MultiClientComms uiComms(config["AdvertisingHostIP"].asCString(), UI_ADVERTISING_PORT);
  
  
#if (USE_BLE_ROBOT_COMMS)
  BLEComms robotComms;
  BLERobotManager robotBLEManager;
#else
  MultiClientComms robotComms(config["AdvertisingHostIP"].asCString(), ROBOT_ADVERTISING_PORT);
#endif
  if (bm != BM_PLAYBACK_SESSION) {

    
#if (USE_BLE_ROBOT_COMMS)
    basestationController.step(BS_TIME_STEP);
    robotBLEManager.DiscoverRobots();
#endif
    
    
    while (basestationController.step(BS_TIME_STEP) != -1) {
      
#if (USE_BLE_ROBOT_COMMS)
      // ====== BTLE ======
      robotBLEManager.Update();
      
      if (robotBLEManager.GetNumConnectedDevices() == 0) {
        // Get list of advertising robots and connect to one
        std::vector<u64> advertisingRobotMfgIDs;
        if (robotBLEManager.GetAdvertisingRobotMfgIDs(advertisingRobotMfgIDs) > 0) {
          
          printf("RobotBLEComms: %ld advertising robots found:", advertisingRobotMfgIDs.size());
          
          // Look for specific robot we're trying to connect to
          bool robotFound = false;
          int advertisingRobotIdx = 0;
          for (advertisingRobotIdx = 0; advertisingRobotIdx<advertisingRobotMfgIDs.size(); ++advertisingRobotIdx) {
            printf(" %llx ", advertisingRobotMfgIDs[advertisingRobotIdx]);
            if (advertisingRobotMfgIDs[advertisingRobotIdx] == COZMO_BLE_UUID) {
              robotFound = true;
              break;
            }
          }
          printf("\n");
          if (!robotFound) {
            continue;
          }

          u64 mfgID = advertisingRobotMfgIDs[advertisingRobotIdx];
          robotBLEManager.ConnectToRobotByMfgID(mfgID);
          
          // Add connected_robot ID to config
          int robotID = robotBLEManager.GetRobotIDFromMfgID(mfgID);
          config[AnkiUtil::kP_CONNECTED_ROBOTS].append(robotID);
          
          printf("RobotBLEComms: Connecting to robot %d (mfg ID=%llx)...\n", robotID, mfgID);
          
          robotBLEManager.StopDiscoveringRobots();
          break;
        }
      }
      
#else
      // ====== TCP ======
      robotComms.Update();
      
      // If not already connected to a robot, connect to the
      // first one that becomes available.
      // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
      if (robotComms.GetNumConnectedDevices() == 0) {
        std::vector<int> advertisingRobotIDs;
        if (robotComms.GetAdvertisingDeviceIDs(advertisingRobotIDs) > 0) {
          for(auto robotID : advertisingRobotIDs) {
            printf("RobotComms connecting to robot %d.\n", robotID);
            if (robotComms.ConnectToDeviceByID(robotID)) {
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

      // If not already connected to a UI device, connect to the
      // first one that becomes available.
      uiComms.Update();
      if (uiComms.GetNumConnectedDevices() == 0) {
        std::vector<int> advertisingUIDeviceIDs;
        if (uiComms.GetAdvertisingDeviceIDs(advertisingUIDeviceIDs) > 0) {
          for(auto uiID : advertisingUIDeviceIDs) {
            printf("UiComms connecting to UI device %d.\n", uiID);
            if (uiComms.ConnectToDeviceByID(uiID)) {
              printf("Connected to UI device %d\n", uiID);
              
              // Add connected ui device ID to config
              config[AnkiUtil::kP_CONNECTED_UI_DEVS].append(uiID);
              
              break;
            } else {
              printf("Failed to connect to UI device %d\n", uiID);
              return BS_END_INIT_ERROR;
            }
          }
        }
      }
      
      // Don't resume until at least one robot and one ui device are connected
      if ((robotComms.GetNumConnectedDevices() > 0) && (uiComms.GetNumConnectedDevices() > 0)) {
        break;
      }
#endif
      
    }
  } // if (bm != BM_PLAYBACK_SESSION)
  
  
#if (USE_BLE_ROBOT_COMMS)
  // Wait until robot is connected
  while (basestationController.step(BS_TIME_STEP) != -1) {
    robotBLEManager.Update();
    if (robotBLEManager.GetNumConnectedDevices() > 0) {
      printf("Connected to robot!\n");
      break;
    }
  }
#endif
  
  basestationController.step(BS_TIME_STEP);
  
  // We have connected robots. Start the basestation loop!
  BasestationMain bs;
  BasestationStatus status = bs.Init(&robotComms, &uiComms, config, bm);
  if (status != BS_OK) {
    PRINT_NAMED_ERROR("Basestation.Init.Fail","status %d\n", status);
    return -1;
  }
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
    // Read all UI messages
    uiComms.Update();
    
    // Read messages from all robots
#if (USE_BLE_ROBOT_COMMS)
    robotBLEManager.Update();  // Necessary?
#endif

    robotComms.Update();
    
    status = bs.Update(SEC_TO_NANOS(basestationController.getTime()));
    if (status != BS_OK) {
      PRINT_NAMED_WARNING("Basestation.Update.NotOK","status %d\n", status);
    }
    
    std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
    if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
      // TODO: stuff?
    }
  } // while still stepping
  
  return 0;
}

