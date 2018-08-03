/**
* File: settingsManager.cpp
*
* Author: Paul Terry
* Created: 6/8/18
*
* Description: Stores robot settings on robot; accepts and sets new settings
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/components/settingsManager.h"

#include "engine/components/jdocsManager.h"
#include "engine/components/settingsCommManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "util/console/consoleInterface.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include <sys/wait.h>

#define LOG_CHANNEL "SettingsManager"

namespace Anki {
namespace Cozmo {



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsManager::SettingsManager()
: IDependencyManagedComponent(this, RobotComponentID::SettingsManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  _jdocsManager = &robot->GetComponent<JdocsManager>();
  _audioClient = robot->GetAudioClient();

  // Get the default settings config
  const auto& defaultSettings = robot->GetContext()->GetDataLoader()->GetSettingsConfig();

  _platform = robot->GetContextDataPlatform();
  DEV_ASSERT(_platform != nullptr, "SettingsManager.InitDependent.DataPlatformIsNull");

  // Call the JdocsManager to see if our robot settings jdoc file exists
  bool settingsDirty = false;
  _currentSettings.clear();
  if (_jdocsManager->JdocNeedsCreation(external_interface::JdocType::ROBOT_SETTINGS))
  {
    LOG_WARNING("SettingsManager.InitDependent.NoSettingsJdocsFile", "Settings jdocs file not found; one will be created shortly");
    settingsDirty = true;
  }
  else
  {
    _currentSettings = _jdocsManager->GetJdocBody(external_interface::JdocType::ROBOT_SETTINGS);
  }

  // Ensure current settings has each of the default settings;
  // if not, initialize each missing setting to default value
  for (Json::ValueConstIterator it = defaultSettings.begin(); it != defaultSettings.end(); ++it)
  {
    if (!_currentSettings.isMember(it.name()))
    {
      const Json::Value& item = (*it);
      _currentSettings[it.name()] = item;
      settingsDirty = true;
      LOG_INFO("SettingsManager.InitDependent.AddDefaultItem", "Adding setting with key %s and default value %s",
               it.name().c_str(), item.asString().c_str());
    }
  }

  // Now look through current settings, and remove any item
  // that is no longer defined in the config
  std::vector<std::string> keysToRemove;
  for (Json::ValueConstIterator it = _currentSettings.begin(); it != _currentSettings.end(); ++it)
  {
    if (!defaultSettings.isMember(it.name()))
    {
      keysToRemove.push_back(it.name());
    }
  }
  for (const auto& key : keysToRemove)
  {
    LOG_INFO("SettingsManager.InitDependent.RemoveItem",
             "Removing setting with key %s", key.c_str());
    _currentSettings.removeMember(key);
    settingsDirty = true;
  }

  if (settingsDirty)
  {
    UpdateSettingsJdoc();
  }

  // Register the actual setting application methods, for those settings that want to execute code when changed:
  _settingSetters[RobotSetting::master_volume] = &SettingsManager::ApplySettingMasterVolume;
  _settingSetters[RobotSetting::eye_color]     = &SettingsManager::ApplySettingEyeColor;
  _settingSetters[RobotSetting::locale]        = &SettingsManager::ApplySettingLocale;
  _settingSetters[RobotSetting::time_zone]     = &SettingsManager::ApplySettingTimeZone;

  // Finally, set a flag so we will apply all of the settings
  // we just loaded and/or set, in the first update
  _applySettingsNextTick = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if (_applySettingsNextTick)
  {
    _applySettingsNextTick = false;
    ApplyAllCurrentSettings();

    auto& settingsCommManager = _robot->GetComponent<SettingsCommManager>();
    settingsCommManager.RefreshConsoleVars();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::SetRobotSetting(const RobotSetting robotSetting,
                                      const Json::Value& valueJson,
                                      const bool updateSettingsJdoc)
{
  const std::string key = RobotSettingToString(robotSetting);
  if (!_currentSettings.isMember(key))
  {
    LOG_ERROR("SettingsManager.SetRobotSetting.InvalidKey", "Invalid key %s; ignoring", key.c_str());
    return false;
  }

  const Json::Value prevValue = _currentSettings[key];
  _currentSettings[key] = valueJson;

  bool success = ApplyRobotSetting(robotSetting);
  if (!success)
  {
    _currentSettings[key] = prevValue;  // Restore previous good value
    return false;
  }

  if (updateSettingsJdoc)
  {
    success = UpdateSettingsJdoc();
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string SettingsManager::GetRobotSettingAsString(const RobotSetting key) const
{
  const std::string& keyString = EnumToString(key);
  if (!_currentSettings.isMember(keyString))
  {
    LOG_ERROR("SettingsManager.GetRobotSetting.InvalidKey", "Invalid key %s", keyString.c_str());
    return "Invalid";
  }

  return _currentSettings[keyString].asString();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::GetRobotSettingAsBool(const RobotSetting key) const
{
  const std::string& keyString = EnumToString(key);
  if (!_currentSettings.isMember(keyString))
  {
    LOG_ERROR("SettingsManager.GetRobotSetting.InvalidKey", "Invalid key %s", keyString.c_str());
    return false;
  }

  return _currentSettings[keyString].asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::UpdateSettingsJdoc()
{
  static const bool saveToDiskImmediately = true;
  const bool success = _jdocsManager->UpdateJdoc(external_interface::JdocType::ROBOT_SETTINGS,
                                                 _currentSettings, saveToDiskImmediately);
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::ApplyAllCurrentSettings()
{
  for (Json::ValueConstIterator it = _currentSettings.begin(); it != _currentSettings.end(); ++it)
  {
    ApplyRobotSetting(RobotSettingFromString(it.name()));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplyRobotSetting(const RobotSetting robotSetting)
{
  // Actually apply the setting; note that some things don't need to be "applied"
  bool success = true;
  const auto it = _settingSetters.find(robotSetting);
  if (it != _settingSetters.end())
  {
    success = (this->*(it->second))();

    if (!success)
    {
      LOG_ERROR("SettingsManager.ApplyRobotSetting", "Error setting %s to %s",
                RobotSettingToString(robotSetting), _currentSettings[RobotSettingToString(robotSetting)].asString().c_str());
    }
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingMasterVolume()
{
  static const std::string& key = RobotSettingToString(RobotSetting::master_volume);
  const std::string& value = _currentSettings[key].asString();
  MasterVolume masterVolume;
  const bool valueIsValid = EnumFromString(value, masterVolume);
  if (!valueIsValid)
  {
    LOG_ERROR("SettingsManager.ApplySettingMasterVolume", "Invalid master volume value %s", value.c_str());
    return false;
  }

  LOG_INFO("ApplySettingMasterVolume.Apply", "Setting robot master volume to %s", value.c_str());
  _robot->GetAudioClient()->SetRobotMasterVolume(masterVolume);

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingEyeColor()
{
  static const std::string& key = RobotSettingToString(RobotSetting::eye_color);
  const std::string& value = _currentSettings[key].asString();
  EyeColor eyeColorUnused;
  const bool valueIsValid = EnumFromString(value, eyeColorUnused);
  if (!valueIsValid)
  {
    LOG_ERROR("SettingsManager.ApplySettingEyeColor", "Invalid eye color value %s", value.c_str());
    return false;
  }

  LOG_INFO("ApplySettingEyeColor.Apply", "Setting robot eye color to %s", value.c_str());
  const auto& config = _robot->GetContext()->GetDataLoader()->GetEyeColorConfig();
  const auto& eyeColorData = config[value];
  const float hue = eyeColorData["Hue"].asFloat();
  const float saturation = eyeColorData["Saturation"].asFloat();

  _robot->SendRobotMessage<RobotInterface::SetFaceHue>(hue);
  _robot->SendRobotMessage<RobotInterface::SetFaceSaturation>(saturation);

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingLocale()
{
  static const std::string& key = RobotSettingToString(RobotSetting::locale);
  const std::string& value = _currentSettings[key].asString();
  DEV_ASSERT(_robot != nullptr, "SettingsManager.ApplySettingLocale.InvalidRobot");
  return _robot->SetLocale(value);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingTimeZone()
{
#if defined(ANKI_PLATFORM_VICOS)
  static const std::string key = RobotSettingToString(RobotSetting::time_zone);
  const std::string value = _currentSettings[key].asString();

  std::vector<std::string> command;
  command.push_back("/usr/bin/timedatectl");
  command.push_back("set-timezone");
  command.push_back(value);
  return ExecCommand(command);
#else
  return true;
#endif
}


#if defined(ANKI_PLATFORM_VICOS)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ExecCommand(const std::vector<std::string>& args)
{
  LOG_INFO("SettingsManager.ExecCommand", "Called with cmd: %s (and %i arguments)",
           args[0].c_str(), (int)(args.size() - 1));

  const pid_t pID = fork();
  if (pID == 0) // child
  {
    char* argv_child[args.size() + 1];

    for (size_t i = 0; i < args.size(); i++)
    {
      argv_child[i] = (char *) malloc(args[i].size() + 1);
      strcpy(argv_child[i], args[i].c_str());
    }
    argv_child[args.size()] = nullptr;

    execv(argv_child[0], argv_child);

    // We'll only get here if execv fails
    for (size_t i = 0 ; i < args.size() + 1 ; ++i)
    {
      free(argv_child[i]);
    }
    exit(0);
  }
  else if (pID < 0) // fail
  {
    LOG_INFO("SettingsManager.ExecCommand.FailedFork", "Failed fork!");
    return false;
  }
  else  // parent
  {
    // Wait for child to complete so we can get an error code
    int status;
    pid_t w = TEMP_FAILURE_RETRY(waitpid(pID, &status, 0));
    if (w == -1)
    {
      return false;
    }
    if (!WIFEXITED(status))
    {
      return false;
    }
    LOG_INFO("SettingsManager.ExecCommand", "Status of forked child process is %i", status);
    return (WEXITSTATUS(status) == 0);
  }
  return true;
}
#endif


} // namespace Cozmo
} // namespace Anki
