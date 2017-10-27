#include "json/json.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "engine/cozmoAPI/cozmoAPI.h"
#include "engine/utils/parsingConstants/parsingConstants.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/androidLogPrintLogger_android.h"
#include "util/logging/logging.h"
#include "util/logging/iFormattedLoggerProvider.h"
#include "util/string/stringUtils.h"

#include "util/logging/multiLoggerProvider.h"
#include "util/logging/channelFilter.h"
#include "util/helpers/templateHelpers.h"

#if !defined(DEV_LOGGER_ENABLED)
  #if defined(FACTORY_TEST)
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
#include <android/log.h>
#include <getopt.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";
const char* LOGNAME = "engine";
                                                                                                    
Anki::Cozmo::CozmoAPI* gEngineAPI = nullptr;
Anki::Util::Data::DataPlatform* gDataPlatform = nullptr;

void configure_engine(Json::Value& config)
{
  if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
    config[AnkiUtil::kP_VIZ_HOST_IP] = VIZ_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = Anki::Cozmo::ROBOT_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = Anki::Cozmo::UI_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT)) {
    config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = Anki::Cozmo::SDK_ON_DEVICE_TCP_PORT;
  }
  
}

Anki::Util::Data::DataPlatform* createPlatform(const std::string& filesPath,
                                         const std::string& cachePath,
                                         const std::string& externalPath,
                                         const std::string& resourcesPath)
{
    Anki::Util::FileUtils::CreateDirectory(filesPath);
    Anki::Util::FileUtils::CreateDirectory(cachePath);
    Anki::Util::FileUtils::CreateDirectory(externalPath);
    Anki::Util::FileUtils::CreateDirectory(resourcesPath);

    return new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcesPath);
}

std::string createResourcesPath(const std::string& resourcesBasePath)
{
  std::string resourcesRefPath = resourcesBasePath + "/current";
  std::string resourcesRef = Anki::Util::FileUtils::ReadFile(resourcesRefPath);
  {
    auto it = std::find_if(resourcesRef.rbegin(), resourcesRef.rend(),
          [](char ch){ return !std::iswspace(ch); });
    resourcesRef.erase(it.base() , resourcesRef.end());
  }
  return resourcesBasePath + "/" + resourcesRef + "/cozmo_resources";
}

void getAndroidPlatformPaths(std::string& filesPath,
                             std::string& cachePath,
                             std::string& externalPath,
                             std::string& resourcesPath,
                             std::string& resourcesBasePath)
{
  filesPath = "/data/data/com.anki.cozmoengine/files";
  cachePath = "/data/data/com.anki.cozmoengine/cache";
  externalPath = "/sdcard/Android/data/com.anki.cozmoengine/files";
  resourcesBasePath = externalPath + "/assets";
  resourcesPath = createResourcesPath(resourcesBasePath);
}

int cozmo_start(const Json::Value& configuration)
{
  int result = 0;
  
  if (gEngineAPI != nullptr) {
      PRINT_STREAM_ERROR("cozmo_startup", "Game already initialized.");
      return 1;
  }

  // Build up a list of enabled log providers
  std::vector<Anki::Util::ILoggerProvider*> loggers;

  Anki::Util::AndroidLogPrintLogger * logPrintLogger = new Anki::Util::AndroidLogPrintLogger(LOGNAME);
  loggers.push_back(logPrintLogger);

  std::string filesPath;
  std::string cachePath;
  std::string externalPath;
  std::string resourcesPath;
  std::string resourcesBasePath;
  
  getAndroidPlatformPaths(filesPath, cachePath, externalPath, resourcesPath, resourcesBasePath);

  // copy existing configuration data
  Json::Value config(configuration);

  if (config.isMember("DataPlatformFilesPath")) {
    filesPath = config["DataPlatformFilesPath"].asCString();
  } else {
    config["DataPlatformFilesPath"] = filesPath;
  }

  if (config.isMember("DataPlatformCachePath")) {
    cachePath = config["DataPlatformCachePath"].asCString();
  } else {
    config["DataPlatformCachePath"] = cachePath;
  }

  if (config.isMember("DataPlatformExternalPath")) {
    externalPath = config["DataPlatformExternalPath"].asCString();
  } else {
    config["DataPlatformExternalPath"] = externalPath;
  }

  if (config.isMember("DataPlatformResourcesBasePath")) {
    resourcesBasePath = config["DataPlatformResourcesBasePath"].asCString();
  } else {
    resourcesBasePath = externalPath + "/assets";
    config["DataPlatformResourcesBasePath"] = resourcesBasePath;
  }

  if (config.isMember("DataPlatformResourcesPath")) {
    resourcesPath = config["DataPlatformResourcesPath"].asCString();
  } else {
    resourcesPath = createResourcesPath(resourcesBasePath);
    config["DataPlatformResourcesPath"] = resourcesPath;
  }

  gDataPlatform = createPlatform(filesPath, cachePath, externalPath, resourcesPath);

  logPrintLogger->PrintLogD(LOGNAME, "CozmoStart.ResourcesPath", {}, resourcesPath.c_str());

  // Initialize logging
  #if DEV_LOGGER_ENABLED
    using DevLoggingSystem = Anki::Cozmo::DevLoggingSystem;
    const std::string& appRunId = Anki::Util::GetUUIDString();
    const std::string& devlogPath = gDataPlatform->pathToResource(Anki::Util::Data::Scope::CurrentGameLog, LOGNAME);
    DevLoggingSystem::CreateInstance(devlogPath, appRunId);
    loggers.push_back(DevLoggingSystem::GetInstancePrintProvider());
  #endif

  Anki::Util::IEventProvider* eventProvider = nullptr;
  Anki::Util::MultiLoggerProvider* loggerProvider = new Anki::Util::MultiLoggerProvider(loggers);

  Anki::Util::gLoggerProvider = loggerProvider;
  Anki::Util::gEventProvider = eventProvider;

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

    // logcatProvider->SetFilter(filterPtr);
  }
  
  PRINT_NAMED_INFO("cozmo_startup", "Creating engine");
  PRINT_NAMED_INFO("cozmo_startup",
                    "Initialized data platform with filesPath = %s, cachePath = %s, externalPath = %s, resourcesPath = %s",
                    filesPath.c_str(), cachePath.c_str(), externalPath.c_str(), resourcesPath.c_str());

  configure_engine(config);
  
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(gDataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "consoleVars.ini").c_str());
  NativeAnkiUtilConsoleLoadVars();

  Anki::Cozmo::CozmoAPI* engineInstance = new Anki::Cozmo::CozmoAPI();

  bool engineResult = engineInstance->StartRun(gDataPlatform, config);
  if (! engineResult) {
    delete engineInstance;
    return (int)engineResult;
  }
  
  gEngineAPI = engineInstance;
  
  return result;
}

int cozmo_stop()
{
  int result = (int)0;
    
  Anki::Util::SafeDelete(gEngineAPI);
  Anki::Util::gEventProvider = nullptr;
  Anki::Util::SafeDelete(Anki::Util::gLoggerProvider);
#if DEV_LOGGER_ENABLED
  Anki::Cozmo::DevLoggingSystem::DestroyInstance();
#endif
  Anki::Util::SafeDelete(gDataPlatform);

  return result;
}

int main(int argc, char* argv[])
{
    char cwd[PATH_MAX] = { 0 };
    getcwd(cwd, sizeof(cwd));
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
    const char* env_config = getenv("COZMO_ENGINE_CONFIG");
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
        printf("failed to start cozmoengine\n");
        exit(res);
    }

    printf("cozmoengine started\n");
    
    while(true) {
        usleep(2000);
    }

    return res;
}
