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

#include "json/json.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "platform/common/diagnosticDefines.h"

#include "engine/cozmoAPI/cozmoAPI.h"
#include "../../animProcess/src/cozmoAnim/animEngine.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

#include "util/console/consoleSystem.h"
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

#include <string>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#include <csignal>

using namespace Anki;
using namespace Anki::Vector;

// What IP do we use for advertisement?
constexpr const char * ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";

// What process name do we use for logging?
constexpr const char * LOG_PROCNAME = "vic-engine";

// What channel name do we use for logging?
constexpr const char * LOG_CHANNEL = "CozmoEngineMain";

// How often do we check for engine stop?
constexpr const int SLEEP_DELAY_US = (10*1000);

// Global singletons
Anki::Vector::CozmoAPI* gEngineAPI = nullptr;
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

static void sigterm(int)
{
  Anki::Util::DropBreadcrumb(false, nullptr, -1);
  LOG_INFO("CozmoEngineMain.SIGTERM", "Shutting down");
  gShutdown = true;
}

void configure_engine(Json::Value& config)
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

static int cozmo_start(const Json::Value& configuration)
{
  int result = 0;

  if (gEngineAPI != nullptr) {
      LOG_ERROR("cozmo_start", "Game already initialized");
      return 1;
  }

  //
  // In normal usage, private singleton owns the logger until application exits.
  // When collecting developer logs, ownership of singleton VictorLogger is transferred to
  // singleton MultiLogger.
  //
  gVictorLogger = std::make_unique<Anki::Util::VictorLogger>(LOG_PROCNAME);

  Anki::Util::gLoggerProvider = gVictorLogger.get();
  Anki::Util::gEventProvider = gVictorLogger.get();


  std::string persistentPath;
  std::string cachePath;
  std::string resourcesPath;
  std::string resourcesBasePath;

  // copy existing configuration data
  Json::Value config(configuration);

  if (config.isMember("DataPlatformPersistentPath")) {
    persistentPath = config["DataPlatformPersistentPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoEngineMain.createPlatform.DataPlatformPersistentPathUndefined", "");
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoEngineMain.createPlatform.DataPlatformCachePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesBasePath")) {
    resourcesBasePath = config["DataPlatformResourcesBasePath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoEngineMain.createPlatform.DataPlatformResourcesBasePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoEngineMain.createPlatform.DataPlatformResourcesPathUndefined", "");
  }

  gDataPlatform = createPlatform(persistentPath, cachePath, resourcesPath);

  LOG_DEBUG("CozmoStart.ResourcesPath", "%s", resourcesPath.c_str());

  #if (USE_DAS || DEV_LOGGER_ENABLED)
  const std::string& appRunId = Anki::Util::GetUUIDString();
  #endif

  // - console filter for logs
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();
    
    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
    if (!gDataPlatform->readAsJson(Anki::Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      LOG_ERROR("cozmo_start", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
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

  LOG_INFO("cozmo_start", "Creating engine");
  LOG_INFO("cozmo_start",
            "Initialized data platform with persistentPath = %s, cachePath = %s, resourcesPath = %s",
            persistentPath.c_str(), cachePath.c_str(), resourcesPath.c_str());

  configure_engine(config);

  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(gDataPlatform->GetCachePath("consoleVarsEngine.ini").c_str());

  Anki::Vector::CozmoAPI* engineInstance = new Anki::Vector::CozmoAPI();

  bool engineResult = engineInstance->StartRun(gDataPlatform, config);
  if (!engineResult) {
    delete engineInstance;
    return (int)engineResult;
  }

  gEngineAPI = engineInstance;

  return result;
}

static bool cozmo_is_running()
{
  if (gEngineAPI) {
    return gEngineAPI->IsRunning();
  }
  return false;
}

static int cozmo_stop()
{
  int result = 0;

  if (nullptr != gEngineAPI) {
    gEngineAPI->Clear();
  }

  Anki::Util::SafeDelete(gEngineAPI);
  Anki::Util::SafeDelete(gDataPlatform);

  Anki::Util::gEventProvider = nullptr;
  Anki::Util::gLoggerProvider = nullptr;

#if DEV_LOGGER_ENABLED
  Anki::Vector::DevLoggingSystem::DestroyInstance();
#endif

  sync();

  return result;
}

Anki::Util::Data::DataPlatform* createPlatform()
{
  char config_file_path[PATH_MAX] = { 0 };
  const char* env_config = "/anki/etc/config/platform_config.json"; // TODO:REG getenv("VIC_ANIM_CONFIG");
  if (env_config != NULL) {
    strncpy(config_file_path, env_config, sizeof(config_file_path));
  }

  Json::Value config;

  printf("config_file: %s\n", config_file_path);
  if (strlen(config_file_path)) {
    std::string config_file{config_file_path};
    if (!Anki::Util::FileUtils::FileExists(config_file)) {
      fprintf(stderr, "config file not found: %s\n", config_file_path);
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s\n", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      PRINT_STREAM_ERROR("cozmo_startup",
        "json configuration parsing error: " << reader.getFormattedErrorMessages());
    }
  }

  std::string persistentPath;
  std::string cachePath;
  std::string resourcesPath;

  if (config.isMember("DataPlatformPersistentPath")) {
    persistentPath = config["DataPlatformPersistentPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformPersistentPathUndefined", "");
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformCachePathUndefined", "");
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    PRINT_NAMED_ERROR("cozmoAnimMain.createPlatform.DataPlatformResourcesPathUndefined", "");
  }

  return createPlatform(persistentPath, cachePath, resourcesPath);
}

void* ANIM_THREAD(void *) {
  auto dataPlatform = createPlatform();

  // Create and init AnimEngine
  AnimEngine * animEngine = new AnimEngine(dataPlatform);

  Result result = animEngine->Init();
  if (RESULT_OK != result) {
    LOG_ERROR("CozmoAnimMain.main.InitFailed", "Unable to initialize (exit %d)", result);
    delete animEngine;
    Util::gLoggerProvider = nullptr;
    Util::gEventProvider = nullptr;
#ifdef VICOS
    UninstallCrashReporter();
#endif
    sync();
    exit(result);
  }

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
    const BaseStationTime_t curTime_ns = Anki::Util::numeric_cast<BaseStationTime_t>(Util::SecToNanoSec(curTime_s.count()));

    result = animEngine->Update(curTime_ns);
    if (RESULT_OK != result) {
      PRINT_NAMED_WARNING("CozmoAnimMain.main.UpdateFailed", "Unable to update (result %d)", result);

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
      PRINT_NAMED_WARNING("CozmoAnimMain.overtime", "Update() (%dms max) is behind by %.3fms",
                          ANIM_TIME_STEP_MS, (float)(-remaining_us).count() * 0.001f);
    }
#endif
    // Now we ALWAYS sleep, but if we're overtime, we 'sleep zero' which still
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
      PRINT_NAMED_WARNING("CozmoAnimMain.catchup",
                          "Update was too far behind so moving target end frame time forward by an additional %.3fms",
                          (float)(forwardJumpDuration * 0.001f));
#endif
    }
    tickStart = TimeClock::now();

    const auto timeSinceLastTick_us = duration_cast<microseconds>(tickStart - prevTickStart);
    prevTickStart = tickStart;

    const auto sleepTimeActual_us = duration_cast<microseconds>(tickStart - tickAfterAnimExecution);
    animEngine->RegisterTickPerformance(tickDuration_us.count() * 0.001f,
                                        timeSinceLastTick_us.count() * 0.001f,
                                        sleepTime_us.count() * 0.001f,
                                        sleepTimeActual_us.count() * 0.001f);
  }

  LOG_INFO("CozmoAnimMain.main.Shutdown", "Shutting down (exit %d)", result);

  return nullptr;
}

void* ENGINE_THREAD(void *) {
  int argc = 0;
  char argv0[32] = "cozmoEngineMain.cpp";
  char *argv[] = { argv0 };

  char cwd[PATH_MAX] = { 0 };
  (void)getcwd(cwd, sizeof(cwd));
  printf("CWD: %s\n", cwd);
  printf("argv[0]: %s\n", argv[0]);
  printf("exe path: %s/%s\n", cwd, argv[0]);

  int verbose_flag = 0;
  int help_flag = 0;

  const char *opt_string = "vhc:";

  const struct option long_options[] = {
      { "verbose",    no_argument,            &verbose_flag,  'v' },
      { "config",     required_argument,      NULL,           'c' },
      { "help",       no_argument,            &help_flag,     'h' },
      { NULL,         no_argument,            NULL,           0   }
  };

  char config_file_path[PATH_MAX] = { 0 };
  const char* env_config = "/anki/etc/config/platform_config.json"; // TODO:REG getenv("VIC_ENGINE_CONFIG");
  if (env_config != NULL) {
    strncpy(config_file_path, env_config, sizeof(config_file_path));
  }

  while(1) {
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
        printf ("option %s", long_options[option_index].name);
        if (optarg)
          printf (" with arg %s", optarg);
        printf ("\n");
        break;
      }
      case 'c':
      {
        strncpy(config_file_path, optarg, sizeof(config_file_path));
        config_file_path[PATH_MAX-1] = 0;
        break;
      }
      case 'v':
        verbose_flag = 1;
        break;
      case 'h':
        help_flag = 1;
        break;
      case '?':
        break;
      default:
        abort();
    }
  }

  printf("help_flag: %d\n", help_flag);

  if (help_flag) {
    char* prog_name = basename(argv[0]);
    printf("%s <OPTIONS>\n", prog_name);
    printf("  -h, --help                          print this help message\n");
    printf("  -v, --verbose                       dump verbose output\n");
    printf("  -c, --config [JSON FILE]            load config json file\n");
    return (void*)1;
  }

  if (verbose_flag) {
    printf("verbose!\n");
  }

  Json::Value config;

  printf("config_file: %s\n", config_file_path);
  if (strlen(config_file_path)) {
    std::string config_file{config_file_path};
    if (!Anki::Util::FileUtils::FileExists(config_file)) {
      fprintf(stderr, "config file not found: %s\n", config_file_path);
      return (void*)1;
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s\n", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      PRINT_STREAM_ERROR("cozmo_startup", "json configuration parsing error: " << reader.getFormattedErrorMessages());
      return (void*)1;
    }
  }

  int res = cozmo_start(config);
  if (0 != res) {
      printf("failed to start engine\n");
#ifdef VICOS
      Anki::Vector::UninstallCrashReporter();
#endif
      exit(res);
  }

  LOG_INFO("CozmoEngineMain.main", "Engine started");

  while (!gShutdown) {
    if (!cozmo_is_running()) {
      LOG_INFO("CozmoEngineMain.main", "Engine has stopped");
      break;
    }
    usleep(SLEEP_DELAY_US);
  }

  LOG_INFO("CozmoEngineMain.main", "Stopping engine");
  res = cozmo_stop();

  return nullptr;
}

int main(int argc, char* argv[])
{
  // Install signal handler
  signal(SIGTERM, sigterm);

#ifdef VICOS
  Anki::Vector::InstallCrashReporter(LOG_PROCNAME);
#endif

  pthread_t ENGINE_thread_id;
  pthread_t ANIM_thread_id;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&ANIM_thread_id, &attr, ANIM_THREAD, NULL);
  pthread_create(&ENGINE_thread_id, &attr, ENGINE_THREAD, NULL);
  while(1);

#ifdef VICOS
  Anki::Vector::UninstallCrashReporter();
#endif
  return 0;
}
