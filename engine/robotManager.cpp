//
//  robotManager.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/perfMetric.h"
#include "engine/robot.h"
#include "engine/robotInitialConnection.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/robot/config.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "json/json.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/time/stepTimers.h"
#include <sys/stat.h>

#include "coretech/common/robot/config.h"
#include "util/global/globalDefinitions.h"

#define LOG_CHANNEL "RobotState"

namespace Anki {
namespace Cozmo {

RobotManager::RobotManager(const CozmoContext* context)
: _robot(nullptr)
, _context(context)
, _robotEventHandler(context)
, _robotMessageHandler(new RobotInterface::MessageHandler())
{
}

RobotManager::~RobotManager()
{
}

void RobotManager::Init(const Json::Value& config)
{
  auto startTime = std::chrono::steady_clock::now();

  Anki::Util::Time::PushTimedStep("RobotManager::Init");
  _robotMessageHandler->Init(config, this, _context);
  Anki::Util::Time::PopTimedStep(); // RobotManager::Init

  Anki::Util::Time::PrintTimedSteps();
  Anki::Util::Time::ClearSteps();

  auto endTime = std::chrono::steady_clock::now();
  auto timeSpent_millis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

  if (ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS)
  {
    constexpr auto maxInitTime_millis = 3000;
    if (timeSpent_millis > maxInitTime_millis)
    {
      LOG_WARNING("RobotManager.Init.TimeSpent",
                  "%lld milliseconds spent initializing, expected %d",
                  timeSpent_millis,
                  maxInitTime_millis);
    }
  }

  PRINT_NAMED_INFO("robot.init.time_spent_ms", "%lld", timeSpent_millis);
}

void RobotManager::Shutdown()
{
  // Order of destruction matters! Robot actions call back into robot manager, so
  // they must be released before robot manager itself.
  LOG_INFO("RobotManager.Shutdown", "Shutting down");
  _robot.reset();
}

void RobotManager::AddRobot(const RobotID_t withID)
{
  if (_robot == nullptr) {
    LOG_INFO("RobotManager.AddRobot.Adding", "Adding robot with ID=%d", withID);
    _robot.reset(new Robot(withID, _context));
    _initialConnection.reset(new RobotInitialConnection(_context));
  } else {
    LOG_WARNING("RobotManager.AddRobot.AlreadyAdded", "Robot already exists. Must remove first.");
  }
}

void RobotManager::RemoveRobot(bool robotRejectedConnection)
{
  if(_robot != nullptr) {
    LOG_INFO("RobotManager.RemoveRobot.Removing", "Removing robot with ID=%d", _robot->GetID());

    // ask initial connection tracker if it's handling this
    bool handledDisconnect = false;
    if (_initialConnection) {
      const auto result = robotRejectedConnection ? RobotConnectionResult::ConnectionRejected : RobotConnectionResult::ConnectionFailure;
      handledDisconnect = _initialConnection->HandleDisconnect(result);
    }
    if (!handledDisconnect) {
      _context->GetExternalInterface()->OnRobotDisconnected(_robot->GetID());
      _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDisconnected(0.0f)));
    }

    _context->GetPerfMetric()->OnRobotDisconnected();

    _robot.reset();
    _initialConnection.reset();

    // Clear out the global DAS values that contain the robot hardware IDs.
    Anki::Util::sSetGlobal(DPHYS, nullptr);
    Anki::Util::sSetGlobal(DGROUP, nullptr);
  } else {
    LOG_WARNING("RobotManager.RemoveRobot.NoRobotToRemove", "");
  }
}

// for when you don't care and you just want a damn robot
Robot* RobotManager::GetRobot()
{
  return _robot.get();
}

bool RobotManager::DoesRobotExist(const RobotID_t withID) const
{
  if (_robot) {
    return _robot->GetID() == withID;
  }
  return false;
}


Result RobotManager::UpdateRobot()
{
  ANKI_CPU_PROFILE("RobotManager::UpdateRobot");

  if (_robot) {
    // Call update
    Result result = _robot->Update();

    switch (result)
    {
      case RESULT_FAIL_IO_TIMEOUT:
      {
        LOG_WARNING("RobotManager.UpdateRobot.FailIOTimeout", "Signaling robot disconnect");
        RemoveRobot(false);
        break;
      }

      // TODO: Handle other return results here

      default:
        break;
    }

    if (_robot->HasReceivedRobotState()) {
      _context->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(_robot->GetRobotState()));
    }
    else {
      LOG_PERIODIC_INFO(10, "RobotManager.UpdateRobot",
                        "Not sending robot %d state (none available).", _robot->GetID());
    }

    // If the robot got a message to shutdown
    if(_robot->ToldToShutdown())
    {
      LOG_INFO("RobotManager.UpdateRobot.Shutdown","");
      Shutdown();
      return RESULT_SHUTDOWN;
    }
  }

  return RESULT_OK;
}

Result RobotManager::UpdateRobotConnection()
{
  ANKI_CPU_PROFILE("RobotManager::UpdateRobotConnection");
  return _robotMessageHandler->ProcessMessages();
}

bool RobotManager::ShouldFilterMessage(const RobotInterface::RobotToEngineTag msgType) const
{
  if (_initialConnection) {
    return _initialConnection->ShouldFilterMessage(msgType);
  }
  return false;
}

bool RobotManager::ShouldFilterMessage(const RobotInterface::EngineToRobotTag msgType) const
{
  if (_initialConnection) {
    return _initialConnection->ShouldFilterMessage(msgType);
  }
  return false;
}

} // namespace Cozmo
} // namespace Anki
