//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#ifndef __CozmoGame__csharp_binding__
#define __CozmoGame__csharp_binding__

extern "C" {

    enum BindingError {
        BINDING_OK = 0,
        BINDING_ERROR_NOT_INITIALIZED = 0x8001,
        BINDING_ERROR_ALREADY_INITIALIZED = 0x8002,
        BINDING_ERROR_INVALID_CONFIGURATION = 0x8003,
        BINDING_ERROR_FAILED_INITIALIZATION = 0x8005,
    };
  
    // Determine if any logs are available; if so, set length
    bool cozmo_has_log(int* receive_length);
    
    // Retrieve and remove the next log into the specified buffer with given max length
    void cozmo_pop_log(char* buffer, int max_length);
    
    // Creates a new CozmoEngineHost instance
    int cozmo_game_create(const char* configuration_data);
    
    // Destroys the current CozmoEngineHost instance, if any
    int cozmo_game_destroy();
  
    // Update tick
    int cozmo_game_update(float currentTime);
}

#endif /* defined(__CozmoGame__csharp_binding__) */
