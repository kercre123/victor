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

namespace Anki {
namespace Cozmo {
namespace CSharpBinding {

// Update tick
void cozmo_add_log(const std::string& message);

} // namespace CSharpBinding
} // namespace Cozmo
} // namespace Anki

#ifndef _cplusplus
extern "C" {
#endif

    // Determines if any logs are available; if so, set length.
    // Exists independently of startup/shutdown.
    bool cozmo_has_log(int* receive_length);
    
    // Retrieves and remove the next log into the specified buffer with given max length.
    // Exists independentkly of startup/shutdown.
    void cozmo_pop_log(char* buffer, int max_length);
    
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
