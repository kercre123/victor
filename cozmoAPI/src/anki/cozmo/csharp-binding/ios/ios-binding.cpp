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
#include "clipboardPaster.h"

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::iOSBinding;


int Anki::Cozmo::iOSBinding::cozmo_startup(Anki::Util::Data::DataPlatform* dataPlatform, const std::string& apprun)
{
  ConfigureDASForPlatform(dataPlatform, apprun);
  CreateHockeyApp();

  return RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_shutdown()
{
  return (int)RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  return RESULT_OK;
}

void Anki::Cozmo::iOSBinding::cozmo_engine_send_to_clipboard(const char* log) {
  WriteToClipboard(log);
}
