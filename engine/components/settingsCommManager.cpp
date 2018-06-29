/**
* File: settingsCommManager.cpp
*
* Author: Paul Terry
* Created: 6/8/18
*
 * Description: Communicates settings with App and Cloud; calls into SettingsManager
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/components/settingsCommManager.h"

#include "engine/robot.h"
#include "engine/components/settingsManager.h"
#include "engine/cozmoAPI/comms/protoMessageHandler.h"
#include "engine/externalInterface/externalMessageRouter.h"

#include "util/console/consoleInterface.h"

#include "clad/types/robotSettingsTypes.h"

// Log options
#define LOG_CHANNEL "SettingsCommManager"

namespace Anki {
namespace Cozmo {


namespace
{
  SettingsCommManager* s_SettingsCommManager = nullptr;

#if REMOTE_CONSOLE_ENABLED

  static const char* kConsoleGroup = "RobotSettings";

  // NOTE: Need to keep kMasterVolumeLevels in sync with MasterVolume in robotSettings.clad
  constexpr const char* kMasterVolumeLevels = "Mute,Low,MediumLow,Medium,MediumHigh,High";
  CONSOLE_VAR_ENUM(u8, kMasterVolumeLevel, kConsoleGroup, 0, kMasterVolumeLevels);
  void DebugSetMasterVolume(ConsoleFunctionContextRef context)
  {
    const std::string volumeSettingValue = MasterVolumeToString(static_cast<MasterVolume>(kMasterVolumeLevel));
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.MasterVolume", volumeSettingValue);
  }
  CONSOLE_FUNC(DebugSetMasterVolume, kConsoleGroup);
  
  // NOTE: Need to keep kEyeColors in sync with EyeColor in robotSettings.clad
  constexpr const char* kEyeColors = "Teal,Orange,Yellow,LimeGreen,AzureBlue,Purple,MatrixGreen";
  CONSOLE_VAR_ENUM(u8, kEyeColor, kConsoleGroup, 0, kEyeColors);
  void DebugSetEyeColor(ConsoleFunctionContextRef context)
  {
    const std::string eyeColorValue = EyeColorToString(static_cast<EyeColor>(kEyeColor));
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.EyeColor", eyeColorValue);
  }
  CONSOLE_FUNC(DebugSetEyeColor, kConsoleGroup);

  void DebugSetLocale(ConsoleFunctionContextRef context)
  {
    const std::string localeValue = ConsoleArg_Get_String(context, "localeValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.Locale", localeValue);
  }
  CONSOLE_FUNC(DebugSetLocale, kConsoleGroup, const char* localeValue);

  void DebugSetTimeZone(ConsoleFunctionContextRef context)
  {
    const std::string timeZoneValue = ConsoleArg_Get_String(context, "timeZoneValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.TimeZone", timeZoneValue);
  }
  CONSOLE_FUNC(DebugSetTimeZone, kConsoleGroup, const char* timeZoneValue);

  void DebugSetDefaultLocation(ConsoleFunctionContextRef context)
  {
    const std::string defaultLocationValue = ConsoleArg_Get_String(context, "defaultLocationValue");
    s_SettingsCommManager->HandleRobotSettingChangeRequest("Robot.DefaultLocaction", defaultLocationValue);
  }
  CONSOLE_FUNC(DebugSetDefaultLocation, kConsoleGroup, const char* defaultLocationValue);

  void DebugToggle24HourClock(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->HandleRobotSettingToggleRequest("Robot.24HourClock");
  }
  CONSOLE_FUNC(DebugToggle24HourClock, kConsoleGroup);

  void DebugToggleTempIsFahrenheit(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->HandleRobotSettingToggleRequest("Robot.TempIsFahrenheit");
  }
  CONSOLE_FUNC(DebugToggleTempIsFahrenheit, kConsoleGroup);

  void DebugToggleDistIsMetric(ConsoleFunctionContextRef context)
  {
    s_SettingsCommManager->HandleRobotSettingToggleRequest("Robot.DistIsMetric");
  }
  CONSOLE_FUNC(DebugToggleDistIsMetric, kConsoleGroup);

#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
SettingsCommManager::SettingsCommManager()
: IDependencyManagedComponent(this, RobotComponentID::SettingsCommManager)
{
}


// TODO:  Does this need to be registered somehow such that a bunch of other things are initialized before it?
// (e.g. to ensure audio system is up, before we set volume settings in this InitDependent call.)
// Another strategy is to delay setting the settings until the first UpdateDependent call...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  s_SettingsCommManager = this;
  _settingsManager = &robot->GetComponent<SettingsManager>();
  _gatewayInterface = robot->GetGatewayInterface();
  auto* gi = _gatewayInterface;
  if (gi != nullptr)
  {
    auto commonCallback = std::bind(&SettingsCommManager::HandleEvents, this, std::placeholders::_1);
    // Subscribe to desired simple events
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapper::OneofMessageTypeCase::kPullSettingsRequest,   commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapper::OneofMessageTypeCase::kPushSettingsRequest,   commonCallback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapper::OneofMessageTypeCase::kUpdateSettingsRequest, commonCallback));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::UpdateDependent(const RobotCompMap& dependentComps)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsCommManager::HandleRobotSettingChangeRequest(const std::string& settingKey, const std::string& settingValue)
{
  // Change the robot setting and apply the change
  const bool success = _settingsManager->SetRobotSetting(settingKey, settingValue);
  if (!success)
  {
    LOG_ERROR("SettingsCommManager.HandleRobotSettingChangeRequest",
              "Error setting key %s to value %s", settingKey.c_str(), settingValue.c_str());
  }

  // TODO Update the jdoc (maybe only if successful?)
  // TODO Potentially send message to Cloud (only if successful?)
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// this is a helper function mostly for testing
bool SettingsCommManager::HandleRobotSettingToggleRequest(const std::string& settingKey)
{
  bool curSetting = _settingsManager->GetRobotSettingAsBool(settingKey);
  std::string newValue = curSetting ? "false" : "true";
  return HandleRobotSettingChangeRequest(settingKey, newValue);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::RefreshConsoleVars()
{
#if REMOTE_CONSOLE_ENABLED
  const auto& masterVolumeValue = _settingsManager->GetRobotSettingAsString("Robot.MasterVolume");
  MasterVolume masterVolume;
  if (EnumFromString(masterVolumeValue, masterVolume))
  {
    kMasterVolumeLevel = static_cast<u8>(masterVolume);
  }
  const auto& eyeColorValue = _settingsManager->GetRobotSettingAsString("Robot.EyeColor");
  EyeColor eyeColor;
  if (EnumFromString(eyeColorValue, eyeColor))
  {
    kEyeColor = static_cast<u8>(eyeColor);
  }
#endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  switch(event.GetData().oneof_message_type_case())
  {
    case external_interface::GatewayWrapper::OneofMessageTypeCase::kPullSettingsRequest:
      OnRequestPullSettings(event.GetData().pull_settings_request());
      break;
    case external_interface::GatewayWrapper::OneofMessageTypeCase::kPushSettingsRequest:
      OnRequestPushSettings(event.GetData().push_settings_request());
      break;
    case external_interface::GatewayWrapper::OneofMessageTypeCase::kUpdateSettingsRequest:
      OnRequestUpdateSettings(event.GetData().update_settings_request());
      break;
    default:
      LOG_ERROR("SettingsCommManager.HandleEvents",
                "HandleEvents called for unknown message");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestPullSettings(const external_interface::PullSettingsRequest& pullSettingsRequest)
{
  // TODO
  LOG_INFO("SettingsCommManager.OnRequestPullSettings", "Pull settings request");
  auto* pullSettingsResp = new external_interface::PullSettingsResponse();
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pullSettingsResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestPushSettings(const external_interface::PushSettingsRequest& pushSettingsRequest)
{
  // TODO
  LOG_INFO("SettingsCommManager.OnRequestPushSettings", "Push settings request");
  
  auto* pushSettingsResp = new external_interface::PushSettingsResponse();
  
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(pushSettingsResp));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsCommManager::OnRequestUpdateSettings(const external_interface::UpdateSettingsRequest& updateSettingsRequest)
{
  LOG_INFO("SettingsCommManager.OnRequestUpdateSettings", "Update settings request");
  const auto& settings = updateSettingsRequest.settings();
  
  if (settings.oneof_clock_24_hour_case() == external_interface::RobotSettingsConfig::OneofClock24HourCase::kClock24Hour)
  {
    HandleRobotSettingChangeRequest("Robot.24HourClock", settings.clock_24_hour() ? "true" : "false");
  }
  if (settings.oneof_eye_color_case() == external_interface::RobotSettingsConfig::OneofEyeColorCase::kEyeColor)
  {
    // Cast the protobuf3 enum to our CLAD enum, then call into CLAD to get the string version
    // todo:  replace the clad enum completely
    const std::string eyeColor = EnumToString(static_cast<EyeColor>(settings.eye_color()));
    HandleRobotSettingChangeRequest("Robot.EyeColor", eyeColor);
  }
  if (settings.oneof_default_location_case() == external_interface::RobotSettingsConfig::OneofDefaultLocationCase::kDefaultLocation)
  {
    HandleRobotSettingChangeRequest("Robot.DefaultLocation", settings.default_location());
  }
  if (settings.oneof_dist_is_metric_case() == external_interface::RobotSettingsConfig::OneofDistIsMetricCase::kDistIsMetric)
  {
    HandleRobotSettingChangeRequest("Robot.DistIsMetric", settings.dist_is_metric() ? "true" : "false");
  }
  if (settings.oneof_locale_case() ==  external_interface::RobotSettingsConfig::OneofLocaleCase::kLocale)
  {
    HandleRobotSettingChangeRequest("Robot.Locale", settings.locale());
  }
  if (settings.oneof_master_volume_case() == external_interface::RobotSettingsConfig::OneofMasterVolumeCase::kMasterVolume)
  {
    // Cast the protobuf3 enum to our CLAD enum, then call into CLAD to get the string version
    // todo:  replace the clad enum completely
    const std::string masterVolume = EnumToString(static_cast<MasterVolume>(settings.master_volume()));
    HandleRobotSettingChangeRequest("Robot.MasterVolume", masterVolume);;
  }
  if (settings.oneof_temp_is_fahrenheit_case() == external_interface::RobotSettingsConfig::OneofTempIsFahrenheitCase::kTempIsFahrenheit)
  {
    HandleRobotSettingChangeRequest("Robot.TempIsFahrenheit", settings.temp_is_fahrenheit() ? "true" : "false");
  }
  if (settings.oneof_time_zone_case() == external_interface::RobotSettingsConfig::OneofTimeZoneCase::kTimeZone)
  {
    HandleRobotSettingChangeRequest("Robot.TimeZone", settings.time_zone());
  }

  auto* updateSettingsResp = new external_interface::UpdateSettingsResponse();
  // TODO put the jdoc in the response?
  _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(updateSettingsResp));
}


} // namespace Cozmo
} // namespace Anki
