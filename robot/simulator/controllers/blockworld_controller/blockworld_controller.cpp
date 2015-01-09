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
#include "anki/cozmo/robot/cozmoConfig.h"
#include "json/json.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/cozmo/basestation/events/BaseStationEvent.h"

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


// Example game class built around a CozmoEngine
class CozmoGameHost : public IBaseStationEventListener
{
public:
  
  CozmoGameHost() :
  cozmoEngine_(nullptr)
  {
    // Register for all events of interest
    BSE_RobotConnect::Register( this );
  }
  ~CozmoGameHost()
  {
    // Unregister for all events
    BSE_RobotConnect::Unregister( this );
    
    if(cozmoEngine_) {
      delete cozmoEngine_;
    }
  }
  
  void Init(const Json::Value& config) {
    
    if (cozmoEngine_)
      return;

    // There is no "device" (e.g. phone) with a camera so we can just use a bogus camera calibration
    Vision::CameraCalibration bogusDeviceCamCalib;
    
    cozmoEngine_ = new CozmoEngineHost();
    cozmoEngine_->Init(config, bogusDeviceCamCalib);
  }
  
  void Update() {
    // Connect to any robots we see:
    std::vector<CozmoEngine::AdvertisingRobot> advertisingRobots;
    cozmoEngine_->GetAdvertisingRobots(advertisingRobots);
    for(auto robot : advertisingRobots) {
      cozmoEngine_->ConnectToRobot(robot);
    }
    
    // Connect to any UI devices we see:
    std::vector<CozmoEngine::AdvertisingUiDevice> advertisingUiDevices;
    cozmoEngine_->GetAdvertisingUiDevices(advertisingUiDevices);
    for(auto device : advertisingUiDevices) {
      cozmoEngine_->ConnectToUiDevice(device);
    }
    
    Result status = cozmoEngine_->Update(SEC_TO_NANOS(basestationController.getTime()));
    if (status != RESULT_OK) {
      PRINT_NAMED_WARNING("CozmoEngine.Update.NotOK","Status %d\n", status);
    }
    
    /*
     std::vector<BasestationMain::ObservedObjectBoundingBox> boundingQuads;
     if(true == bs.GetCurrentVisionMarkers(1, boundingQuads) ) {
     // TODO: stuff?
     }
     */
  }
  
  // Process raised events from CozmoEngine
  virtual void OnEventRaised( const IBaseStationEventInterface* event )
  {
    switch( event->GetEventType() ) {
        
      case BSETYPE_RobotConnect:
      {
        BSE_RobotConnect *rcEvent = (BSE_RobotConnect*)event;
        printf("CozmoGame: Robot %d connect: %s !!!!!!\n", rcEvent->robotID_, rcEvent->successful_ ? "SUCCESS" : "FAILED");
        break;
      }
      default:
        printf("CozmoGame: Received unknown event %d\n", event->GetEventType());
        break;
    }
  }
  
private:
  CozmoEngineHost *cozmoEngine_;
};


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
  
  // Get basestation mode
  BasestationMode bm = BM_DEFAULT;
  int bmInt;
  if(JsonTools::GetValueOptional(config, AnkiUtil::kP_BASESTATION_MODE, bmInt)) {
    BasestationMode bm = (BasestationMode)bmInt;
    assert(bm <= BM_PLAYBACK_SESSION);
  } 
  
  if (bm != BM_PLAYBACK_SESSION) {
    
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
    cozmoGame.Update();
  } // while still stepping
  
  return 0;
}

