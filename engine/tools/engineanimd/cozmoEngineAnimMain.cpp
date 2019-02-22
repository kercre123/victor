/** 
* File: cozmoEngineMain.cpp
*
* Author: Various Artists
* Created: 6/26/17
*
* Description: Cozmo Engine Process on Victor
*
* Copyright: Anki, inc. 2017
*
*/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "osState/osState.h"

#include "engine/cozmoAPI/cozmoAPI.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

#include "platform/common/diagnosticDefines.h"

#include "util/console/consoleSystem.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"

#include "util/logging/channelFilter.h"
#include "util/logging/iEventProvider.h"
#include "util/logging/iFormattedLoggerProvider.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/logging/victorLogger.h"
#include "util/string/stringUtils.h"

#include "anki/cozmo/shared/factory/emrHelper.h"
#include "platform/victorCrashReports/victorCrashReporter.h"

#if !defined(DEV_LOGGER_ENABLED)
  #if FACTORY_TEST
    #define DEV_LOGGER_ENABLED 1
  #else
    #define DEV_LOGGER_ENABLED 0
  #endif
#endif

#if DEV_LOGGER_ENABLED
#include "engine/debug/devLoggerProvider.h"
#include "engine/debug/devLoggingSystem.h"
#endif

#include <getopt.h>
#include <unistd.h>
#include <csignal>


#include "cozmoAnim/animEngine.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/channelFilter.h"
#include "util/logging/victorLogger.h"

#include "platform/common/diagnosticDefines.h"
#include "platform/victorCrashReports/victorCrashReporter.h"

#include <thread>
#include <unistd.h>
#include <csignal>


// What IP do we use for advertisement?
constexpr const char * ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";

// What process name do we use for logging?
constexpr const char * LOG_PROCNAME = "vic-engine";

// What channel name do we use for logging?
#define LOG_CHANNEL "CozmoEngineMain"

using namespace Anki;
using namespace Anki::Vector;

// Global singletons
Anki::Vector::CozmoAPI* gEngineAPI = nullptr;
Anki::Vector::Anim::AnimEngine* gAnimEngine = nullptr;
Anki::Util::Data::DataPlatform* gDataPlatform = nullptr;


namespace {

  // Termination flag
  bool gShutdown = false;

  // Private singleton
  std::unique_ptr<Anki::Util::VictorLogger> gVictorLogger;

  #if DEV_LOGGER_ENABLED
  // Private singleton
  std::unique_ptr<Anki::Util::MultiLoggerProvider> gMultiLogger;
  #endif

}

static void sigterm(int signum)
{
  Anki::Util::DropBreadcrumb(false, nullptr, -1);
  LOG_INFO("CozmoEngineAnimMain.SIGTERM", "Shutting down on signal %d", signum);
  gShutdown = true;
}

static void configure_engine_advertising(Json::Value& config)
{
  if (!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }
  if (!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = Anki::Vector::UI_ADVERTISING_PORT;
  }
}

static Anki::Util::Data::DataPlatform* createPlatform(const std::string& persistentPath,
                                                      const std::string& cachePath,
                                                      const std::string& resourcesPath)
{
  Anki::Util::FileUtils::CreateDirectory(persistentPath);
  Anki::Util::FileUtils::CreateDirectory(cachePath);
  Anki::Util::FileUtils::CreateDirectory(resourcesPath);

  return new Anki::Util::Data::DataPlatform(persistentPath, cachePath, resourcesPath);
}

Anki::Util::Data::DataPlatform* createPlatform()
{
  char config_file_path[PATH_MAX] = { 0 };
  const char* engine_env_config = getenv("VIC_ENGINE_CONFIG");
  if (engine_env_config != NULL) {
    strncpy(config_file_path, engine_env_config, sizeof(config_file_path));
  }
  const char* anim_env_config = getenv("VIC_ANIM_CONFIG");
  if (anim_env_config != NULL) {
    strncpy(config_file_path, anim_env_config, sizeof(config_file_path));
  }

  Json::Value config;

  printf("config_file: %s\n", config_file_path);
  if (strlen(config_file_path) > 0) {
    std::string config_file{config_file_path};
    if (!Anki::Util::FileUtils::FileExists(config_file)) {
      fprintf(stderr, "config file not found: %s\n", config_file_path);
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      PRINT_STREAM_ERROR("CozmoAnimMain.createPlatform",
        "json configuration parsing error: " << reader.getFormattedErrorMessages());
    }
  }

  std::string persistentPath;
  std::string cachePath;
  std::string resourcesPath;

  if (config.isMember("DataPlatformPersistentPath")) {
    persistentPath = config["DataPlatformPersistentPath"].asCString();
  } else {
    LOG_ERROR("cozmoAnimMain.createPlatform.DataPlatformPersistentPathUndefined", "");
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    LOG_ERROR("cozmoAnimMain.createPlatform.DataPlatformCachePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    LOG_ERROR("cozmoAnimMain.createPlatform.DataPlatformResourcesPathUndefined", "");
  }

  return createPlatform(persistentPath, cachePath, resourcesPath);
}

static bool common_start(const Json::Value& configuration)
{
  //
  // In normal usage, private singleton owns the logger until application exits.
  // When collecting developer logs, ownership of singleton VictorLogger is transferred to
  // singleton MultiLogger.
  //
  gVictorLogger = std::make_unique<Anki::Util::VictorLogger>(LOG_PROCNAME);

  Anki::Util::gLoggerProvider = gVictorLogger.get();
  Anki::Util::gEventProvider = gVictorLogger.get();
  LOG_INFO("cozmo_start", "Initializing engine");

  gDataPlatform = createPlatform();

#if (USE_DAS || DEV_LOGGER_ENABLED)
  const std::string& appRunId = Anki::Util::GetUUIDString();
#endif

  // - console filter for logs
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();
    
    // load file config
    Json::Value consoleFilterConfig;
    static const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
    if (!gDataPlatform->readAsJson(Anki::Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      LOG_ERROR("cozmo_start", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
      return false;
    }

    // initialize console filter for this platform
    const std::string& platformOS = gDataPlatform->GetOSPlatformString();
    const Json::Value& consoleFilterConfigOnPlatform = consoleFilterConfig[platformOS];
    consoleFilter->Initialize(consoleFilterConfigOnPlatform);

    // set filter in the loggers
    std::shared_ptr<const IChannelFilter> filterPtr( consoleFilter );

    Anki::Util::gLoggerProvider->SetFilter(filterPtr);
  }

#if DEV_LOGGER_ENABLED
  if(!FACTORY_TEST || (FACTORY_TEST && !Anki::Vector::Factory::GetEMR()->fields.PACKED_OUT_FLAG))
  {
    // Initialize Developer Logging System
    using DevLoggingSystem = Anki::Vector::DevLoggingSystem;
    const std::string& devlogPath = gDataPlatform->GetCurrentGameLogPath(LOG_PROCNAME);
    DevLoggingSystem::CreateInstance(devlogPath, appRunId);

    //
    // Replace singleton victor logger with a MultiLogger that manages both victor logger and dev logger.
    // Ownership of victor logger is transferred to MultiLogger.
    // Ownership of MultiLogger is managed by singleton unique_ptr.
    //
    std::vector<Anki::Util::ILoggerProvider*> loggers = {gVictorLogger.release(), DevLoggingSystem::GetInstancePrintProvider()};
    gMultiLogger = std::make_unique<Anki::Util::MultiLoggerProvider>(loggers);

    Anki::Util::gLoggerProvider = gMultiLogger.get();
  }
#endif
  return true;
}

static void common_stop()
{
  Anki::Util::SafeDelete(gDataPlatform);

  Anki::Util::gEventProvider = nullptr;
  Anki::Util::gLoggerProvider = nullptr;

#if DEV_LOGGER_ENABLED
  Anki::Vector::DevLoggingSystem::DestroyInstance();
#endif

  sync();
}

static bool cozmo_start(const Json::Value& configuration)
{
  Json::Value config = configuration;
  configure_engine_advertising(config);

  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(gDataPlatform->GetCachePath("consoleVarsEngine.ini").c_str());

  Anki::Vector::CozmoAPI* engineInstance = new Anki::Vector::CozmoAPI();

  const bool engineStarted = engineInstance->Start(gDataPlatform, config);
  if (!engineStarted) {
    delete engineInstance;
    return false;
  }

  gEngineAPI = engineInstance;

  return true;
}

static void cozmo_stop() {
  LOG_INFO("CozmoEngineMain.main", "Stopping engine");

  Anki::Util::SafeDelete(gEngineAPI);
}

static bool anim_start()
{
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(gDataPlatform->GetCachePath("consoleVarsAnim.ini").c_str());

  // Create and init AnimEngine
  gAnimEngine = new Anim::AnimEngine(gDataPlatform);

  Result result = gAnimEngine->Init();
  if (RESULT_OK != result) {
    LOG_ERROR("CozmoAnimMain.main.InitFailed", "Unable to initialize (exit %d)", result);
    delete gAnimEngine;
    return false;
  }

  return true;
}

static void* anim_main(void*)
{
  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();
  auto prevTickStart  = runStart;
  auto tickStart      = runStart;

  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(ANIM_TIME_STEP_US);

  // Loop until shutdown or error
  while (!gShutdown) {

    const duration<double> curTime_s = tickStart - runStart;
    const BaseStationTime_t curTime_ns = Util::numeric_cast<BaseStationTime_t>(Util::SecToNanoSec(curTime_s.count()));

    Result result = gAnimEngine->Update(curTime_ns);
    if (RESULT_OK != result) {
      LOG_WARNING("CozmoAnimMain.main.UpdateFailed", "Unable to update (result %d)", result);

      // Don't exit with error code so as not to trigger
      // fault code 800 on what is actually a clean shutdown.
      if (result == RESULT_SHUTDOWN) {
        result = RESULT_OK;
      }
      break;
    }

    const auto tickAfterAnimExecution = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickAfterAnimExecution);
    const auto tickDuration_us = duration_cast<microseconds>(tickAfterAnimExecution - tickStart);

    tracepoint(anki_ust, vic_anim_loop_duration, tickDuration_us.count());
#if ENABLE_TICK_TIME_WARNINGS
    // Complain if we're going overtime
    if (remaining_us < microseconds(-ANIM_OVERTIME_WARNING_THRESH_US))
    {
      LOG_WARNING("CozmoAnimMain.overtime", "Update() (%dms max) is behind by %.3fms",
                  ANIM_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }
#endif
    // We ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
    // allows other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    const auto sleepTime_us = std::max(minimumSleepTime_us, remaining_us);
    std::this_thread::sleep_for(sleepTime_us);

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(ANIM_TIME_STEP_US);

    // See if we've fallen very far behind (this happens e.g. after a 5-second blocking
    // load operation); if so, compensate by catching the target frame end time up somewhat.
    // This is so that we don't spend the next SEVERAL frames catching up.
    const auto timeBehind_us = -remaining_us;
    static const auto kusPerFrame = ((microseconds)(ANIM_TIME_STEP_US)).count();
    static const int kTooFarBehindFramesThreshold = 2;
    static const auto kTooFarBehindThreshold = (microseconds)(kTooFarBehindFramesThreshold * kusPerFrame);
    if (timeBehind_us >= kTooFarBehindThreshold)
    {
      const int framesBehind = (int)(timeBehind_us.count() / kusPerFrame);
      const auto forwardJumpDuration = kusPerFrame * framesBehind;
      targetEndFrameTime += (microseconds)forwardJumpDuration;
#if ENABLE_TICK_TIME_WARNINGS
      LOG_WARNING("CozmoAnimMain.catchup",
                  "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                  (float)(forwardJumpDuration * 0.001f));
#endif
    }
    tickStart = TimeClock::now();

    const auto timeSinceLastTick_us = duration_cast<microseconds>(tickStart - prevTickStart);
    prevTickStart = tickStart;

    const auto sleepTimeActual_us = duration_cast<microseconds>(tickStart - tickAfterAnimExecution);
    gAnimEngine->RegisterTickPerformance(tickDuration_us.count() * 0.001f,
                                        timeSinceLastTick_us.count() * 0.001f,
                                        sleepTime_us.count() * 0.001f,
                                        sleepTimeActual_us.count() * 0.001f);
  }

  return NULL;
}

static void anim_stop() {
  LOG_INFO("CozmoAnimMain.main.Shutdown", "Shutting down.");

  delete gAnimEngine;
}

int main(int argc, char* argv[])
{
  // Install signal handler
  signal(SIGTERM, sigterm);

  Anki::Vector::InstallCrashReporter(LOG_PROCNAME);

  char cwd[PATH_MAX] = { 0 };
  (void)getcwd(cwd, sizeof(cwd));
  printf("CWD: %s\n", cwd);
  printf("argv[0]: %s\n", argv[0]);
  printf("exe path: %s/%s\n", cwd, argv[0]);

  int help_flag = 0;

  static const char *opt_string = "hc:";

  static const struct option long_options[] = {
      { "config",     required_argument,      NULL,           'c' },
      { "help",       no_argument,            &help_flag,     'h' },
      { NULL,         no_argument,            NULL,           0   }
  };

  char config_file_path[PATH_MAX] = { 0 };
  const char* env_config = getenv("VIC_ENGINE_CONFIG");
  if (env_config != NULL) {
    strncpy(config_file_path, env_config, sizeof(config_file_path));
  }

  while(true) {
    int option_index = 0;
    int c = getopt_long(argc, argv, opt_string, long_options, &option_index);

    if (-1 == c) {
      break;
    }

    switch(c) {
      case 0:
      case 1:
      {
        if (long_options[option_index].flag != 0)
          break;
        printf("option %s", long_options[option_index].name);
        if (optarg)
          printf(" with arg %s", optarg);
        printf("\n");
        break;
      }
      case 'c':
      {
        strncpy(config_file_path, optarg, sizeof(config_file_path));
        config_file_path[PATH_MAX-1] = 0;
        break;
      }
      case 'h':
        help_flag = 1;
        break;
      case '?':
        break;
      default:
        abort();
    }
  }

  if (help_flag) {
    char* prog_name = basename(argv[0]);
    printf("%s <OPTIONS>\n", prog_name);
    printf("  -h, --help                          print this help message\n");
    printf("  -c, --config [JSON FILE]            load config json file\n");
    Anki::Vector::UninstallCrashReporter();
    return 1;
  }

  Json::Value config;

  printf("config_file: %s\n", config_file_path);
  if (strlen(config_file_path) > 0) {
    std::string config_file{config_file_path};
    if (!Anki::Util::FileUtils::FileExists(config_file)) {
      fprintf(stderr, "config file not found: %s\n", config_file_path);
      Anki::Vector::UninstallCrashReporter();
      return 1;
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      printf("CozmoEngineMain.main: json configuration parsing error: %s\n",
             reader.getFormattedErrorMessages().c_str());
      Anki::Vector::UninstallCrashReporter();
      return 1;
    }
  }

  const bool started1 = common_start(config);
  if (!started1) {
    printf("failed to common\n");
    Anki::Vector::UninstallCrashReporter();
    return 1;
  }
  const bool started2 = cozmo_start(config);
  if (!started2) {
    printf("failed to start engine\n");
    Anki::Vector::UninstallCrashReporter();
    return 1;
  }
  const bool started3 = anim_start();
  if (!started3) {
    printf("failed to start anim\n");
    Anki::Vector::UninstallCrashReporter();
    return 1;
  }

  LOG_INFO("CozmoEngineMain.main", "Engine started");

  pthread_t vicanim_thread;
  pthread_attr_t vicanim_attr;
  pthread_attr_init(&vicanim_attr);
  pthread_create(&vicanim_thread, &vicanim_attr, anim_main, NULL);

  // engine has started, plus it's shared stuff, now start anim

  using namespace std::chrono;
  using TimeClock = steady_clock;

  const auto runStart = TimeClock::now();
  auto prevTickStart  = runStart;
  auto tickStart      = runStart;

  // Set the target time for the end of the first frame
  auto targetEndFrameTime = runStart + (microseconds)(Anki::Vector::BS_TIME_STEP_MICROSECONDS);

  while (!gShutdown)
  {
    const duration<double> curTimeSeconds = tickStart - runStart;
    const double curTimeNanoseconds = Anki::Util::SecToNanoSec(curTimeSeconds.count());

    const bool tickSuccess = gEngineAPI->Update(Anki::Util::numeric_cast<BaseStationTime_t>(curTimeNanoseconds));

    const auto tickAfterEngineExecution = TimeClock::now();
    const auto remaining_us = duration_cast<microseconds>(targetEndFrameTime - tickAfterEngineExecution);
    const auto tickDuration_us = duration_cast<microseconds>(tickAfterEngineExecution - tickStart);

    tracepoint(anki_ust, vic_engine_loop_duration, tickDuration_us.count());
#if ENABLE_TICK_TIME_WARNINGS
    // Only complain if we're more than 10ms behind
    if (remaining_us < microseconds(-10000))
    {
      LOG_WARNING("CozmoEngineMain.main.overtime", "Update() (%dms max) is behind by %.3fms",
                  Anki::Vector::BS_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }
#endif

    // We ALWAYS sleep, but if we're overtime, we 'sleep zero' which still allows
    // other threads to run
    static const auto minimumSleepTime_us = microseconds((long)0);
    const auto sleepTime_us = std::max(minimumSleepTime_us, remaining_us);
    {
      using namespace Anki;
      ANKI_CPU_PROFILE("CozmoEngineMain.main.Sleep");

      std::this_thread::sleep_for(sleepTime_us);
    }

    // Set the target end time for the next frame
    targetEndFrameTime += (microseconds)(Anki::Vector::BS_TIME_STEP_MICROSECONDS);

    // See if we've fallen quite far behind; if so, compensate by catching the target frame end time up somewhat.
    // This is so that we don't spend SEVERAL frames trying to catch up (by depriving sleep time).
    const auto timeBehind_us = -remaining_us;
    static const auto kusPerFrame = ((microseconds)(Anki::Vector::BS_TIME_STEP_MICROSECONDS)).count();
    static const int kTooFarBehindFramesThreshold = 2;
    static const auto kTooFarBehindThreshold = (microseconds)(kTooFarBehindFramesThreshold * kusPerFrame);
    if (timeBehind_us >= kTooFarBehindThreshold)
    {
      const int framesBehind = (int)(timeBehind_us.count() / kusPerFrame);
      const auto forwardJumpDuration = kusPerFrame * framesBehind;
      targetEndFrameTime += (microseconds)forwardJumpDuration;
#if ENABLE_TICK_TIME_WARNINGS
      LOG_WARNING("CozmoEngineMain.main.catchup",
                  "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                  (float)(forwardJumpDuration * 0.001f));
#endif
    }

    tickStart = TimeClock::now();

    const auto timeSinceLastTick_us = duration_cast<microseconds>(tickStart - prevTickStart);
    prevTickStart = tickStart;

    const auto sleepTimeActual_us = duration_cast<microseconds>(tickStart - tickAfterEngineExecution);
    gEngineAPI->RegisterEngineTickPerformance(tickDuration_us.count() * 0.001f,
                                              timeSinceLastTick_us.count() * 0.001f,
                                              sleepTime_us.count() * 0.001f,
                                              sleepTimeActual_us.count() * 0.001f);

    if (!tickSuccess)
    {
      // If we fail to update properly, stop running (but after we've recorded the above stuff)
      LOG_INFO("CozmoEngineMain.main", "Engine has stopped");
      break;
    }
  } // End of tick loop

  cozmo_stop();
  anim_stop();
  common_stop();

  Anki::Vector::UninstallCrashReporter();
  sync();

  return 0;
}

#if 0
/**
* File: cozmoEngineAnimMain.cpp
*
* Author: Richard Gale
* Created: 2/6/19
*
* Description: Combine Engine+Anim Process on Victor
*
* Copyright: Anki, inc. 2019
*
*/

#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  typedef void *(threadPtr)(void *);

  char *error;

  void *vicanim_handle = dlopen("libvic-anim.so", RTLD_LAZY);
  if (!vicanim_handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }

  threadPtr* vicanim_fn = (threadPtr*)dlsym(vicanim_handle, "threadmain");
  if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    exit(1);
  }

  void *vicengine_handle = dlopen("libvic-engine.so", RTLD_LAZY);
  if (!vicengine_handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(1);
  }

  threadPtr* vicengine_fn = (threadPtr*)dlsym(vicengine_handle, "threadmain");
  if ((error = dlerror()) != NULL)  {
    fprintf(stderr, "%s\n", error);
    exit(1);
  }

  struct argcv {
    int argc;
    char **argv;
  } p;

  p.argc = argc;
  p.argv = argv;

  pthread_t vicanim_thread;
  pthread_attr_t vicanim_attr;
  pthread_attr_init(&vicanim_attr);
  pthread_create(&vicanim_thread, &vicanim_attr, vicanim_fn, &p);

  pthread_t vicengine_thread;
  pthread_attr_t vicengine_attr;
  pthread_attr_init(&vicengine_attr);
  pthread_create(&vicengine_thread, &vicengine_attr, vicengine_fn, &p);

  pthread_join(vicengine_thread, NULL);

  return 1;
}
#endif
