/*
 * File:          blockworld_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "anki/cozmo/game/cozmoGame.h"

#include "anki/common/basestation/platformPathManager.h"
#include "anki/common/basestation/utils/logging/logging.h"
#include "anki/cozmo/shared/cozmoConfig.h"
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

/*
// Helper object to listen for advertising robots/devices and tell the game
// to connect to them. This is here since we don't need/have a UI to instruct
// the game to make these connections. (I.e., just connect to everything that
// advertises.)
class DeviceConnector : IBaseStationEventListener
{
public:
  DeviceConnector(CozmoGameHost* game)
  : _game(game)
  {
    BSE_RobotAvailable::Register(this);
    BSE_UiDeviceAvailable::Register(this);
  }
  
  ~DeviceConnector()
  {
    BSE_RobotAvailable::Unregister(this);
    BSE_UiDeviceAvailable::Unregister(this);
  }
  
protected:
  
  CozmoGameHost* _game;
  
  virtual void OnEventRaised( const IBaseStationEventInterface* event )
  {
    switch(event->GetEventType())
    {
      case BSETYPE_RobotAvailable:
      {
        const BSE_RobotAvailable* raEvent = reinterpret_cast<const BSE_RobotAvailable*>(event);
        if(true == _game->ConnectToRobot(raEvent->robotID_)) {
          printf("Connected to robot %d!\n", raEvent->robotID_);
        }
        break;
      }
        
      case BSETYPE_UiDeviceAvailable:
      {
        const BSE_UiDeviceAvailable* daEvent = reinterpret_cast<const BSE_UiDeviceAvailable*>(event);
        if(true == _game->ConnectToUiDevice(daEvent->deviceID_)) {
          printf("Connected to UI device %d!\n", daEvent->deviceID_);
        }
        break;
      }
        
      default:
        break;
    }
  }
}; // class DeviceConnector
*/

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
  if(!config.isMember("VizHostIP")) {
    config["VizHostIP"] = VIZ_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = ROBOT_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  }
  
  // Get engine playback mode mode
  CozmoGameHost::PlaybackMode playbackMode = CozmoGameHost::LIVE_SESSION_NO_RECORD;
  int pmInt;
  if(JsonTools::GetValueOptional(config, AnkiUtil::kP_ENGINE_PLAYBACK_MODE, pmInt)) {
    playbackMode = (CozmoGameHost::PlaybackMode)pmInt;
    assert(playbackMode <= CozmoGameHost::PLAYBACK_SESSION);
  } 
  
  if (playbackMode != CozmoGameHost::PLAYBACK_SESSION) {
    
    // Wait for at least one robot and UI device to connect
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 1;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
    
  } else {
    
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 0;
    
  } // if (bm != BM_PLAYBACK_SESSION)
  
  // Initialize the engine
  CozmoGameHost cozmoGame;
  cozmoGame.Init(config);

  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
    cozmoGame.Update(basestationController.getTime());
  } // while still stepping
  
  return 0;
}

