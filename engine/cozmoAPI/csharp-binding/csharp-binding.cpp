//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

//
// This file is not used on victor. Contents are preserved for reference.
//
#if !defined(VICOS_LA) && !defined(VICOS_LE)

#include "engine/cozmoAPI/csharp-binding/csharp-binding.h"

#include "engine/util/file/archiveUtil.h"
#include "engine/utils/parsingConstants/parsingConstants.h"
#include "engine/cozmoAPI/csharp-binding/dasLoggerProvider.h"
#include "engine/cozmoAPI/cozmoAPI.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "util/fileUtils/fileUtils.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/helpers/ankiDefines.h"
#include "util/logging/channelFilter.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"
#include "util/md5/md5.h"

#include <algorithm>
#include <string>
#include <vector>

#if ANKI_DEV_CHEATS
#include "engine/debug/cladLoggerProvider.h"
#include "engine/debug/devLoggerProvider.h"
#include "engine/debug/devLoggingSystem.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/rollingFileLogger.h"
#endif



#if USE_DAS
#include <DAS/DAS.h>
#include <DAS/DASPlatform.h>
#endif

#if defined(ANKI_PLATFORM_IOS)
#include "engine/cozmoAPI/csharp-binding/ios/ios-binding.h"
#elif defined(ANKI_PLATFORM_ANDROID)
#include "engine/cozmoAPI/csharp-binding/android/android-binding.h"
#include "util/jni/jniUtils.h"
#if USE_DAS
#include <DAS/dasPlatform_android.h>
#endif
#endif

// Where do we store device ID?
#define DEVICE_ID_FILE "uniqueDeviceID.dat"

using namespace Anki;
using namespace Anki::Cozmo;

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";

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
  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT)) {
    config[AnkiUtil::kP_SDK_ON_DEVICE_TCP_PORT] = SDK_ON_DEVICE_TCP_PORT;
  }

}

#if USE_DAS
static const std::string kDASArchiveFileExtension = ".tar.gz";
static bool das_archive_function(const std::string& inputFilePath)
{
  const std::string filename = Anki::Util::FileUtils::GetFileName(inputFilePath);
  const std::string baseDirectory = inputFilePath.substr(0, inputFilePath.size() - filename.size());
  const std::string outputFilePath = baseDirectory + filename + kDASArchiveFileExtension;

  bool success =
  ANKI_VERIFY(Anki::Cozmo::ArchiveUtil::CreateArchiveFromFiles(outputFilePath, baseDirectory, {inputFilePath}),
              "csharp-binding.das_archive_function.CreateArchiveFromFiles.Fail",
              "Unable to create archive at path %s", outputFilePath.c_str());
  if (!success)
  {
    return false;
  }

  Anki::Util::FileUtils::DeleteFile(inputFilePath);
  return true;
}

static std::string das_unarchive_function(const std::string& inputFilePath)
{
  const std::string filename = Anki::Util::FileUtils::GetFileName(inputFilePath);
  const std::string baseDirectory = inputFilePath.substr(0, inputFilePath.size() - filename.size());

  const auto expectedIndexOfExtension = filename.size() - kDASArchiveFileExtension.size();
  const auto actualIndexOfExtension = filename.rfind(kDASArchiveFileExtension);
  ANKI_VERIFY(expectedIndexOfExtension == actualIndexOfExtension,
              "csharp-binding.das_unarchive_function.unexpectedFileExtension",
              "Filename %s missing extension %s", filename.c_str(), kDASArchiveFileExtension.c_str());

  const std::string outputFilename = filename.substr(0, expectedIndexOfExtension);


  bool success =
  ANKI_VERIFY(Anki::Cozmo::ArchiveUtil::CreateFilesFromArchive(inputFilePath, baseDirectory),
              "csharp-binding.das_unarchive_function.CreateFilesFromArchive.Fail",
              "Unable to create files at path %s from archive %s",
              baseDirectory.c_str(), inputFilePath.c_str());
  Anki::Util::FileUtils::DeleteFile(inputFilePath);
  if(!success)
  {
    return "";
  }

  return outputFilename;
}
#endif

static void cozmo_configure_das(const std::string& resourcesBasePath,
                                const Anki::Util::Data::DataPlatform* platform,
                                bool dataCollectionEnabled)
{
#if USE_DAS
  DASSetArchiveLogConfig(std::bind(das_archive_function, std::placeholders::_1),
                         std::bind(das_unarchive_function, std::placeholders::_1),
                         kDASArchiveFileExtension);

  if(dataCollectionEnabled)
  {
    DASEnableNetwork(DASDisableNetworkReason_UserOptOut);
  }
  else
  {
    DASDisableNetwork(DASDisableNetworkReason_UserOptOut);
  }

  std::string dasConfigPath = resourcesBasePath + "/DASConfig.json";
  std::string dasLogPath = platform->pathToResource(Anki::Util::Data::Scope::Cache, "DASLogs");
  std::string gameLogPath = platform->pathToResource(Anki::Util::Data::Scope::CurrentGameLog, "");

  // Does the actual init of the system
  DASConfigure(dasConfigPath.c_str(), dasLogPath.c_str(), gameLogPath.c_str());
#endif
}

static void cozmo_disable_das_networking()
{
#if USE_DAS
  // Prevent uploads to remote storage during shutdown.
  // Note this does not cancel uploads already in progress!
  // DAS worker threads stay active during shutdown.
  DASDisableNetwork(DASDisableNetworkReason_Shutdown);
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
  std::string persistentPath = config["DataPlatformPersistentPath"].asCString();
  std::string cachePath = config["DataPlatformCachePath"].asCString();
  std::string resourcesPath = config["DataPlatformResourcesPath"].asCString();
  std::string resourcesBasePath = config["DataPlatformResourcesBasePath"].asCString();
  std::string appRunId = config["appRunId"].asCString();

  dataPlatform = new Anki::Util::Data::DataPlatform(persistentPath, cachePath, resourcesPath);

  bool dataCollectionEnabled = config["DataCollectionEnabled"].asBool();
  cozmo_configure_das(resourcesBasePath, dataPlatform, dataCollectionEnabled);

  // Initialize logging
  #if ANKI_DEV_CHEATS
    DevLoggingSystem::CreateInstance(dataPlatform->pathToResource(Util::Data::Scope::CurrentGameLog, "devLogger"), appRunId);
    Util::IFormattedLoggerProvider* unityLoggerProvider = new CLADLoggerProvider();
  #endif

  Anki::Util::IEventProvider* eventProvider = nullptr;
  #if USE_DAS
    Util::DasLoggerProvider* dasLoggerProvider = new Util::DasLoggerProvider();
    eventProvider = dasLoggerProvider;
  #endif

  Anki::Util::MultiLoggerProvider*loggerProvider = new Anki::Util::MultiLoggerProvider({
#if USE_DAS
    dasLoggerProvider,
#endif
#if ANKI_DEV_CHEATS
    unityLoggerProvider,
    new DevLoggerProvider(DevLoggingSystem::GetInstance()->GetQueue(),
                            Util::FileUtils::FullFilePath( {DevLoggingSystem::GetInstance()->GetDevLoggingBaseDirectory(), DevLoggingSystem::kPrintName} ))
#endif
  });

  Anki::Util::gLoggerProvider = loggerProvider;
  Anki::Util::gEventProvider = eventProvider;

#if defined(ANKI_PLATFORM_IOS)
  // init DAS among other things
  result = Anki::Cozmo::iOSBinding::cozmo_startup(dataPlatform, appRunId);
  Anki::Cozmo::iOSBinding::update_settings_bundle(appRunId.c_str(), DASGetPlatform()->GetDeviceId());
#elif defined(ANKI_PLATFORM_ANDROID) && USE_DAS
  std::unique_ptr<DAS::DASPlatform_Android> dasPlatform{new DAS::DASPlatform_Android(appRunId, dataPlatform->pathToResource(Anki::Util::Data::Scope::Persistent, DEVICE_ID_FILE))};
  if (config.get("standalone", false).asBool()) {
    auto envWrapper = Anki::Util::JNIUtils::getJNIEnvWrapper();
    JNIEnv* env = envWrapper->GetEnv();
    JNI_CHECK(env);
    Anki::Util::JObjectHandle activity{Anki::Util::JNIUtils::getCurrentActivity(env), env};
    dasPlatform->Init(activity.get());
  } else {
    dasPlatform->InitForUnityPlayer();
  }
  DASNativeInit(std::move(dasPlatform), "cozmo");
#endif
  // DAS is started up with uploading paused, so now that it is initialized we can unpause. This prevents DAS from
  // potentially logging events related to uploading before all initialization data has been set (which can cause
  // malformed DAS events to be created which will be thrown out by the server).
  DASPauseUploadingToServer(false);

#if USE_DAS && ANKI_DEV_CHEATS
  // Now that das has been initialized, update the devlog data with the deviceID
  DevLoggingSystem::GetInstance()->UpdateDeviceId(DASGetPlatform()->GetDeviceId());
#endif

#if USE_DAS
  MD5 playerIDHash(DASGetPlatform()->GetDeviceId());
  Anki::Util::sSetGlobal("$player_id", playerIDHash.hexdigest().c_str());
#endif

  #if USE_DAS
  // try to post to server just in case we have internet at app startup
  auto callback = [] (bool success, std::string response) {
    (void) response;
    LOG_EVENT(success ? "das.upload" : "das.upload.fail", "live");
  };
  DASForceFlushWithCallback(callback);
  #endif


  // - console filter for logs
  {
    using namespace Anki::Util;
    ChannelFilter* consoleFilter = new ChannelFilter();

    // load file config
    Json::Value consoleFilterConfig;
    const std::string& consoleFilterConfigPath = "config/engine/console_filter_config.json";
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

#if ANKI_DEV_CHEATS
    unityLoggerProvider->SetFilter(filterPtr);
    unityLoggerProvider->ParseLogLevelSettings(consoleFilterConfigOnPlatform);
#endif

    #define FILTER_DAS 0 // for local testing only
    #if USE_DAS && FILTER_DAS
      dasLoggerProvider->SetFilter(filterPtr);
    #endif

  }

  PRINT_NAMED_INFO("cozmo_startup", "Creating engine");
  PRINT_NAMED_DEBUG("cozmo_startup", "Initialized data platform with persistentPath = %s, cachePath = %s, resourcesPath = %s", persistentPath.c_str(), cachePath.c_str(), resourcesPath.c_str());

  configure_engine(config);

  // Set up the console vars to load from file, if it exists
  ANKI_CONSOLE_SYSTEM_INIT(dataPlatform->pathToResource(Anki::Util::Data::Scope::Cache, "consoleVarsEngine.ini").c_str());

  Cozmo::CozmoAPI* created_engine = new Cozmo::CozmoAPI();

  bool engineResult = created_engine->StartRun(dataPlatform, config);
  if (! engineResult) {
    delete created_engine;
    return (int)engineResult;
  }

  engineAPI = created_engine;

  return result;
}

//
// cozmo_shutdown() is called to release any resources allocated by cozmo_startup().
// It is called by Cozmo app in response to RobotEngineManager.OnDisable() during application teardown.
//
// Note that some platforms (Android) do not guarantee delivery of application stop events.
// This means that Unity does does not always perform application shutdown as expected.
//
// http://answers.unity3d.com/questions/824790/help-with-onapplicationquit-android.html
// https://developer.android.com/reference/android/app/Activity.html#ProcessLifecycle
//

int cozmo_shutdown()
{
  int result = (int)RESULT_OK;

#if defined(ANKI_PLATFORM_IOS)
  result = Anki::Cozmo::iOSBinding::cozmo_shutdown();
#elif defined(ANKI_PLATFORM_ANDROID)
  result = Anki::Cozmo::AndroidBinding::cozmo_shutdown();
#endif

  cozmo_disable_das_networking();

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

void cozmo_execute_background_transfers()
{
  if (engineAPI == nullptr) {
    return;
  }
  engineAPI->ExecuteBackgroundTransfers();
}

#if defined(ANKI_PLATFORM_ANDROID)
const char * cozmo_get_device_id_file_path(const char * persistentDataPath)
{
  static std::string path;
  path = std::string(persistentDataPath) + DEVICE_ID_FILE;
  return path.c_str();
}
#endif

uint32_t cozmo_activate_experiment(const uint8_t* requestBuffer, size_t requestSize,
                                   uint8_t* responseBuffer, size_t responseSize)
{
  if (engineAPI == nullptr) {
    return 0;
  }
  return engineAPI->ActivateExperiment(requestBuffer, requestSize, responseBuffer, responseSize);
}

#endif
