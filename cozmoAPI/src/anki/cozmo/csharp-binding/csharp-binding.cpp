//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

#include "anki/cozmo/csharp-binding/csharp-binding.h"

#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/csharp-binding/dasLoggerProvider.h"
#include "anki/cozmo/cozmoAPI.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/helpers/ankiDefines.h"
#include "util/logging/channelFilter.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/sosLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"

#include <algorithm>
#include <string>
#include <vector>

#if ANKI_DEV_CHEATS
#include "anki/cozmo/basestation/debug/devLoggerProvider.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#endif

#if defined(ANKI_PLATFORM_IOS)
#include "anki/cozmo/csharp-binding/ios/ios-binding.h"
#elif defined(ANKI_PLATFORM_ANDROID)
#include "anki/cozmo/csharp-binding/android/android-binding.h"
#if USE_DAS
#include <DAS/dasPlatform_android.h>
#endif
#endif

using namespace Anki;
using namespace Anki::Cozmo;

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";

CozmoAPI* engineAPI = nullptr;
Anki::Util::Data::DataPlatform* dataPlatform = nullptr;

void Unity_DAS_Event(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sEventF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogE(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sErrorF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogW(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sWarningF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogI(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sChanneledInfoF("Unity", eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogD(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sChanneledDebugF("Unity", eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_Ch_LogI(const char* channelName, const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount)
{
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sChanneledInfoF(channelName, eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_Ch_LogD(const char* channelName, const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount)
{
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sChanneledDebugF(channelName, eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_SetGlobal(const char* key, const char* value)
{
  Anki::Util::sSetGlobal(key,value);
}

void configure_engine(Json::Value& config)
{
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
  if(!config.isMember(AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT)) {
    config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = SDK_ON_DEVICE_TCP_PORT;
  }
  
}

static void cozmo_configure_das(const std::string& resourcesBasePath, const Anki::Util::Data::DataPlatform* platform)
{
#if USE_DAS
  std::string dasConfigPath = resourcesBasePath + "/DASConfig.json";
  std::string dasLogPath = platform->pathToResource(Anki::Util::Data::Scope::Cache, "DASLogs");
  std::string gameLogPath = platform->pathToResource(Anki::Util::Data::Scope::CurrentGameLog, "");
  DASConfigure(dasConfigPath.c_str(), dasLogPath.c_str(), gameLogPath.c_str());
#endif
}

void cozmo_install_google_breakpad(const char* path)
{
  #ifdef ANDROID
  Anki::Cozmo::AndroidBinding::InstallGoogleBreakpad(path);
  #endif
}

void cozmo_uninstall_google_breakpad()
{
  #ifdef ANDROID
  Anki::Cozmo::AndroidBinding::UnInstallGoogleBreakpad();
  #endif
}

int cozmo_startup(const char *configuration_data)
{
  int result = (int)RESULT_OK;
  
  if (engineAPI != nullptr) {
      PRINT_STREAM_ERROR("cozmo_startup", "Game already initialized.");
      return (int)RESULT_FAIL;
  }
  
  Json::Reader reader;
  Json::Value config;
  if (!reader.parse(configuration_data, configuration_data + std::strlen(configuration_data), config)) {
      PRINT_STREAM_ERROR("cozmo_startup", "json configuration parsing error: " << reader.getFormattedErrorMessages());
      return (int)RESULT_FAIL;
  }

  // Create the data platform with the folders sent from Unity
  std::string filesPath = config["DataPlatformFilesPath"].asCString();
  std::string cachePath = config["DataPlatformCachePath"].asCString();
  std::string externalPath = config["DataPlatformExternalPath"].asCString();
  std::string resourcesPath = config["DataPlatformResourcesPath"].asCString();
  std::string resourcesBasePath = config["DataPlatformResourcesBasePath"].asCString();
  std::string appRunId = config["appRunId"].asCString();

  dataPlatform = new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcesPath);

  cozmo_configure_das(resourcesBasePath, dataPlatform);

  // Initialize logging
  #if ANKI_DEV_CHEATS
    DevLoggingSystem::CreateInstance(dataPlatform->pathToResource(Util::Data::Scope::CurrentGameLog, ""), appRunId);
  #endif
  
  Anki::Util::IEventProvider* eventProvider = nullptr;
  #if USE_DAS
    Util::DasLoggerProvider* dasLoggerProvider = new Util::DasLoggerProvider();
    eventProvider = dasLoggerProvider;
  #endif
  
  Util::IFormattedLoggerProvider* sosLoggerProvider = new Util::SosLoggerProvider();
  Anki::Util::MultiLoggerProvider*loggerProvider = new Anki::Util::MultiLoggerProvider({
    sosLoggerProvider
#if USE_DAS
    , dasLoggerProvider
#endif
#if ANKI_DEV_CHEATS
    , new DevLoggerProvider(DevLoggingSystem::GetInstance()->GetQueue(),
                            Util::FileUtils::FullFilePath( {DevLoggingSystem::GetInstance()->GetDevLoggingBaseDirectory(), DevLoggingSystem::kPrintName} ))
#endif
  });

  Anki::Util::gLoggerProvider = loggerProvider;
  Anki::Util::gEventProvider = eventProvider;

#if defined(ANKI_PLATFORM_IOS)
  // init DAS among other things
  result = Anki::Cozmo::iOSBinding::cozmo_startup(dataPlatform, appRunId);
#elif defined(ANKI_PLATFORM_ANDROID) && USE_DAS
  std::unique_ptr<DAS::DASPlatform_Android> dasPlatform{new DAS::DASPlatform_Android(appRunId, dataPlatform->pathToResource(Anki::Util::Data::Scope::Persistent, "uniqueDeviceID.dat"))};
  dasPlatform->InitForUnityPlayer();
  DASNativeInit(std::move(dasPlatform), "cozmo");
#endif

  #if USE_DAS
  // try to post to server just in case we have internet at app startup
  auto callback = [] (bool success) {
    PRINT_NAMED_EVENT(success ? "das.upload" : "das.upload.fail", "live");
  };
  DASForceFlushWithCallback(callback);
  #endif


  // - console filter for logs
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();
    
    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/basestation/config/console_filter_config.json";
    if (!dataPlatform->readAsJson(Util::Data::Scope::Resources, consoleFilterConfigPath, consoleFilterConfig))
    {
      PRINT_NAMED_ERROR("webotsCtrlGameEngine.main.loadConsoleConfig", "Failed to parse Json file '%s'", consoleFilterConfigPath.c_str());
    }
    
    // initialize console filter for this platform
    const std::string& platformOS = dataPlatform->GetOSPlatformString();
    const Json::Value& consoleFilterConfigOnPlatform = consoleFilterConfig[platformOS];
    consoleFilter->Initialize(consoleFilterConfigOnPlatform);
    
    // set filter in the loggers
    std::shared_ptr<const IChannelFilter> filterPtr( consoleFilter );
    sosLoggerProvider->SetFilter(filterPtr);
    
    // also parse additional info for providers
    sosLoggerProvider->ParseLogLevelSettings(consoleFilterConfigOnPlatform);
    
    #define FILTER_DAS 0 // for local testing only
    #if USE_DAS && FILTER_DAS
      dasLoggerProvider->SetFilter(filterPtr);
    #endif
    
  }
  
  PRINT_NAMED_INFO("cozmo_startup", "Creating engine");
  PRINT_NAMED_DEBUG("cozmo_startup", "Initialized data platform with filesPath = %s, cachePath = %s, externalPath = %s, resourcesPath = %s", filesPath.c_str(), cachePath.c_str(), externalPath.c_str(), resourcesPath.c_str());

  configure_engine(config);
  
  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "consoleVars.ini").c_str());
  NativeAnkiUtilConsoleLoadVars();

  Cozmo::CozmoAPI* created_engine = new Cozmo::CozmoAPI();

  bool engineResult = created_engine->StartRun(dataPlatform, config);
  if (! engineResult) {
    delete created_engine;
    return (int)engineResult;
  }
  
  engineAPI = created_engine;
  
  return result;
}

int cozmo_shutdown()
{
  int result = (int)RESULT_OK;
    
#if defined(ANKI_PLATFORM_IOS)
    result = Anki::Cozmo::iOSBinding::cozmo_shutdown();
#endif
  
  Anki::Util::SafeDelete(engineAPI);
  Anki::Util::gEventProvider = nullptr;
  Anki::Util::SafeDelete(Anki::Util::gLoggerProvider);
#if ANKI_DEV_CHEATS
  DevLoggingSystem::DestroyInstance();
#endif
  Anki::Util::SafeDelete(dataPlatform);

  cozmo_uninstall_google_breakpad();

  return result;
}

int cozmo_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  int result = (int)RESULT_OK;
  
#if defined(ANKI_PLATFORM_IOS)
  result = Anki::Cozmo::iOSBinding::cozmo_engine_wifi_setup(wifiSSID, wifiPasskey);
#endif
  
  return result;
}

void cozmo_send_to_clipboard(const char* log) {
#if defined(ANKI_PLATFORM_IOS)
  Anki::Cozmo::iOSBinding::cozmo_engine_send_to_clipboard(log);
#endif
}

size_t cozmo_transmit_engine_to_game(uint8_t* buffer, const size_t size)
{
  if (engineAPI == nullptr) {
    return 0;
  }
  return engineAPI->SendMessages(buffer, size);
}

void cozmo_transmit_game_to_engine(const uint8_t* buffer, const size_t size)
{
  if (engineAPI == nullptr) {
    return;
  }
  engineAPI->ReceiveMessages(buffer, size);
}

size_t cozmo_transmit_viz_to_game(uint8_t* const buffer, const size_t size)
{
  if (engineAPI == nullptr) {
    return 0;
  }
  return engineAPI->SendVizMessages(buffer, size);
}

void cozmo_execute_background_transfers()
{
  if (engineAPI == nullptr) {
    return;
  }
  engineAPI->ExecuteBackgroundTransfers();
}
