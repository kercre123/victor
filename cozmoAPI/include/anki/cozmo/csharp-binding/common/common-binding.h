//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#ifndef __CSharpBinding__common_binding__
#define __CSharpBinding__common_binding__

namespace Anki {
namespace Cozmo {
namespace CSharpBinding {

// Creates a new CozmoEngineHost instance
int cozmo_engine_create(const char* configuration_data);

// Destroys the current CozmoEngineHost instance, if any
int cozmo_engine_destroy();
  
int cozmo_engine_wifi_setup(const char* wifiSSID, const char* wifiPasskey);
  
void cozmo_engine_send_to_clipboard(const char* log);

} // namespace CSharpBinding
} // namespace Cozmo
} // namespace Anki

#endif // __CSharpBinding__ios_binding__
