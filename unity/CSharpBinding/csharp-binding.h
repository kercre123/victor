//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#ifndef __CozmoGame__csharp_binding__
#define __CozmoGame__csharp_binding__

#include "anki/cozmo/basestation/cozmoEngine.h"

extern "C" {
    
    enum BindingError {
        BINDING_OK = 0,
        BINDING_ERROR_NOT_INITIALIZED = 0x8001,
        BINDING_ERROR_ALREADY_INITIALIZED = 0x8002,
        BINDING_ERROR_INVALID_CONFIGURATION = 0x8003,
        // also include Anki::Result values
    };
    
    bool cozmo_has_log(int* receive_length);
    
    void cozmo_get_log(char* buffer, int max_length);
    
    int cozmo_enginehost_create(const char* configurationData);
    
    int cozmo_enginehost_destroy();
    
    int cozmo_enginehost_update(float currentTime);
    
    int cozmo_enginehost_forceaddrobot(int robot_id, const char* robot_ip, bool robot_is_simulated);
    
}

#endif /* defined(__CozmoGame__csharp_binding__) */
