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

#include "engine/cozmoAPI/cozmoAPI.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/androidLogPrintLogger_vicos.h"
#include "util/logging/channelFilter.h"
#include "util/logging/iEventProvider.h"
#include "util/logging/iFormattedLoggerProvider.h"
#include "util/logging/logging.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/string/stringUtils.h"

#if USE_DAS
#include "DAS/DAS.h"
#include "DAS/DASPlatform.h"
#include "platform/victorDAS/DASPlatform.h"
#include "platform/victorDAS/DASLogger.h"
#endif

#include "anki/cozmo/shared/factory/emrHelper.h"
// FIXME: We need to build Breakpad libs for VICOS
// #include "platform/victorCrashReports/google_breakpad.h"

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


// What IP do we use for advertisement?
constexpr const char * ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";

// What process name do we use for logging?
constexpr const char * LOG_PROCNAME = "vic-engine";

// What channel name do we use for logging?
constexpr const char * LOG_CHANNEL = "CozmoEngineMain";

// How often do we check for engine stop?
constexpr const int SLEEP_DELAY_US = (10*1000);

// Global singletons
Anki::Cozmo::CozmoAPI* gEngineAPI = nullptr;
Anki::Util::Data::DataPlatform* gDataPlatform = nullptr;

// Signal handlers
namespace {
  bool gShutdown = false;
}

static void sigterm(int)
{
  LOG_INFO("CozmoEngineMain.SIGTERM", "Shutting down");
  gShutdown = true;
}

void configure_engine(Json::Value& config)
{
  if (!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }
  if (!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = Anki::Cozmo::UI_ADVERTISING_PORT;
  }
  if (!config.isMember(AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT)) {
    config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = Anki::Cozmo::SDK_ON_DEVICE_TCP_PORT;
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

  // Build up a list of enabled log providers
  std::vector<Anki::Util::ILoggerProvider*> loggers;

  Anki::Util::AndroidLogPrintLogger * logPrintLogger = new Anki::Util::AndroidLogPrintLogger(LOG_PROCNAME);
  loggers.push_back(logPrintLogger);

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

  logPrintLogger->PrintLogD(LOG_PROCNAME, "CozmoStart.ResourcesPath", {}, resourcesPath.c_str());

  #if (USE_DAS || DEV_LOGGER_ENABLED)
  const std::string& appRunId = Anki::Util::GetUUIDString();
  #endif

  // Initialize logging
  #if DEV_LOGGER_ENABLED
  if(!FACTORY_TEST || (FACTORY_TEST && !Anki::Cozmo::Factory::GetEMR()->fields.PACKED_OUT_FLAG))
  {
    using DevLoggingSystem = Anki::Cozmo::DevLoggingSystem;
    const std::string& devlogPath = gDataPlatform->pathToResource(Anki::Util::Data::Scope::CurrentGameLog, LOG_PROCNAME);
    DevLoggingSystem::CreateInstance(devlogPath, appRunId);
    loggers.push_back(DevLoggingSystem::GetInstancePrintProvider());
  }
  #endif

  #if USE_DAS

  // Initialize DAS configuration with no persistent storage.
  // Message filtering will be determined by configuration file.
  const std::string& dasConfigPath = gDataPlatform->pathToResource(Anki::Util::Data::Scope::Resources, "config/DAS/vic-engine.json");
  DASConfigure(dasConfigPath.c_str(), nullptr, nullptr);

  // Initialize DAS platform settings.
  // This is required for each instance of DAS, even if platform settings
  // are duplicated by multiple services.
  auto dasPlatform = std::make_unique<Anki::VictorDAS::DASPlatform>(appRunId);

  dasPlatform->SetAppVersion("app.version");
  dasPlatform->SetPlatform("vicos");
  dasPlatform->SetOsVersion("VicOS 1.2");
  dasPlatform->SetCombinedSystemVersion("VicOS 1.2.3.456");

  DASNativeInit(std::move(dasPlatform), LOG_PROCNAME);

  // Attach log provider to forward log events into DAS.  DAS will garnish each log event
  // with attributes like apprun and forward to aggregator.
  auto dasLogger = std::make_unique<Anki::VictorDAS::DASLogger>();
  Anki::Util::gEventProvider = dasLogger.get();
  loggers.push_back(dasLogger.release());

  #endif

  Anki::Util::MultiLoggerProvider* loggerProvider = new Anki::Util::MultiLoggerProvider(loggers);
  Anki::Util::gLoggerProvider = loggerProvider;

  // - console filter for logs
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();

    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
    if (!gDataPlatform->readAsJson(Anki::Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      PRINT_NAMED_ERROR("webotsCtrlGameEngine.main.loadConsoleConfig", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
    }

    // initialize console filter for this platform
    const std::string& platformOS = gDataPlatform->GetOSPlatformString();
    const Json::Value& consoleFilterConfigOnPlatform = consoleFilterConfig[platformOS];
    consoleFilter->Initialize(consoleFilterConfigOnPlatform);

    // set filter in the loggers
    std::shared_ptr<const IChannelFilter> filterPtr( consoleFilter );

    // loggerProvider->SetFilter(filterPtr);
  }

  LOG_INFO("cozmo_start", "Creating engine");
  LOG_INFO("cozmo_start",
            "Initialized data platform with persistentPath = %s, cachePath = %s, resourcesPath = %s",
            persistentPath.c_str(), cachePath.c_str(), resourcesPath.c_str());

  configure_engine(config);

  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(gDataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "consoleVarsEngine.ini").c_str());

  Anki::Cozmo::CozmoAPI* engineInstance = new Anki::Cozmo::CozmoAPI();

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
  Anki::Util::gEventProvider = nullptr;
  Anki::Util::SafeDelete(Anki::Util::gLoggerProvider);
#if DEV_LOGGER_ENABLED
  Anki::Cozmo::DevLoggingSystem::DestroyInstance();
#endif
  Anki::Util::SafeDelete(gDataPlatform);

  sync();

  return result;
}

int main(int argc, char* argv[])
{
  // Install signal handler
  signal(SIGTERM, sigterm);

  // FIXME: We need to build Breakpad libs for VICOS
  // static char const* filenamePrefix = "engine";
  // GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);

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
  const char* env_config = getenv("VIC_ENGINE_CONFIG");
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
    return 1;
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
      return (int)1;
    }

    std::string jsonContents = Anki::Util::FileUtils::ReadFile(config_file);
    printf("jsonContents: %s\n", jsonContents.c_str());
    Json::Reader reader;
    if (!reader.parse(jsonContents, config)) {
      PRINT_STREAM_ERROR("cozmo_startup", "json configuration parsing error: " << reader.getFormattedErrorMessages());
      return (int)1;
    }
  }

  int res = cozmo_start(config);
  if (0 != res) {
      printf("failed to start engine\n");
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

  // FIXME: We need to build Breakpad libs for VICOS
  // GoogleBreakpad::UnInstallGoogleBreakpad();

  return res;
}
