/*
 * File:          webotsCtrlGameEngine.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/cozmoAPI.h"
#include "util/logging/logging.h"
#include "util/logging/channelFilter.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "json/json.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "util/console/consoleSystem.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/sosLoggerProvider.h"
#include "util/logging/multiFormattedLoggerProvider.h"
#include "util/global/globalDefinitions.h"

#include "util/time/stopWatch.h"

#include <fstream>

#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/devLoggerProvider.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#endif

#ifndef NO_WEBOTS
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

//#ifndef RESOURCE_PATH
//#error RESOURCE_PATH not defined.
//#endif
//#ifndef TEMP_DATA_PATH
//#error TEMP_DATA_PATH not defined.
//#endif


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
#define SDK_ADVERTISING_HOST_IP   "127.0.0.1"
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
  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  
  // create platform.
  // Unfortunately, CozmoAPI does not properly receive a const DataPlatform, and that change
  // is too big of a change, since it involves changing down to the context, so create a non-const platform
  //const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0]);
  Anki::Util::Data::DataPlatform dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlGameEngine");
  
  
#if ANKI_DEV_CHEATS
  DevLoggingSystem::CreateInstance(dataPlatform.pathToResource(Util::Data::Scope::CurrentGameLog, ""), "mac");
#endif

  // - create and set logger
  Util::IFormattedLoggerProvider* sosLoggerProvider = new Util::SosLoggerProvider();
  Util::IFormattedLoggerProvider* printfLoggerProvider = new Util::PrintfLoggerProvider(Anki::Util::ILoggerProvider::LOG_LEVEL_WARN);
  Anki::Util::MultiFormattedLoggerProvider loggerProvider({
    sosLoggerProvider
    , printfLoggerProvider
#if ANKI_DEV_CHEATS
    , new DevLoggerProvider(DevLoggingSystem::GetInstance()->GetQueue(),
            Util::FileUtils::FullFilePath( {DevLoggingSystem::GetInstance()->GetDevLoggingBaseDirectory(), DevLoggingSystem::kPrintName} ))
#endif
  });
  loggerProvider.SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
  Anki::Util::gLoggerProvider = &loggerProvider;
  Anki::Util::sSetGlobal(DPHYS, "0xdeadffff00000001");
  
  // - console filter for logs
  if ( params.filterLog )
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();
    
    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/basestation/config/console_filter_config.json";
    if (!dataPlatform.readAsJson(Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      PRINT_NAMED_ERROR("webotsCtrlGameEngine.main.loadConsoleConfig", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
    }
    
    // initialize console filter for this platform
    const std::string& platformOS = dataPlatform.GetOSPlatformString();
    const Json::Value& consoleFilterConfigOnPlatform = consoleFilterConfig[platformOS];
    consoleFilter->Initialize(consoleFilterConfigOnPlatform);
    
    // set filter in the loggers
    std::shared_ptr<const IChannelFilter> filterPtr( consoleFilter );
    printfLoggerProvider->SetFilter(filterPtr);
    sosLoggerProvider->SetFilter(filterPtr);
    
    // also parse additional info for providers
    printfLoggerProvider->ParseLogLevelSettings(consoleFilterConfigOnPlatform);
    sosLoggerProvider->ParseLogLevelSettings(consoleFilterConfigOnPlatform);
  }
  else
  {
    PRINT_CH_INFO("LOG", "webotsCtrlGameEngine.main", "Console will not be filtered due to program args");
  }
  
  // Start with a step so that we can attach to the process here for debugging
  basestationController.step(BS_TIME_STEP);
  
  // Get configuration JSON
  Json::Value config;

  if (!dataPlatform.readAsJson(Util::Data::Scope::Resources,
                               "config/basestation/config/configuration.json", config)) {
    PRINT_NAMED_ERROR("webotsCtrlGameEngine.main.loadConfig", "Failed to parse Json file config/basestation/config/configuration.json");
  }

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

  if(!config.isMember(AnkiUtil::kP_SDK_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_SDK_ADVERTISING_HOST_IP] = SDK_ADVERTISING_HOST_IP;
  }
  
  if(!config.isMember(AnkiUtil::kP_SDK_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_SDK_ADVERTISING_PORT] = SDK_ADVERTISING_PORT;
  }
  
  if(!config.isMember(AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT)) {
    config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = SDK_ON_DEVICE_TCP_PORT;
  }
  
  int numUIDevicesToWaitFor = 1;
#ifndef NO_WEBOTS
  webots::Field* numUIsField = basestationController.getSelf()->getField("numUIDevicesToWaitFor");
  if (numUIsField) {
    numUIDevicesToWaitFor = numUIsField->getSFInt32();
  } else {
    PRINT_NAMED_INFO("webotsCtrlGameEngine.main.MissingField", "numUIDevicesToWaitFor not found in BlockworldComms");
  }
#endif
  
  
  config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
  config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
  config[AnkiUtil::kP_NUM_SDK_DEVICES_TO_WAIT_FOR] = 1;
  
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT("consoleVars.ini");
  NativeAnkiUtilConsoleLoadVars();
  
  // Initialize the API
  CozmoAPI myCozmo;
  myCozmo.Start(&dataPlatform, config);

  PRINT_NAMED_INFO("webotsCtrlGameEngine.main", "CozmoGame created and initialized.");
  
  /*
  // TODO: Wait to receive this from UI (webots keyboard controller)
  cozmoGame.StartEngine(config);
  
  
  // TODO: Have UI (webots kb controller) send this
  // Force add a robot
  if (FORCE_ADD_ROBOT) {
    cozmoGame.ForceAddRobot(forcedRobotId, forcedRobotIP, FORCED_ROBOT_IS_SIM);
  }  
   */

  Anki::Util::Time::StopWatch stopWatch("tick");

  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(BS_TIME_STEP) != -1)
  {
#ifdef NO_WEBOTS
    auto tick_start = std::chrono::system_clock::now();
#endif
    stopWatch.Start();

    myCozmo.Update(basestationController.getTime());

    double timeMS = stopWatch.Stop();

    if( timeMS >= BS_TIME_STEP ) {
      PRINT_NAMED_WARNING("EngineHeartbeat.Overtime", "Update took %f ms (tick heartbeat is %dms)", timeMS, BS_TIME_STEP);
    }
    else if( timeMS >= 0.85*BS_TIME_STEP) {
      PRINT_NAMED_INFO("EngineHeartbeat.SlowTick", "Update took %f ms (tick heartbeat is %dms)", timeMS, BS_TIME_STEP);
    }
    
#ifdef NO_WEBOTS
    auto ms_left = std::chrono::milliseconds(BS_TIME_STEP) - (std::chrono::system_clock::now() - tick_start);
    if (ms_left < std::chrono::milliseconds(0)) {
      PRINT_NAMED_WARNING("EngineHeartbeat.overtime", "over by " << std::chrono::duration_cast<std::chrono::seconds>(-ms_left).count() << "ms");
    }
    std::this_thread::sleep_for(ms_left);
#endif
    
  } // while still stepping
#if ANKI_DEV_CHEATS
  DevLoggingSystem::DestroyInstance();
#endif

  Anki::Util::gLoggerProvider = nullptr;
  return 0;
}

