//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "csharp-binding.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace Anki;
using namespace Anki::Cozmo;

CozmoEngineHost* host;
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

void cozmo_get_log(char* buffer, int* receive_length, int buffer_length)
{
    if (error_messages.empty()) {
        *receive_length = 0;
        return;
    }
    *receive_length = std::min(buffer_length, (int)error_messages.back().length());
    std::memcpy(buffer, error_messages.back().data(), *receive_length);
    error_messages.pop_back();
}

int cozmo_enginehost_create(const char* configurationData)
{
    if (host != nullptr) {
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
    
    host = new CozmoEngineHost();
    return (int)host->Init(config);
}

int cozmo_enginehost_destroy()
{
    if (host != nullptr) {
        delete host;
    }
    return (int)BINDING_OK;
}

int cozmo_enginehost_update(float currentTime)
{
    if (host == nullptr) {
        return (int)BINDING_ERROR_NOT_INITIALIZED;
    }
    return (int)host->Update(currentTime);
}

int cozmo_enginehost_forceaddrobot(int robot_id, const char* robot_ip, bool robot_is_simulated)
{
    if (host == nullptr) {
        return (int)BINDING_ERROR_NOT_INITIALIZED;
    }
    host->ForceAddRobot((u32)robot_id, robot_ip, robot_is_simulated);
    return (int)BINDING_OK;
}

