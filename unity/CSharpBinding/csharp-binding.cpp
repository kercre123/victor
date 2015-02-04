//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "csharp-binding.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/robot.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace Anki;
using namespace Anki::Cozmo;

CozmoEngineHost* host = nullptr;
CozmoEngine* engine = nullptr;
std::deque<std::string> error_messages;

void add_log(std::string log_string)
{
    error_messages.push_back(log_string);
}

bool cozmo_has_log(int* receive_length)
{
    if (error_messages.empty()) {
        *receive_length = 0;
        return false;
    }
    *receive_length = (int)error_messages.back().length();
    return true;
}

void cozmo_pop_log(char* buffer, int buffer_length)
{
    if (error_messages.empty()) {
        return;
    }
    int length = std::min(buffer_length, (int)error_messages.back().length());
    std::memcpy(buffer, error_messages.back().data(), length);
    error_messages.pop_back();
}

int cozmo_engine_host_create(const char* configurationData)
{
    if (engine != nullptr) {
        return (int)BINDING_ERROR_ALREADY_INITIALIZED;
    }
    
    if (configurationData == nullptr) {
        return (int)BINDING_ERROR_INVALID_CONFIGURATION;
    }
    
    Json::Reader reader;
    Json::Value config;
    if (!reader.parse(configurationData, configurationData + std::strlen(configurationData), config)) {
        add_log("Json Parsing Error: " + reader.getFormattedErrorMessages());
        return (int)BINDING_ERROR_INVALID_CONFIGURATION;
    }
    
    CozmoEngineHost* created_host = new CozmoEngineHost();
    
    Result result = created_host->Init(config);
    if (result == RESULT_OK) {
        created_host->ListenForRobotConnections(true);
    }
    if (result != RESULT_OK) {
        delete created_host;
        return result;
    }
    
    host = created_host;
    engine = host;
    return RESULT_OK;
}

int cozmo_engine_host_destroy()
{
    if (host != nullptr) {
        delete host;
        host = nullptr;
        engine = nullptr;
    }
    return (int)BINDING_OK;
}

int cozmo_engine_host_force_add_robot(int robot_id, const char* robot_ip, bool robot_is_simulated)
{
    if (host == nullptr) {
        return (int)BINDING_ERROR_NOT_INITIALIZED;
    }
    
    host->ForceAddRobot((u32)robot_id, robot_ip, robot_is_simulated);
    return (int)BINDING_OK;
}

int cozmo_engine_host_is_robot_connected(bool* is_connected, int robot_id)
{
    if (host == nullptr) {
        return (int)BINDING_ERROR_NOT_INITIALIZED;
    }
    Robot* robot = host->GetRobotByID((u32)robot_id);
    *is_connected = (robot != nullptr);
    
    if (host->ConnectToRobot((u32)robot_id)) {
        robot = host->GetRobotByID((u32)robot_id);
        *is_connected = (robot != nullptr);
    }
    
    return (int)BINDING_OK;
}

int cozmo_engine_update(float currentTime)
{
    if (engine == nullptr) {
        return (int)BINDING_ERROR_NOT_INITIALIZED;
    }
    return (int)engine->Update(currentTime);
}

int cozmo_robot_drive_wheels(int robot_id, float left_wheel_speed_mmps, float right_wheel_speed_mmps)
{
    Robot* robot = host->GetRobotByID((u32)robot_id);
    if (robot == nullptr) {
        return (int)BINDING_ERROR_ROBOT_NOT_READY;
    }
    return (int)robot->DriveWheels((f32)left_wheel_speed_mmps, (f32)right_wheel_speed_mmps);
}

int cozmo_robot_stop_all_motors(int robot_id)
{
    Robot* robot = host->GetRobotByID((u32)robot_id);
    if (robot == nullptr) {
        return (int)BINDING_ERROR_ROBOT_NOT_READY;
    }
    return (int)robot->StopAllMotors();
}


