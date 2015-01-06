/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/basestation/cozmoEngine.h"

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
  
  CozmoEngineHost cozmoEngine;
  
  cozmoEngine.Init(config);
  
  // Get basestation mode
  int bmInt;
  Json::Value bmValue = JsonTools::GetValueOptional(config, "basestation_mode", bmInt);
  BasestationMode bm = (BasestationMode)bmInt;
  assert(bm <= BM_PLAYBACK_SESSION);
  
  if (bm != BM_PLAYBACK_SESSION) {
    
    // Wait for at least one robot and UI device to connect
    bool isRobotConnected = false;
    bool isUiDeviceConnected = false;

    while (basestationController.step(BS_TIME_STEP) != -1 &&
           (!isRobotConnected || !isUiDeviceConnected))
    {
      std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
      cozmoEngine.GetAdvertisingRobots(advertisingRobots);
      for(auto robot : advertisingRobots) {
        if(cozmoEngine.ConnectToRobot(robot)) {
          isRobotConnected = true;
        }
      }
      
      std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
      cozmoEngine.GetAdvertisingUiDevices(advertisingUiDevices);
      for(auto device : advertisingUiDevices) {
        if(cozmoEngine.ConnectToUiDevice(device)) {
          isUiDeviceConnected = true;
        }
      }
    }
    
  } // if (bm != BM_PLAYBACK_SESSION)
  
  basestationController.step(BS_TIME_STEP);
  
  // We have connected robots. Start the basestation loop!
  Result status = cozmoEngine.StartBasestation();
  //  BasestationStatus status = bs.Init(&robotComms, &uiComms, config, bm);
  if (status != RESULT_OK) {
    PRINT_NAMED_ERROR("CozmoEngine.StartupFail","Status %d\n", status);
    return -1;
  }
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
    status = cozmoEngine.Update(SEC_TO_NANOS(basestationController.getTime()));
    if (status != RESULT_OK) {
      PRINT_NAMED_WARNING("CozmoEngine.Update.NotOK","Status %d\n", status);
    }
    
    /*
    std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
    if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
      // TODO: stuff?
    }
     */
  } // while still stepping
  
  return 0;
}

