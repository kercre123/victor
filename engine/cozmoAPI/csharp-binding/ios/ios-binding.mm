//
//  ios-binding.mm
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
#include "util/string/stringUtils.h"
#include "dasConfiguration.h"
#include "clipboardPaster.h"

#import <Foundation/Foundation.h>


using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::iOSBinding;


int Anki::Cozmo::iOSBinding::cozmo_startup(Anki::Util::Data::DataPlatform* dataPlatform, const std::string& apprun)
{
  ConfigureDASForPlatform(dataPlatform, apprun);
  EnableHockeyApp();
  return RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_shutdown()
{
  DisableHockeyApp();
  return RESULT_OK;
}

int Anki::Cozmo::iOSBinding::cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  return RESULT_OK;
}

void Anki::Cozmo::iOSBinding::cozmo_engine_send_to_clipboard(const char* log) {
  WriteToClipboard(log);
}

void Anki::Cozmo::iOSBinding::update_settings_bundle(const char* appRunId, const char* deviceId)
{
  NSString *marketingVersion = [[NSBundle mainBundle]
    objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
  NSString *buildVersion = [[NSBundle mainBundle]
    objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
  
  NSString *settingsVersion = [NSString stringWithFormat:@"%@ (%@)",
    marketingVersion, buildVersion];
  [[NSUserDefaults standardUserDefaults] setObject:settingsVersion forKey:@"anki_marketing_version"];
  
  static const int kIdTruncatedStringLen = 13;
  
  std::string shortDeviceIdStr(deviceId);
  shortDeviceIdStr = shortDeviceIdStr.substr(0, kIdTruncatedStringLen); // To fit on screen, as OD does
  shortDeviceIdStr = Anki::Util::StringToUpper(shortDeviceIdStr);
  NSString *NSdeviceId = [NSString stringWithUTF8String:shortDeviceIdStr.c_str()];
  [[NSUserDefaults standardUserDefaults] setObject:NSdeviceId forKey:@"anki_device_id"];

  std::string shortAppRunIdStr(appRunId);
  shortAppRunIdStr = shortAppRunIdStr.substr(0, kIdTruncatedStringLen);
  shortAppRunIdStr = Anki::Util::StringToUpper(shortAppRunIdStr);
  NSString *shortAppRunId = [NSString stringWithUTF8String:shortAppRunIdStr.c_str()];
  [[NSUserDefaults standardUserDefaults] setObject:shortAppRunId forKey:@"anki_apprun_id"];
}
