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

  // Hook for initialization code. If this errors out, do not run anything else.
  int cozmo_startup(const char *configuration_data);
    
  // Hook for deinitialization. Should be fine to call startup after this call, even on failure.
  // Return value is just for informational purposes. Should never fail, even if not initialized.
  int cozmo_shutdown();
    
  // Update tick; only call if cozmo_startup succeeded.
  int cozmo_update(float current_time);
  
  void Unity_DAS_Event(const char* eventName, const char* eventValue);
  
  void Unity_DAS_LogE(const char* eventName, const char* eventValue);
  
  void Unity_DAS_LogW(const char* eventName, const char* eventValue);
  
  void Unity_DAS_LogI(const char* eventName, const char* eventValue);
  
  void Unity_DAS_LogD(const char* eventName, const char* eventValue);

#ifndef _cplusplus
}
#endif

#endif // __CSharpBinding__csharp_binding__
