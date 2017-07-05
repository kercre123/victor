/*
 * File:          webotsCtrlAnim.cpp
 * Date:
 * Description:   Cozmo 2.0 animation process for Webots simulation
 * Author:        
 * Modifications: 
 */

#include "cozmoAnim/cozmoAnim.h"

#include "../shared/ctrlCommonInitialization.h"
#include "util/logging/logging.h"
#include "util/logging/channelFilter.h"
//#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "json/json.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/console/consoleInterface.h"
//#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "util/console/consoleSystem.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/multiFormattedLoggerProvider.h"
#include "util/global/globalDefinitions.h"

#include "util/time/stopWatch.h"

#include <fstream>

#include <webots/Supervisor.hpp>



#define ROBOT_ADVERTISING_HOST_IP "127.0.0.1"
#define SDK_ADVERTISING_HOST_IP   "127.0.0.1"
#define VIZ_HOST_IP               "127.0.0.1"

namespace Anki {
  namespace Cozmo {
    CONSOLE_VAR_EXTERN(bool, kEnableCladLogger);
  }
}

using namespace Anki;
using namespace Anki::Cozmo;


// Instantiate supervisor and pass to AndroidHAL
webots::Supervisor animSupervisor;


int main(int argc, char **argv)
{
  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  
  // create platform.
  // Unfortunately, CozmoAPI does not properly receive a const DataPlatform, and that change
  // is too big of a change, since it involves changing down to the context, so create a non-const platform
  //const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0]);
  Anki::Util::Data::DataPlatform dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlAnim");
  
  
  // - create and set logger
  Util::IFormattedLoggerProvider* printfLoggerProvider = new Util::PrintfLoggerProvider(Anki::Util::ILoggerProvider::LOG_LEVEL_WARN);
  Anki::Util::MultiFormattedLoggerProvider loggerProvider({
    printfLoggerProvider
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
    
    // also parse additional info for providers
    printfLoggerProvider->ParseLogLevelSettings(consoleFilterConfigOnPlatform);
    
  }
  else
  {
    PRINT_CH_INFO("LOG", "webotsCtrlGameEngine.main", "Console will not be filtered due to program args");
  }

  // Start with a step so that we can attach to the process here for debugging
  animSupervisor.step(ANIM_TIME_STEP);
  
  // Get configuration JSON
  Json::Value config;

  if (!dataPlatform.readAsJson(Util::Data::Scope::Resources,
                               "config/basestation/config/configuration.json", config)) {
    PRINT_NAMED_ERROR("webotsCtrlGameEngine.main.loadConfig", "Failed to parse Json file config/basestation/config/configuration.json");
  }

//  if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
//    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
//  }
//  if(!config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
//    config[AnkiUtil::kP_VIZ_HOST_IP] = VIZ_HOST_IP;
//  }
//  if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
//    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = ROBOT_ADVERTISING_PORT;
//  }
//  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
//    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
//  }

  
//  int numUIDevicesToWaitFor = 1;
//  webots::Field* numUIsField = engineSupervisor.getSelf()->getField("numUIDevicesToWaitFor");
//  if (numUIsField) {
//    numUIDevicesToWaitFor = numUIsField->getSFInt32();
//  } else {
//    PRINT_NAMED_WARNING("webotsCtrlGameEngine.main.MissingField", "numUIDevicesToWaitFor not found in BlockworldComms");
//  }
  
//  config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
//  config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
//  config[AnkiUtil::kP_NUM_SDK_DEVICES_TO_WAIT_FOR] = 1;
  
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT("consoleVars.ini");
  NativeAnkiUtilConsoleLoadVars();
  
  // Initialize the API
  CozmoAnim cozmoAnim(&dataPlatform);
  cozmoAnim.Init(config);

  PRINT_NAMED_INFO("webotsCtrlAnim.main", "CozmoAnim created and initialized.");

  Anki::Util::Time::StopWatch stopWatch("tick");

  //
  // Main Execution loop: step the world forward forever
  //
  auto tick_start = std::chrono::system_clock::now();
  while (animSupervisor.step(ANIM_TIME_STEP) != -1)
  {
    stopWatch.Start();

    double currTimeNanoseconds = Util::SecToNanoSec(animSupervisor.getTime());
    cozmoAnim.Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

    double timeMS = stopWatch.Stop();

    if( timeMS >= ANIM_TIME_STEP ) {
      PRINT_NAMED_WARNING("CozmoAnimHeartbeat.Overtime", "Update took %f ms (tick heartbeat is %dms)", timeMS, ANIM_TIME_STEP);
    }
    else if( timeMS >= 0.85*ANIM_TIME_STEP) {
      PRINT_NAMED_INFO("CozmoAnimHeartbeat.SlowTick", "Update took %f ms (tick heartbeat is %dms)", timeMS, ANIM_TIME_STEP);
    }
    
    tick_start = std::chrono::system_clock::now();
    
  } // while still stepping

  Anki::Util::gLoggerProvider = nullptr;
  return 0;
}

