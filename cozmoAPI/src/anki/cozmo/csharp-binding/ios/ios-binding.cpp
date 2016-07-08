//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "ios-binding.h"
#include "hockeyApp.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "dasConfiguration.h"
#include "wifiConfigure.h"
#include "clipboardPaster.h"

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::iOSBinding;

WifiConfigure* wifiConfigure = nullptr;

int Anki::Cozmo::iOSBinding::cozmo_startup(Anki::Util::Data::DataPlatform* dataPlatform, const std::string& apprun)
{
  ConfigureDASForPlatform(dataPlatform, apprun);
  CreateHockeyApp();
  
  wifiConfigure = new WifiConfigure();

  return RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_shutdown()
{
  // TODO:(lc) We probably don't want or need the wifiConfigure running the entire lifetime of the app, so figure out
  // the lifetime it really needs
  Anki::Util::SafeDelete(wifiConfigure);
  
  return (int)RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  if (nullptr == wifiConfigure)
  {
    PRINT_NAMED_ERROR("iOSBinding.cozmo_engine_wifi_setup", "Tried to setup wifi with no wifiConfigure object created");
    return RESULT_FAIL;
  }
  
  if (!wifiConfigure->UpdateMobileconfig(wifiSSID, wifiPasskey))
  {
    PRINT_NAMED_ERROR("iOSBinding.cozmo_engine_wifi_setup", "Problem updating mobileconfig to be used for wifi configuration");
    return RESULT_FAIL;
  }
  
  wifiConfigure->InstallMobileconfig();
  
  return RESULT_OK;
}

void Anki::Cozmo::iOSBinding::cozmo_engine_send_to_clipboard(const char* log) {
  WriteToClipboard(log);
}
