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
  // TODO Send message to App to acknowledge the change (and whether successful?)
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


// TODO:  Message handlers from App
// TODO:  Message handlers from Cloud

} // namespace Cozmo
} // namespace Anki
