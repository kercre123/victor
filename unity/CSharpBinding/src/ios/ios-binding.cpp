//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "ios-binding.h"
#include "../csharp-binding.h"
#include "dataPlatformCreator.h"
#include "hockeyApp.h"
#include "anki/cozmo/cozmoAPI.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/helpers/templateHelpers.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "dasLoggerProvider.h"
#include "dasConfiguration.h"
#include "wifiConfigure.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::CSharpBinding;

CozmoAPI* engineAPI = nullptr;
Anki::Util::Data::DataPlatform* dataPlatform = nullptr;

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";

WifiConfigure* wifiConfigure = nullptr;

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

int Anki::Cozmo::CSharpBinding::cozmo_engine_create(const char* configuration_data)
{
  Anki::Util::DasLoggerProvider* loggerProvider = new Anki::Util::DasLoggerProvider();
  Anki::Util::gLoggerProvider = loggerProvider;
  Anki::Util::gEventProvider = loggerProvider;
  PRINT_NAMED_INFO("CSharpBinding.cozmo_game_create", "engine creating engine");

  dataPlatform = CreateDataPlatform();
  ConfigureDASForPlatform(dataPlatform);
  CreateHockeyApp();
  
  using namespace Cozmo;

  if (engineAPI != nullptr) {
      PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Game already initialized.");
      return (int)RESULT_FAIL;
  }
  
  if (configuration_data == nullptr) {
      PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Null pointer for configuration_data.");
      return (int)RESULT_FAIL_INVALID_PARAMETER;
  }
  
  Json::Reader reader;
  Json::Value config;
  if (!reader.parse(configuration_data, configuration_data + std::strlen(configuration_data), config)) {
      PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "json configuration parsing error: " << reader.getFormattedErrorMessages());
      return (int)RESULT_FAIL;
  }

  configure_engine(config);

  CozmoAPI* created_engine = new CozmoAPI();

  bool result = created_engine->StartRun(dataPlatform, config);
  if (! result) {
    delete created_engine;
    return (int)result;
  }
  
  engineAPI = created_engine;
  
  wifiConfigure = new WifiConfigure();

  return RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_engine_destroy()
{
  // TODO:(lc) We probably don't want or need the wifiConfigure running the entire lifetime of the app, so figure out
  // the lifetime it really needs
  Anki::Util::SafeDelete(wifiConfigure);
  
  Anki::Util::SafeDelete(engineAPI);
  Anki::Util::SafeDelete(Anki::Util::gLoggerProvider);
  Anki::Util::SafeDelete(dataPlatform);
  return (int)RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  if (nullptr == wifiConfigure)
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
  
  return RESULT_OK;
}
