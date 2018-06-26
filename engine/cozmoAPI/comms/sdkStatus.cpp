/**
 * File: sdkStatus
 *
 * Author: Mark Wesley
 * Created: 08/30/16
 *
 * Description: Status of the SDK connection and usage
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "engine/externalInterface/externalInterface.h"
#include "engine/cozmoAPI/comms/sdkStatus.h"
#include "engine/cozmoAPI/comms/iSocketComms.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include "util/time/universalTime.h"


namespace Anki {
namespace Cozmo {

SdkStatus::SdkStatus(IExternalInterface* externalInterface)
  : _recentCommands(10)
  , _externalInterface(externalInterface)
{
  assert(_externalInterface);
}


double SdkStatus::GetCurrentTime_s()
{
  return Util::Time::UniversalTime::GetCurrentTimeInSeconds();
}


inline double GetTimeBetween_s(double startTime_s, double endTime_s)
{
  if (startTime_s < 0.0)
  {
    return SdkStatus::kInvalidTime_s;
  }

  const double timeSince_s = endTime_s - startTime_s;
  DEV_ASSERT_MSG((timeSince_s >= 0.0), "GetTimeBetween_s.NegTime", "timeSince_s = %f (%f - %f)",
                 timeSince_s, endTime_s, startTime_s);

  return timeSince_s;
}


void SdkStatus::ResetRobot(bool isExitingSDKMode)
{
  using GToE = ExternalInterface::MessageGameToEngine;

  if (isExitingSDKMode) {
    // Return to freeplay
    //_externalInterface->Broadcast( GToE(ExternalInterface::ActivateHighLevelActivity(HighLevelActivity::Freeplay)) );
  }
  else {
    // Clear Behaviors
    /**_externalInterface->Broadcast( GToE(ExternalInterface::ActivateHighLevelActivity(HighLevelActivity::Selection)) );
    _externalInterface->Broadcast( GToE(ExternalInterface::ExecuteBehaviorByExecutableType(
               ExecutableBehaviorTypeToString(ExecutableBehaviorType::Wait), -1)) );

    ReactionTriggerToBehavior noneTrigger;
    noneTrigger.trigger = ReactionTrigger::NoneTrigger;
    _externalInterface->Broadcast( GToE(ExternalInterface::ExecuteReactionTrigger(noneTrigger)) );**/
  }

  // Do not put cubes to sleep for internal SDK
  if (_isInExternalSdkMode) {
    // Turn off all Cube Lights
    _externalInterface->Broadcast( GToE(ExternalInterface::EnableCubeSleep(true, true)) );
    _externalInterface->Broadcast( GToE(ExternalInterface::EnableLightStates(false, -1)) );
  }

  // Ensure auto-exposure is (re) enabled
  _externalInterface->Broadcast( GToE(ExternalInterface::SetCameraSettings(true, 0, 0.0f)) );

  // Disable color images from camera
  _externalInterface->Broadcast( GToE(ExternalInterface::EnableColorImages(false)) );

  // Undefine (and delete) all custom marker objects
  _externalInterface->Broadcast( GToE(ExternalInterface::UndefineAllCustomMarkerObjects()) );

  // Delete all fixed custom objects from the world
  _externalInterface->Broadcast( GToE(ExternalInterface::DeleteAllCustomObjects()) );

  // Stop everything else
  _externalInterface->Broadcast( GToE(ExternalInterface::StopRobotForSdk()) );

  // Re-enable Lift Power (in case user disabled it in their program)
  _externalInterface->Broadcast( GToE(ExternalInterface::EnableLiftPower(true)) );
}


void SdkStatus::EnterMode(bool isExternalSdkMode)
{
  const bool wasAlreadyInSdkMode = IsInAnySdkMode();
  if (isExternalSdkMode)
  {
    DEV_ASSERT(!_isInExternalSdkMode, "SdkStatus.EnterMode.AlreadyInExternalMode");
    Util::sInfoF("robot.sdk_mode_on", {}, "");
    _isInExternalSdkMode = true;
  }
  else
  {
    DEV_ASSERT(!_isInInternalSdkMode, "SdkStatus.EnterMode.AlreadyInInternalMode");
    _isInInternalSdkMode = true;
    _isConnected = true;
  }

  if (!wasAlreadyInSdkMode)
  {
    ResetRobot(false);
    _enterSdkModeTime_s = GetCurrentTime_s();
  }
}


void SdkStatus::ExitMode(bool isExternalSdkMode)
{
  if (isExternalSdkMode)
  {
    DEV_ASSERT(_isInExternalSdkMode, "SdkStatus.ExitMode.NotInExternalMode");
    _isInExternalSdkMode = false;
  }
  else
  {
    DEV_ASSERT(_isInInternalSdkMode, "SdkStatus.ExitMode.NotInInternalMode");
    _isInInternalSdkMode = false;
  }

  const bool isStillInSdkMode = IsInAnySdkMode();

  if (!isStillInSdkMode)
  {
    // Disconnect before sending exit mode event so that all connect/disconnects are wrapped by sdk on/off
    OnDisconnect(true);

    if (isExternalSdkMode)
    {
      const double timeInSdkMode = TimeInMode_s(GetCurrentTime_s());
      Util::sInfoF("robot.sdk_mode_off", {{DDATA, std::to_string(timeInSdkMode).c_str()}}, "%d", _numTimesConnected);
    }
  }
}


void SdkStatus::OnConnectionSuccess(const ExternalInterface::UiDeviceConnectionSuccess& message)
{
  if (!_isConnected)
  {
    if (_isInExternalSdkMode)
    {
      Util::sInfoF("robot.sdk_connection_started", {{DDATA, message.sdkModuleVersion.c_str()}}, "%s", message.buildVersion.c_str());
      Util::sInfoF("robot.sdk_python_version", {{DDATA, message.pythonVersion.c_str()}}, "%s", message.pythonImplementation.c_str());
      Util::sInfoF("robot.sdk_system_version", {{DDATA, message.osVersion.c_str()}}, "%s", message.cpuVersion.c_str());
    }

    _isConnected = true;
    _numCommandsSentOverConnection = 0;
    ++_numTimesConnected;
    _connectionStartTime_s = GetCurrentTime_s();
    _isWrongSdkVersion = false;
    _connectedSdkBuildVersion = message.buildVersion;
    _stopRobotOnDisconnect = true; // Always stop unless explicitly requested by this program run

    if(_shouldAutoConnectToCubes)
    {
      // Request a cube connection
      _externalInterface->Broadcast(ExternalInterface::MessageGameToEngine(ExternalInterface::ConnectToCube()));
    }
  }
  else
  {
    // Multiple connection calls are expected in Dev builds if connected via internal mode (Code Lab) and then connect
    // by external mode for dev/debugging without enabling sdk mode.
    const bool isExternalSdkMode = message.connectionType != UiConnectionType::UI;
    const bool isInExternalSdkModeOrConnection = _isInExternalSdkMode || isExternalSdkMode;
    if (!(isInExternalSdkModeOrConnection && _isInInternalSdkMode))
    {
      PRINT_NAMED_WARNING("SdkStatus.OnConnectionSuccess.AlreadyConnected", "");
    }
  }
}


void SdkStatus::OnWrongVersion(const ExternalInterface::UiDeviceConnectionWrongVersion& message)
{
  if (_isInExternalSdkMode)
  {
    Util::sInfoF("robot.sdk_wrong_version", {{DDATA, message.buildVersion.c_str()}}, "");
  }
  OnDisconnect(false);
  _isWrongSdkVersion = true;
  _connectedSdkBuildVersion = message.buildVersion;
}


void SdkStatus::OnDisconnect(bool isExitingSDKMode)
{
  if (_isConnected)
  {
    if (_isInExternalSdkMode)
    {
      Util::sInfoF("robot.sdk_connection_ended", {{DDATA, std::to_string(TimeInCurrentConnection_s(GetCurrentTime_s(), true)).c_str()}},
                   "%u", _numCommandsSentOverConnection);
    }

    if (_stopRobotOnDisconnect)
    {
      ResetRobot(isExitingSDKMode);
    }

    if(_shouldAutoDisconnectFromCubes)
    {
      // Disconnect from cube immediately
      const float gracePeriod_sec = 0.f;
      _externalInterface->Broadcast(ExternalInterface::MessageGameToEngine(ExternalInterface::DisconnectFromCube(gracePeriod_sec)));
    }

    _isConnected = false;
  }
}


void SdkStatus::SetStopRobotOnDisconnect(bool newVal)
{
  if (_isConnected)
  {
    _stopRobotOnDisconnect = newVal;
  }
  else
  {
    PRINT_NAMED_ERROR("SdkStatus.OnRequestNoRobotResetOnSdkDisconnect.NotConnected", "");
  }
}


void SdkStatus::OnRecvMessage(const ExternalInterface::MessageGameToEngine& message, size_t messageSize)
{
  _recentCommands.push_back( message.GetTag() );

  _lastSdkMessageTime_s = GetCurrentTime_s();
  if (message.GetTag() != ExternalInterface::MessageGameToEngineTag::Ping)
  {
    _lastSdkCommandTime_s = _lastSdkMessageTime_s;
    ++_numCommandsSentOverConnection;
  }
}


const char* SdkStatus::GetRecentCommandName(size_t index) const
{
  return MessageGameToEngineTagToString( _recentCommands[index] );
}


void SdkStatus::UpdateConnectionStatus(const ISocketComms* sdkSocketComms)
{
  if (_isConnected && !_isInInternalSdkMode)
  {
    if (sdkSocketComms->GetNumConnectedDevices() == 0)
    {
      OnDisconnect(false);
    }
  }
}


double SdkStatus::TimeInMode_s(double timeNow_s) const
{
  if (IsInAnySdkMode())
  {
    return GetTimeBetween_s(_enterSdkModeTime_s, timeNow_s);
  }

  return kInvalidTime_s;
}


double SdkStatus::TimeInCurrentConnection_s(double timeNow_s, bool activeTime) const
{
  if (_isConnected)
  {
    if (activeTime)
    {
      if (_lastSdkMessageTime_s < _connectionStartTime_s)
      {
        // no message received on the connection yet
        return 0.0;
      }
      else
      {
        return GetTimeBetween_s(_connectionStartTime_s, _lastSdkMessageTime_s);
      }
    }
    else
    {
      return GetTimeBetween_s(_connectionStartTime_s, timeNow_s);
    }
  }

  return kInvalidTime_s;
}


double SdkStatus::TimeSinceLastSdkMessage_s(double timeNow_s) const
{
  return GetTimeBetween_s(_lastSdkMessageTime_s, timeNow_s);
}


double SdkStatus::TimeSinceLastSdkCommand_s(double timeNow_s) const
{
  return GetTimeBetween_s(_lastSdkCommandTime_s, timeNow_s);
}



} // namespace Cozmo
} // namespace Anki
