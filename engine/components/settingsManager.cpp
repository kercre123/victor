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

#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "util/console/consoleInterface.h"

#include "clad/types/robotSettingsTypes.h"

// Log options
#define LOG_CHANNEL "SettingsManager"

namespace Anki {
namespace Cozmo {


namespace
{
  const std::string kSettingsManagerFolder = "settings";
  const std::string kSettingsFilename = "settings.json";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
SettingsManager::SettingsManager()
: IDependencyManagedComponent(this, RobotComponentID::SettingsManager)
{
}


// TODO:  Does this need to be registered somehow such that a bunch of other things are initialized before it?
// (e.g. to ensure audio system is up, before we set volume settings in this InitDependent call.)
// Another strategy is to delay setting the settings until the first UpdateDependent call...
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _audioClient = robot->GetAudioClient();

  // Read the default settings config
  const auto& config = robot->GetContext()->GetDataLoader()->GetSettingsConfig();
  _defaultSettings.clear();
  for (Json::ValueConstIterator it = config.begin(); it != config.end(); ++it)
  {
    const Json::Value& item = (*it);
    _defaultSettings[it.name()] = Setting(item.asString());
    LOG_INFO("SettingsManager.InitDependent", "JSON item: name is %s; value is %s",
             it.name().c_str(), item.asString().c_str());
  }

  _platform = robot->GetContextDataPlatform();
  DEV_ASSERT(_platform != nullptr, "SettingsManager.InitDependent.DataPlatformIsNull");

  _savePath = _platform->pathToResource(Util::Data::Scope::Persistent, kSettingsManagerFolder);
  if (!Util::FileUtils::CreateDirectory(_savePath))
  {
    LOG_ERROR("SettingsManager.InitDependent.FailedToCreateFolder", "Failed to create folder %s", _savePath.c_str());
    return;
  }

  // Read saved settings from robot if they exist; handle missing settings file
  bool settingsDirty = false;
  _currentSettings.clear();
  _fullPathSettingsFile = Util::FileUtils::FullFilePath({_savePath, kSettingsFilename});
  if (Util::FileUtils::FileExists(_fullPathSettingsFile))
  {
    if (!LoadSettingsFile())
    {
      LOG_ERROR("SettingsManager.InitDependent.FailedLoadingSettingsFile", "Error loading settings file");
      settingsDirty = true;
    }
  }
  else
  {
    LOG_WARNING("SettingsManager.InitDependent.NoSettingsFile", "Settings file not found; one will be created shortly");
    settingsDirty = true;
  }

  // Ensure current settings has each of the default settings;
  // if not, initialize each missing setting to default value
  for (const auto& item : _defaultSettings)
  {
    if (_currentSettings.find(item.first) == _currentSettings.end())
    {
      _currentSettings[item.first] = Setting(item.second);
      settingsDirty = true;
      LOG_INFO("SettingsManager.InitDependent.AddDefaultItem", "Adding setting with key %s and default value %s",
               item.first.c_str(), item.second._settingValue.c_str());
    }
  }

  // Now look through current settings, and remove any item
  // that is no longer defined in the config
  std::vector<std::string> keysToRemove;
  for (const auto& item : _currentSettings)
  {
    if (_defaultSettings.find(item.first) == _defaultSettings.end())
    {
      keysToRemove.push_back(item.first);
    }
  }
  for (const auto& key : keysToRemove)
  {
    LOG_INFO("SettingsManager.InitDependent.RemoveItem",
             "Removing setting with key %s", key.c_str());
    _currentSettings.erase(key);
    settingsDirty = true;
  }

  if (settingsDirty)
  {
    SaveSettingsFile();
  }

  // Register the actual setting application methods:
  _currentSettings["Robot.MasterVolume"]._settingSetter = &SettingsManager::ApplySettingMasterVolume;
  _currentSettings["Robot.EyeColor"]._settingSetter = &SettingsManager::ApplySettingEyeColor;

  // Finally, apply all of the settings we just loaded and/or set
  ApplyAllCurrentSettings();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::SetRobotSetting(const std::string& key, const std::string& value,
                                      const bool saveSettingsFile)
{
  if (_currentSettings.find(key) == _currentSettings.end())
  {
    LOG_ERROR("SettingsManager.SetRobotSetting.InvalidKey", "Invalid key %s; ignoring", key.c_str());
    return false;
  }

  // Actually apply the setting
  const auto& setterFunc = _currentSettings[key]._settingSetter;
  if (setterFunc != nullptr)
  {
    const bool success = (this->*setterFunc)(value);
    if (!success)
    {
      LOG_ERROR("SettingsManager.SetRobotSetting", "Error setting %s to %s", key.c_str(), value.c_str());
      return false;
    }
  }

  _currentSettings[key]._settingValue = value;

  if (saveSettingsFile)
  {
    SaveSettingsFile();
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::LoadSettingsFile()
{
  Json::Value data;
  if (!_platform->readAsJson(_fullPathSettingsFile, data))
  {
    LOG_ERROR("SettingsManager.LoadSettingsFile.Failed", "Failed to read %s", _fullPathSettingsFile.c_str());
    return false;
  }

  _currentSettings.clear();
  for (Json::ValueConstIterator it = data.begin(); it != data.end(); ++it)
  {
    const Json::Value& item = (*it);
    _currentSettings[it.name()] = Setting(item.asString());
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::SaveSettingsFile() const
{
  Json::Value data;
  for (const auto& setting : _currentSettings)
  {
    data[setting.first] = setting.second._settingValue;
  }

  if (!_platform->writeAsJson(_fullPathSettingsFile, data))
  {
    LOG_ERROR("SettingsManager.SaveSettingsFile.Failed", "Failed to write settings file");
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsManager::ApplyAllCurrentSettings()
{
  static const bool saveSettingsFile = false;
  for (const auto& setting : _currentSettings)
  {
    SetRobotSetting(setting.first, setting.second._settingValue, saveSettingsFile);
  }

  SaveSettingsFile();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingMasterVolume(const std::string& newValue)
{
  MasterVolume masterVolume;
  const bool valueIsValid = EnumFromString(newValue, masterVolume);
  if (!valueIsValid)
  {
    LOG_ERROR("SettingsManager.ApplySettingMasterVolume", "Invalid master volume value %s", newValue.c_str());
    return false;
  }

  // todo: actually set the volume
  LOG_INFO("ApplySettingMasterVolume.Apply", "Setting robot master volume to %s", newValue.c_str());

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsManager::ApplySettingEyeColor(const std::string& newValue)
{
  EyeColor eyeColor;
  const bool valueIsValid = EnumFromString(newValue, eyeColor);
  if (!valueIsValid)
  {
    LOG_ERROR("SettingsManager.ApplySettingEyeColor", "Invalid eye color value %s", newValue.c_str());
    return false;
  }

  // todo: actually set the eye color
  LOG_INFO("ApplySettingEyeColor.Apply", "Setting robot eye color to %s", newValue.c_str());
  return true;
}

} // namespace Cozmo
} // namespace Anki
