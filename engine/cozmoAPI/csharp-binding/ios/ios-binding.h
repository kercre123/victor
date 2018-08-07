//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#ifndef __ios_binding__
#define __ios_binding__

#include <string>

namespace Anki {
    
namespace Util {
namespace Data {
    class DataPlatform;
}
}

namespace Vector {
namespace iOSBinding {

// iOS specific initialization
int cozmo_startup(Anki::Util::Data::DataPlatform* dataPlatform, const std::string& apprun);

// iOS specific finalization
int cozmo_shutdown();
  
int cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey);
  
void cozmo_engine_send_to_clipboard(const char* log);

// Update the iOS app settings
void update_settings_bundle(const char* appRunId, const char* deviceId);

} // namespace iOSBinding
} // namespace Vector
} // namespace Anki

#endif // __ios_binding__
