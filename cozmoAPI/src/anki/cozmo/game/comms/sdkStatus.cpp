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


#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/game/comms/sdkStatus.h"
#include "anki/cozmo/game/comms/iSocketComms.h"
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
  ASSERT_NAMED_EVENT((timeSince_s >= 0.0), "GetTimeBetween_s.NegTime", "timeSince_s = %f (%f - %f)", timeSince_s, endTime_s, startTime_s);
  
  return timeSince_s;
}


void SdkStatus::StopRobotDoingAnything()
{
  using GToE = ExternalInterface::MessageGameToEngine;
  
  // Disable reactionary behaviors
  _externalInterface->Broadcast( GToE(ExternalInterface::EnableReactionaryBehaviors(false)) );
  
  // Clear Behaviors
  _externalInterface->Broadcast( GToE(ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)) );
  _externalInterface->Broadcast( GToE(ExternalInterface::ExecuteBehavior(BehaviorType::NoneBehavior)) );
  
  // Turn off all Cube Lights
  _externalInterface->Broadcast( GToE(ExternalInterface::EnableLightStates(false, -1)) );
  
  // Stop everything else
  _externalInterface->Broadcast( GToE(ExternalInterface::StopRobotForSdk()) );
}


void SdkStatus::EnterMode()
{
  ASSERT_NAMED_EVENT(!_isInSdkMode, "SdkStatus.EnterMode.AlreadyInMode", "");
  Util::sEventF("robot.sdk_mode_on", {}, "");
  
  StopRobotDoingAnything();
  
  _isInSdkMode = true;
  _enterSdkModeTime_s = GetCurrentTime_s();
}


void SdkStatus::ExitMode()
{
  ASSERT_NAMED_EVENT(_isInSdkMode, "SdkStatus.ExitMode.NotInMode", "");
  const double timeInSdkMode = TimeInMode_s(GetCurrentTime_s());
  Util::sEventF("robot.sdk_mode_off", {{DDATA, std::to_string(timeInSdkMode).c_str()}}, "%d", _numTimesConnected);
  
  OnDisconnect();
  _isInSdkMode = false;
}


void SdkStatus::OnConnectionSuccess(const ExternalInterface::UiDeviceConnectionSuccess& message)
{
  if (!_isConnected)
  {
    Util::sEventF("robot.sdk_connection_started", {{DDATA, message.sdkModuleVersion.c_str()}}, "%s", message.buildVersion.c_str());
    Util::sEventF("robot.sdk_python_version", {{DDATA, message.pythonVersion.c_str()}}, "%s", message.pythonImplementation.c_str());
    Util::sEventF("robot.sdk_system_version", {{DDATA, message.osVersion.c_str()}}, "%s", message.cpuVersion.c_str());
    
    _isConnected = true;
    _numCommandsSentOverConnection = 0;
    ++_numTimesConnected;
    _connectionStartTime_s = GetCurrentTime_s();
    _isWrongSdkVersion = false;
    _connectedSdkBuildVersion = message.buildVersion;
    _stopRobotOnDisconnect = true; // Always stop unless explictely requested by this program run
  }
  else
  {
    PRINT_NAMED_ERROR("SdkStatus.OnConnectionSuccess.AlreadyConnected", "");
  }
}
  
  
void SdkStatus::OnWrongVersion(const ExternalInterface::UiDeviceConnectionWrongVersion& message)
{
  Util::sEventF("robot.sdk_wrong_version", {{DDATA, message.buildVersion.c_str()}}, "");
  OnDisconnect();
  _isWrongSdkVersion = true;
  _connectedSdkBuildVersion = message.buildVersion;
}

  
void SdkStatus::OnDisconnect()
{
  if (_isConnected)
  {
    Util::sEventF("robot.sdk_connection_ended", {{DDATA, std::to_string(TimeInCurrentConnection_s(GetCurrentTime_s(), true)).c_str()}},
                  "%u", _numCommandsSentOverConnection);
    
    if (_stopRobotOnDisconnect)
    {
      StopRobotDoingAnything();
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
  if (_isConnected)
  {
    if (sdkSocketComms->GetNumConnectedDevices() == 0)
    {
      OnDisconnect();
    }
  }
}
  
  
double SdkStatus::TimeInMode_s(double timeNow_s) const
{
  if (_isInSdkMode)
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

