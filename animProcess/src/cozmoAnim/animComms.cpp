/**
 * File: cozmoAnim/animComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 7/30/2017
 *
 * Description: Create sockets and manages low-level data transfer to engine and robot processes
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "cozmoAnim/animComms.h"
#include "osState/osState.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/messaging/shared/LocalUdpClient.h"
#include "coretech/messaging/shared/LocalUdpServer.h"
#include "coretech/messaging/shared/socketConstants.h"
#include "util/logging/logging.h"

#include <stdio.h>

// Log options
#define LOG_CHANNEL                    "AnimComms"

// Trace options
// #define LOG_TRACE(name, format, ...)   LOG_DEBUG(name, format, ##__VA_ARGS__)
#define LOG_TRACE(name, format, ...)   {}

namespace Anki {
namespace Vector {
namespace AnimComms {

namespace { // "Private members"

  // For comms with engine
  LocalUdpServer _engineComms;

  // For comms with robot
  LocalUdpClient _robotComms;
}


Result InitRobotComms()
{
  const RobotID_t robotID = OSState::getInstance()->GetRobotID();
  const std::string & client_path = std::string(Victor::ANIM_ROBOT_CLIENT_PATH) + std::to_string(robotID);
  const std::string & server_path = std::string(Victor::ANIM_ROBOT_SERVER_PATH) + std::to_string(robotID);

  LOG_INFO("AnimComms.InitRobotComms", "Connect from %s to %s", client_path.c_str(), server_path.c_str());

  bool ok = _robotComms.Connect(client_path.c_str(), server_path.c_str());
  if (!ok) {
    LOG_ERROR("AnimComms.InitRobotComms", "Unable to connect from %s to %s",
              client_path.c_str(), server_path.c_str());
    return RESULT_FAIL_IO;
  }

  return RESULT_OK;
}

Result InitEngineComms()
{
  const RobotID_t robotID = OSState::getInstance()->GetRobotID();
  const std::string & server_path = std::string(Victor::ENGINE_ANIM_SERVER_PATH) + std::to_string(robotID);

  LOG_INFO("AnimComms.InitEngineComms", "Start listening at %s", server_path.c_str());

  if (!_engineComms.StartListening(server_path)) {
    LOG_ERROR("AnimComms.InitEngineComms", "Unable to listen at %s", server_path.c_str());
    return RESULT_FAIL_IO;
  }

  return RESULT_OK;
}

Result InitComms()
{
  Result result = InitRobotComms();
  if (RESULT_OK != result) {
    LOG_ERROR("AnimComms.InitComms", "Unable to init robot comms (result %d)", result);
    return result;
  }

  result = InitEngineComms();
  if (RESULT_OK != result) {
    LOG_ERROR("AnimComms.InitComms", "Unable to init engine comms (result %d)", result);
    return result;
  }

  return RESULT_OK;
}

bool IsConnectedToRobot(void)
{
  return _robotComms.IsConnected();
}

bool IsConnectedToEngine(void)
{
  return _engineComms.HasClient();
}

void DisconnectRobot()
{
  LOG_DEBUG("AnimComms.DisconnectRobot", "Disconnect robot");
  _robotComms.Disconnect();
}

void DisconnectEngine(void)
{
  LOG_DEBUG("AnimComms.DisconnectEngine", "Disconnect engine");
  _engineComms.Disconnect();
}

bool SendPacketToEngine(const void *buffer, const u32 length)
{
  if (!_engineComms.HasClient()) {
    LOG_TRACE("AnimComms.SendPacketToEngine", "No engine client");
    return false;
  }

  const ssize_t bytesSent = _engineComms.Send((char*)buffer, length);
  if (bytesSent < (ssize_t) length) {
    LOG_ERROR("AnimComms.SendPacketToEngine.FailedSend",
              "Failed to send msg contents (%zd of %d bytes sent)",
              bytesSent, length);
    DisconnectEngine();
    return false;
  }

  return true;
}


u32 GetNextPacketFromEngine(u8* buffer, u32 max_length)
{
  // Read available datagram
  const ssize_t dataLen = _engineComms.Recv((char*)buffer, max_length);
  if (dataLen < 0) {
    // Something went wrong
    LOG_ERROR("GetNextPacketFromEngine.FailedRecv", "Failed to receive from engine");
    DisconnectEngine();
    return 0;
  }
  return (u32) dataLen;
}

  
bool SendPacketToRobot(const void *buffer, const u32 length)
{
  if (!_robotComms.IsConnected()) {
    LOG_TRACE("SendPacketToRobot", "Robot is not connected");
    return false;
  }

  const ssize_t bytesSent = _robotComms.Send((const char*)buffer, length);
  if (bytesSent < (ssize_t) length) {
    LOG_ERROR("SendPacketToRobot.FailedSend", "Failed to send msg contents (%zd bytes sent)", bytesSent);
    DisconnectRobot();
    return false;
  }

  return true;
}

u32 GetNextPacketFromRobot(u8* buffer, u32 max_length)
{
  // Read available datagram
  const ssize_t dataLen = _robotComms.Recv((char*)buffer, max_length);
  if (dataLen < 0) {
    // Something went wrong
    LOG_ERROR("GetNextPacketFromRobot.FailedRecv", "Failed to receive from robot");
    DisconnectRobot();
    return 0;
  }
    
  return (u32) dataLen;
}

} // namespace AnimComms
} // namespace Vector
} // namespace Anki
