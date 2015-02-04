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
        BINDING_ERROR_ROBOT_NOT_READY = 0x8004,
        // also include Anki::Result values
    };
    
    // Determine if any logs are available; if so, set length
    bool cozmo_has_log(int* receive_length);
    
    // Retrieve and remove the next log into the specified buffer with given max length
    void cozmo_pop_log(char* buffer, int max_length);
    
    // Creates a new CozmoEngineHost instance
    int cozmo_engine_host_create(const char* configurationData);
    
    // Destroys the current CozmoEngineHost instance, if any
    int cozmo_engine_host_destroy();
    
    // Forcibly adds a new robot (non-advertised)
    int cozmo_engine_host_force_add_robot(int robot_id, const char* robot_ip, bool robot_is_simulated);
    
    // Determines whether a specified robot is connected yet
    int cozmo_engine_host_is_robot_connected(bool* is_connected, int robot_id);
    
    // Update tick
    int cozmo_engine_update(float currentTime);
    
    // Set the speed of each wheel
    int cozmo_robot_drive_wheels(int robot_id, float left_wheel_speed_mmps, float right_wheel_speed_mmps);
    
    // not 100% sure what this does. set all motor speeds to 0?
    int cozmo_robot_stop_all_motors(int robot_id);
}

#endif /* defined(__CozmoGame__csharp_binding__) */
