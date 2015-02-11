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

#include "anki/common/types.h"
  
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
    int cozmo_engine_host_create(const char* configurationData, const char* visIP);
    
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
    
    // Stops wheel, head, and lift motors (sets their speeds to zero)
    int cozmo_robot_stop_all_motors(int robot_id);
  
    // Define the message types via macros
    // TODO: Replace with Mark Pauley's auto-generated message types
#   undef MESSAGE_DEFINITION_MODE
#   include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
#   define MESSAGE_DEFINITION_MODE MESSAGE_STRUCT_DEFINITION_MODE
#   include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
  
    // Functions for checking for available messages.
    // Each of these returns true as long as there is a message available and
    // populates the passed-in struct if so.
    bool cozmo_engine_check_for_robot_available(MessageG2U_RobotAvailable* msg);
    bool cozmo_engine_check_for_robot_connected(MessageG2U_RobotConnected* msg);
    bool cozmo_engine_check_for_observed_object(MessageG2U_RobotObservedObject* msg);
}

#endif /* defined(__CozmoGame__csharp_binding__) */
