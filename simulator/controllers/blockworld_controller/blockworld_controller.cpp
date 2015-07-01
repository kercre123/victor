/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/game/cozmoGame.h"

#include "anki/common/basestation/platformPathManager.h"
#include "anki/util/logging/logging.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "json/json.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include <fstream>


#ifdef USE_WEBOTS
#include <webots/Supervisor.hpp>
webots::Supervisor basestationController;
#else
#include <chrono>
#include <thread>
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

/*
// For connecting to physical robot
// TODO: Expose these to UI
const bool FORCE_ADD_ROBOT = false;
const bool FORCED_ROBOT_IS_SIM = false;
const u8 forcedRobotId = 1;
//const char* forcedRobotIP = "192.168.3.34";   // cozmo2
const char* forcedRobotIP = "172.31.1.1";     // cozmo3
*/

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
  
  if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
    config[AnkiUtil::kP_VIZ_HOST_IP] = VIZ_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = ROBOT_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_AS_HOST)) {
    config[AnkiUtil::kP_AS_HOST] = true;
  }
  
  // Get engine playback mode mode
  CozmoGame::PlaybackMode playbackMode = CozmoGame::LIVE_SESSION_NO_RECORD;
  int pmInt;
  if(JsonTools::GetValueOptional(config, AnkiUtil::kP_ENGINE_PLAYBACK_MODE, pmInt)) {
    playbackMode = (CozmoGame::PlaybackMode)pmInt;
    assert(playbackMode <= CozmoGame::PLAYBACK_SESSION);
  } 

  int numUIDevicesToWaitFor = 1;
#ifdef USE_WEBOTS
  webots::Field* numUIsField = basestationController.getSelf()->getField("numUIDevicesToWaitFor");
  if (numUIsField) {
    numUIDevicesToWaitFor = numUIsField->getSFInt32();
  } else {
    printf("blockworldController.main.MissingField: numUIDevicesToWaitFor not found in BlockworldComms\n");
  }
#endif
  
  if (playbackMode != CozmoGame::PLAYBACK_SESSION) {
    
    // Wait for at least one robot and UI device to connect
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 1;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = numUIDevicesToWaitFor;
    
  } else {
    
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 0;
    
  } // if (bm != BM_PLAYBACK_SESSION)
  
  // Initialize the engine
  CozmoGame cozmoGame;
  cozmoGame.Init(config);
  
  PRINT_STREAM_INFO("blockworld_controller.main", "CozmoGame created and initialized.");
  
  /*
  // TODO: Wait to receive this from UI (webots keyboard controller)
  cozmoGame.StartEngine(config);
  
  
  // TODO: Have UI (webots kb controller) send this
  // Force add a robot
  if (FORCE_ADD_ROBOT) {
    cozmoGame.ForceAddRobot(forcedRobotId, forcedRobotIP, FORCED_ROBOT_IS_SIM);
  }  
   */

  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
#ifndef USE_WEBOTS
    auto tick_start = std::chrono::system_clock::now();
#endif
    
    cozmoGame.Update(basestationController.getTime());
    
#ifndef USE_WEBOTS
    auto ms_left = std::chrono::milliseconds(BS_TIME_STEP) - (std::chrono::system_clock::now() - tick_start);
    if (ms_left < std::chrono::milliseconds(0)) {
      PRINT_STREAM_WARNING("EngineHeartbeat.overtime", "over by " << std::chrono::duration_cast<std::chrono::seconds>(-ms_left).count() << "ms");
    }
    std::this_thread::sleep_for(ms_left);
#endif
    
  } // while still stepping
  
  return 0;
}

