//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

#ifndef __CSharpBinding__csharp_binding__
#define __CSharpBinding__csharp_binding__

#include <string>

#ifndef _cplusplus
extern "C" {
#endif
    
    // Sets a logging callback, that may be called on any Unity thread.
    // Can call independent of cozmo_startup/cozmo_shutdown.
// may need __stdcall on certain platforms
#define CSHARP_BINDING_CALLING_CONVENTION
//#define CSHARP_BINDING_CALLING_CONVENTION __stdcall
    typedef void (CSHARP_BINDING_CALLING_CONVENTION *LogCallback)(int log_level, const char* buffer);
    int cozmo_set_log_callback(LogCallback callback, int min_log_level);
    
    // Hook for initialization code. If this errors out, do not run anything else.
    int cozmo_startup(const char *configuration_data);
    
    // Hook for deinitialization. Should be fine to call startup after this call, even on failure.
    // Return value is just for informational purposes. Should never fail, even if not initialized.
    int cozmo_shutdown();
    
    // Update tick; only call if cozmo_startup succeeded.
    int cozmo_update(float current_time);

#ifndef _cplusplus
}
#endif

#endif // __CSharpBinding__csharp_binding__
