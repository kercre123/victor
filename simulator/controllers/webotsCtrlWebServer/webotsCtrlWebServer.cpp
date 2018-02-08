/*
 * File:          webotsCtrlWebServer.cpp
 * Date:
 * Description:   Cozmo 2.0 web server process for Webots simulation
 * Author:        
 * Modifications: 
 */

#include "cozmoWebServer.h"

#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"

//#include "osState/osState.h"

#include "json/json.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/channelFilter.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/logging.h"
#include "util/logging/multiFormattedLoggerProvider.h"

#include <fstream>

#include <webots/Supervisor.hpp>

#define LOG_CHANNEL    "webotsCtrlWebServer"

namespace Anki {
  namespace Cozmo {
    CONSOLE_VAR_EXTERN(bool, kEnableCladLogger);
  }
}

using namespace Anki;
using namespace Anki::Cozmo;


// Instantiate supervisor and pass to AndroidHAL
webots::Supervisor webserverSupervisor;


int main(int argc, char **argv)
{
  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  
  // create platform.
  // Unfortunately, CozmoAPI does not properly receive a const DataPlatform, and that change
  // is too big of a change, since it involves changing down to the context, so create a non-const platform
  //const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0]);
  Util::Data::DataPlatform dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlWebServer");
  
  // Set Webots supervisor
  //OSState::SetSupervisor(&webserverSupervisor);

  // - create and set logger
  Util::IFormattedLoggerProvider* printfLoggerProvider = new Util::PrintfLoggerProvider(Anki::Util::ILoggerProvider::LOG_LEVEL_WARN,
                                                                                        params.colorizeStderrOutput);
  Util::MultiFormattedLoggerProvider loggerProvider({
    printfLoggerProvider
  });
  loggerProvider.SetMinLogLevel(Anki::Util::ILoggerProvider::LOG_LEVEL_DEBUG);
  Util::gLoggerProvider = &loggerProvider;
  Util::sSetGlobal(DPHYS, "0xdeadffff00000001");
  
  // - console filter for logs
  if ( params.filterLog )
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();
    
    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
    if (!dataPlatform.readAsJson(Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      LOG_ERROR("webotsCtrlWebServer.main.loadConsoleConfig", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
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
    LOG_INFO("webotsCtrlWebServer.main.noFilter", "Console will not be filtered due to program args");
  }

  // Start with a step so that we can attach to the process here for debugging
  webserverSupervisor.step(WEB_SERVER_TIME_STEP_MS);
  
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT("consoleVars.ini");
  NativeAnkiUtilConsoleLoadVars();
  
  // Initialize the API
  CozmoWebServerEngine cozmoWebServer(&dataPlatform);
  cozmoWebServer.Init();

  LOG_INFO("webotsCtrlWebServer.main", "cozmoWebServer created and initialized.");

  //
  // Main Execution loop: step the world forward forever
  //
  auto tick_start = std::chrono::system_clock::now();
  while (webserverSupervisor.step(WEB_SERVER_TIME_STEP_MS) != -1)
  {
    double currTimeNanoseconds = Util::SecToNanoSec(webserverSupervisor.getTime());
    cozmoWebServer.Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));
    
    tick_start = std::chrono::system_clock::now();
    
  }

  Util::gLoggerProvider = nullptr;
  return 0;
}

