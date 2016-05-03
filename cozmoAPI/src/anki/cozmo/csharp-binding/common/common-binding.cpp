//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "anki/cozmo/csharp-binding/csharp-binding.h"
#include "anki/cozmo/csharp-binding/common/common-binding.h"
#include "anki/cozmo/csharp-binding/common/dataPlatformCreator.h"
//#include "hockeyApp.h"
#include "anki/cozmo/cozmoAPI.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/helpers/templateHelpers.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/logging/sosLoggerProvider.h"
#include "util/logging/multiLoggerProvider.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
//#include "dasLoggerProvider.h"
//#include "dasConfiguration.h"
//#include "wifiConfigure.h"
#include <algorithm>
#include <string>
#include <vector>
//#include "clipboardPaster.h"

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::CSharpBinding;

CozmoAPI* engineAPI = nullptr;
Anki::Util::Data::DataPlatform* dataPlatform = nullptr;

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";

//WifiConfigure* wifiConfigure = nullptr;

void configure_engine(Json::Value config)
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
  
  config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
  config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 0;
}

#if defined(ANDROID)
#include <android/log.h>
#define LOGI( message, args... ) __android_log_print( ANDROID_LOG_INFO, "CozmoEngine", message, ##args )
#else
#define LOGI(message, args...)
#endif

std::string parseString(const Json::Value& config, const char* key)
{
  if (!config.isMember(key)) {
    LOGI("Can't find key %s in the config", key);
    return std::string();
  } else {
    const char* str = config[key].asCString();
    return std::string(str);
  }
}

int Anki::Cozmo::CSharpBinding::cozmo_engine_create(const char* configuration_data)
{
  LOGI("cozmo_engine_create");
  
  //Anki::Util::MultiLoggerProvider*loggerProvider = new Anki::Util::MultiLoggerProvider({new Util::SosLoggerProvider(), new Util::DasLoggerProvider()});
  Anki::Util::PrintfLoggerProvider* loggerProvider = new Anki::Util::PrintfLoggerProvider();
  Anki::Util::gLoggerProvider = loggerProvider;

  PRINT_NAMED_INFO("CSharpBinding.cozmo_game_create", "engine creating engine");

  LOGI("about to parse config %s", configuration_data);

  Json::Reader reader;
  Json::Value config;
  if (!reader.parse(configuration_data, configuration_data + std::strlen(configuration_data), config)) {
    PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "json configuration parsing error: " << reader.getFormattedErrorMessages());
    return (int)RESULT_FAIL;
  }
  
  //dataPlatform = CreateDataPlatform();
  //ConfigureDASForPlatform(dataPlatform);
  //CreateHockeyApp();
  
#if defined(ANDROID)
  LOGI("about to read paths");

  std::string filesPath = parseString(config, "DataPlatformFilesPath");
  std::string cachePath = parseString(config, "DataPlatformCachePath");
  std::string externalPath = parseString(config, "DataPlatformExternalPath");
  std::string resourcesPath = parseString(config, "DataPlatformResourcesPath");
    
  PRINT_NAMED_DEBUG("DataPlatform", "filesPath = %s, cachePath = %s, externalPath = %s, resourcesPath = %s",
                      filesPath.c_str(), cachePath.c_str(), externalPath.c_str(), resourcesPath.c_str());
    
  dataPlatform = new Anki::Util::Data::DataPlatform(filesPath, cachePath, externalPath, resourcesPath);
#endif
  
  LOGI("Paths read");

  using namespace Cozmo;

  if (engineAPI != nullptr) {
      PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Game already initialized.");
      return (int)RESULT_FAIL;
  }
  
  if (configuration_data == nullptr) {
      PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Null pointer for configuration_data.");
      return (int)RESULT_FAIL_INVALID_PARAMETER;
  }
  
  configure_engine(config);

  CozmoAPI* created_engine = new CozmoAPI();

  bool result = created_engine->StartRun(dataPlatform, config);
  if (! result) {
    delete created_engine;
    return (int)result;
  }
  
  engineAPI = created_engine;
  
  //wifiConfigure = new WifiConfigure();

  return RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_engine_destroy()
{
  // TODO:(lc) We probably don't want or need the wifiConfigure running the entire lifetime of the app, so figure out
  // the lifetime it really needs
  //Anki::Util::SafeDelete(wifiConfigure);
  
  Anki::Util::SafeDelete(engineAPI);
  Anki::Util::SafeDelete(Anki::Util::gLoggerProvider);
  Anki::Util::SafeDelete(dataPlatform);
  return (int)RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  /*if (nullptr == wifiConfigure)
  {
    PRINT_NAMED_ERROR("CSharpBinding.cozmo_engine_wifi_setup", "Tried to setup wifi with no wifiConfigure object created");
    return RESULT_FAIL;
  }
  
  if (!wifiConfigure->UpdateMobileconfig(wifiSSID, wifiPasskey))
  {
    PRINT_NAMED_ERROR("CSharpBinding.cozmo_engine_wifi_setup", "Problem updating mobileconfig to be used for wifi configuration");
    return RESULT_FAIL;
  }
  
  wifiConfigure->InstallMobileconfig();
  */
  return RESULT_OK;
}

void Anki::Cozmo::CSharpBinding::cozmo_engine_send_to_clipboard(const char* log) {
  //WriteToClipboard(log);
}
