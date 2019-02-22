/*
 * File:          webotsCtrlGameEngine2.cpp
 * Date:
 * Description:   Cozmo 2.0 engine for Webots simulation
 * Author:
 * Modifications:
 */

#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "camera/cameraService.h"
#include "cubeBleClient/cubeBleClient.h"
#include "osState/osState.h"
#include "whiskeyToF/tof.h"

#if USE_ENGINEANIM_COMBINED
#include "cozmoAnim/animEngine.h"
#endif

#include "json/json.h"

#include "engine/cozmoAPI/cozmoAPI.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/logging/channelFilter.h"
#include "util/logging/eventProviderLoggingAdapter.h"
#include "util/logging/multiFormattedLoggerProvider.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/time/stopWatch.h"

#if ANKI_DEV_CHEATS
#include "engine/debug/devLoggerProvider.h"
#include "engine/debug/devLoggingSystem.h"
#include "util/fileUtils/fileUtils.h"
#endif

#include <webots/Supervisor.hpp>

#define ROBOT_ADVERTISING_HOST_IP "127.0.0.1"

#define LOG_CHANNEL "WebotsCtrlGameEngine"

namespace Anki {
  namespace Vector {
    CONSOLE_VAR_EXTERN(bool, kEnableCladLogger);
  }
}

using namespace Anki;
using namespace Anki::Vector;

#if USE_ENGINEANIM_COMBINED
// Instantiate supervisor and pass to AndroidHAL and cubeBleClient
webots::Supervisor animSupervisor;
#endif

int main(int argc, char **argv)
{
  // Instantiate supervisor and pass to various singletons
#if USE_ENGINEANIM_COMBINED
  CameraService::SetSupervisor(&animSupervisor);
  CubeBleClient::SetSupervisor(&animSupervisor);
  ToFSensor::SetSupervisor(&animSupervisor);

  // Set RobotID
  OSState::getInstance()->SetRobotID(animSupervisor.getSelf()->getField("robotID")->getSFInt32());
#else
  webots::Supervisor engineSupervisor;

  CameraService::SetSupervisor(&engineSupervisor);
  CubeBleClient::SetSupervisor(&engineSupervisor);
  ToFSensor::SetSupervisor(&engineSupervisor);

  // Set RobotID
  OSState::getInstance()->SetRobotID(engineSupervisor.getSelf()->getField("robotID")->getSFInt32());
#endif
  
#if USE_ENGINEANIM_COMBINED
  const u32 ANIM_TIME_STEP_MS = 33;
  animSupervisor.step(ANIM_TIME_STEP_MS);
#else
  // Start with a step so that we can attach to the process here for debugging
  engineSupervisor.step(BS_TIME_STEP_MS);
#endif

  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);

  // create platform.
  // Unfortunately, CozmoAPI does not properly receive a const DataPlatform, and that change
  // is too big of a change, since it involves changing down to the context, so create a non-const platform
  //const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0]);
  Anki::Util::Data::DataPlatform dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlGameEngine2");

  // Get robotID to determine if devlogger should be created
  // Only create devLogs for robot with DEFAULT_ROBOT_ID.
  // The only time it shouldn't create a log is for sim robots
  // with non-DEFAULT_ROBOT_ID so as to avoid multiple devLogs
  // recording to the same folder.

  RobotID_t robotID = OSState::getInstance()->GetRobotID();
  const bool createDevLoggers = robotID == DEFAULT_ROBOT_ID;

#if ANKI_DEV_CHEATS
  if (createDevLoggers) {
    DevLoggingSystem::CreateInstance(dataPlatform.pathToResource(Util::Data::Scope::CurrentGameLog, "devLogger"), "mac");
  } else {
    LOG_WARNING("webotsCtrlGameEngine.main.SkippingDevLogger",
                "RobotID: %d - Only DEFAULT_ROBOT_ID may create loggers", robotID);
  }
#endif

  // - create and set logger
  Util::PrintfLoggerProvider* printfLoggerProvider = new Util::PrintfLoggerProvider(Anki::Util::LOG_LEVEL_WARN,
                                                                                    params.colorizeStderrOutput);
  std::vector<Util::IFormattedLoggerProvider*> loggerVec;
  loggerVec.push_back(printfLoggerProvider);

#if ANKI_DEV_CHEATS
  if (createDevLoggers) {
    loggerVec.push_back(new DevLoggerProvider(DevLoggingSystem::GetInstance()->GetQueue(),
            Util::FileUtils::FullFilePath( {DevLoggingSystem::GetInstance()->GetDevLoggingBaseDirectory(), DevLoggingSystem::kPrintName} )));
  }
#endif

  Anki::Util::MultiFormattedLoggerProvider loggerProvider(loggerVec);

  loggerProvider.SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);

  auto* eventProvider = new Anki::Util::EventProviderLoggingAdapter( &loggerProvider );

  Anki::Util::gLoggerProvider = &loggerProvider;
  Anki::Util::gEventProvider = eventProvider;
  Anki::Util::sSetGlobal(DPHYS, "0xdeadffff00000001");

  // - console filter for logs
  if (params.filterLog)
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();

    // load file config
    Json::Value consoleFilterConfig;
    static const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
    if (!dataPlatform.readAsJson(Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      LOG_ERROR("webotsCtrlGameEngine.main.loadConsoleConfig", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
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

    #if ANKI_DEV_CHEATS
    {
      // Disable the Clad logger by default - prevents it sending the log messages,
      // otherwise anyone with a config set to spam every message could run into issues
      // with the amount of messages overwhelming the socket on engine startup/load.
      // Anyone who needs the log messages can enable it afterwards via Unity, SDK or Webots
      Anki::Vector::kEnableCladLogger = false;
    }
    #endif // ANKI_DEV_CHEATS
  }
  else
  {
    LOG_INFO("webotsCtrlGameEngine.main.noFilter", "Console will not be filtered due to program args");
  }

  // Get configuration JSON
  Json::Value config;

  if (!dataPlatform.readAsJson(Util::Data::Scope::Resources,
                               "config/engine/configuration.json", config)) {
    LOG_ERROR("webotsCtrlGameEngine.main.loadConfig", "Failed to parse Json file config/engine/configuration.json");
  }

  if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }

  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  }

  int numUIDevicesToWaitFor = 1;
#if USE_ENGINEANIM_COMBINED
  webots::Field* numUIsField = animSupervisor.getSelf()->getField("numUIDevicesToWaitFor");
#else
  webots::Field* numUIsField = engineSupervisor.getSelf()->getField("numUIDevicesToWaitFor");
#endif
  if (numUIsField) {
    numUIDevicesToWaitFor = numUIsField->getSFInt32();
  } else {
    LOG_WARNING("webotsCtrlGameEngine.main.MissingField", "numUIDevicesToWaitFor not found in BlockworldComms");
  }

  config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
  config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;

  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT("consoleVarsEngine.ini");

  // Initialize the API
  CozmoAPI myVictor;
  const bool initSuccess = myVictor.Start(&dataPlatform, config);
  if (!initSuccess) {
    LOG_ERROR("webotsCtrlGameEngine.main", "Failed in creation/initialization of CozmoAPI");
  }
  else {
#if USE_ENGINEANIM_COMBINED
    // Initialize the anim engine
    Anim::AnimEngine animEngine(&dataPlatform);
    const auto animSuccess = animEngine.Init();
    if (animSuccess != RESULT_OK) {
      LOG_ERROR("webotsCtrlGameEngine.main", "Failed in creation/initialization of AnimEngine");
    } else {
      LOG_INFO("webotsCtrlGameEngine.main", "CozmoAPI and AnimEngine created and initialized.");

      Anki::Util::Time::StopWatch stopWatch("tick");

      //
      // Main Execution loop: step the world forward
      //
//      double last_time_step = Util::SecToNanoSec(animSupervisor.getTime());;
      while (animSupervisor.step(ANIM_TIME_STEP_MS) != -1)
      { 
        stopWatch.Start();
        
        const double currTimeNanoseconds = Util::SecToNanoSec(animSupervisor.getTime());

        // Update time
        BaseStationTimer::getInstance()->UpdateTime(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

        // Update OSState
        OSState::getInstance()->Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

        const bool tickSuccess = myVictor.Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));
        if (!tickSuccess) {
          break;
        }

        animEngine.Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

//        const double diff = currTimeNanoseconds - last_time_step;
//        if (diff >= BS_TIME_STEP_MS * 1000) {
//          const bool tickSuccess = myVictor.Update(Util::numeric_cast<BaseStationTime_t>(BS_TIME_STEP_MS * 1000));
//          if (!tickSuccess) {
//            break;
//          }
//
//          last_time_step += BS_TIME_STEP_MS * 1000;
//        }

        const float time_ms = Util::numeric_cast<float>(stopWatch.Stop());

        // Record engine tick performance; this includes a call to PerfMetric.
        // For webots, we 'fake' the sleep time here.  Unlike in Cozmo webots,
        // we don't actually sleep in this loop
        static const float kTargetDuration_ms = Util::numeric_cast<float>(BS_TIME_STEP_MS);
        const float engineFreq_ms = std::max(time_ms, kTargetDuration_ms);
        const float sleepTime_ms = std::max(0.0f, kTargetDuration_ms - time_ms);
        const float sleepTimeActual_ms = sleepTime_ms;
        myVictor.RegisterEngineTickPerformance(time_ms,
                                               engineFreq_ms,
                                               sleepTime_ms,
                                               sleepTimeActual_ms);

//        const float time_ms = Util::numeric_cast<float>(stopWatch.Stop());

        // Record tick performance; this includes a call to PerfMetric.
        // For webots, we 'fake' the sleep time here.  Unlike in Cozmo webots,
        // we don't actually sleep in this loop
//        static const float kTargetDuration_ms = Util::numeric_cast<float>(ANIM_TIME_STEP_MS);
        const float animFreq_ms  = std::max(time_ms, kTargetDuration_ms);
//        const float sleepTime_ms = std::max(0.0f, kTargetDuration_ms - time_ms);
//        const float sleepTimeActual_ms = sleepTime_ms;
        animEngine.RegisterTickPerformance(time_ms,
                                           animFreq_ms,
                                           sleepTime_ms,
                                           sleepTimeActual_ms);
      } // end tick loop
    }
#else
    LOG_INFO("webotsCtrlGameEngine.main", "CozmoAPI created and initialized.");

    Anki::Util::Time::StopWatch stopWatch("tick");

    //
    // Main Execution loop: step the world forward
    //
    while (engineSupervisor.step(BS_TIME_STEP_MS) != -1)
    { 
      stopWatch.Start();
      
      const double currTimeNanoseconds = Util::SecToNanoSec(engineSupervisor.getTime());

      // Update time
      BaseStationTimer::getInstance()->UpdateTime(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

      // Update OSState
      OSState::getInstance()->Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

      const bool tickSuccess = myVictor.Update(Util::numeric_cast<BaseStationTime_t>(currTimeNanoseconds));

      const float time_ms = Util::numeric_cast<float>(stopWatch.Stop());

      // Record engine tick performance; this includes a call to PerfMetric.
      // For webots, we 'fake' the sleep time here.  Unlike in Cozmo webots,
      // we don't actually sleep in this loop
      static const float kTargetDuration_ms = Util::numeric_cast<float>(BS_TIME_STEP_MS);
      const float engineFreq_ms = std::max(time_ms, kTargetDuration_ms);
      const float sleepTime_ms = std::max(0.0f, kTargetDuration_ms - time_ms);
      const float sleepTimeActual_ms = sleepTime_ms;
      myVictor.RegisterEngineTickPerformance(time_ms,
                                             engineFreq_ms,
                                             sleepTime_ms,
                                             sleepTimeActual_ms);

      if (!tickSuccess) {
        break;
      }
    } // end tick loop
#endif
  }

#if ANKI_DEV_CHEATS
  DevLoggingSystem::DestroyInstance();
#endif

  Anki::Util::gLoggerProvider = nullptr;

  Anki::Util::gEventProvider = nullptr;
  delete eventProvider;

  return 0;
}
