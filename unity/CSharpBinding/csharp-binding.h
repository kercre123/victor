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
        BINDING_ERROR_ROBOT_NOT_FOUND = 0x8004,
        // also include Anki::Result values
    };
    
    bool cozmo_has_log(int* receive_length);
    
    void cozmo_pop_log(char* buffer, int max_length);
    
    int cozmo_engine_host_create(const char* configurationData);
    
    int cozmo_engine_host_destroy();
    
    int cozmo_engine_host_force_add_robot(int robot_id, const char* robot_ip, bool robot_is_simulated);
    
    int cozmo_engine_update(float currentTime);
    
    int cozmo_robot_drive_wheels(float left_wheel_speed_mmps, float right_wheel_speed_mmps);
    
    int cozmo_robot_stop_all_motors();
}

#endif /* defined(__CozmoGame__csharp_binding__) */
