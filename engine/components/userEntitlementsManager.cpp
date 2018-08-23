/**
 * File: userEntitlementsManager.cpp
 *
 * Author: Paul Terry
 * Created: 8/22/2018
 *
 * Description: Stores user entitlements on robot; accepts new user entitlements from app
 * or cloud; provides access to these for other components
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/components/userEntitlementsManager.h"

#include "engine/components/jdocsManager.h"
#include "engine/components/settingsCommManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"

//#include "clad/robotInterface/messageEngineToRobot.h"

//#include <sys/wait.h>

#define LOG_CHANNEL "UserEntitlementsManager"

namespace Anki {
namespace Vector {

namespace {
//  static const char* kConfigDefaultValueKey = "defaultValue";
//  static const char* kConfigUpdateCloudOnChangeKey = "updateCloudOnChange";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UserEntitlementsManager::UserEntitlementsManager()
: IDependencyManagedComponent(this, RobotComponentID::UserEntitlementsManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserEntitlementsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  _jdocsManager = &robot->GetComponent<JdocsManager>();

//  _accountSettingsConfig = &robot->GetContext()->GetDataLoader()->GetSettingsConfig();

  // Call the JdocsManager to see if our robot settings jdoc file exists
//  bool settingsDirty = false;
//  const bool jdocNeedsCreation = _jdocsManager->JdocNeedsCreation(external_interface::JdocType::USER_ENTITLEMENTS);
//  _currentUserEntitlements.clear();
//  if (jdocNeedsCreation)
//  {
//    LOG_INFO("UserEntitlementsManager.InitDependent.NoSettingsJdocFile",
//             "User entitlements jdoc file not found; one will be created shortly");
//  }
//  else
//  {
//    _currentUserEntitlements = _jdocsManager->GetJdocBody(external_interface::JdocType::USER_ENTITLEMENTS);

//    if (_jdocsManager->JdocNeedsMigration(external_interface::JdocType::USER_ENTITLEMENTS))
//    {
//      // TODO (this has its own ticket, VIC-5669):
//      //   Handle format migration (from loaded jdoc file) here.  We need to know the old
//      //   and new format versions.  Also put it in a function, and call that ALSO in the
//      //   case of migration triggered when pulling a new version of jdoc from the cloud.
//      //   consider another callback, similar to the 'overwritten' callback, but for this
//      //   format migration.
//      // Not doing this now because we're not changing format versions yet.
//    }
//  }

//  // Ensure current settings has each of the defined settings;
//  // if not, initialize each missing setting to default value
//  for (Json::ValueConstIterator it = _settingsConfig->begin(); it != _settingsConfig->end(); ++it)
//  {
//    if (!_currentSettings.isMember(it.name()))
//    {
//      const Json::Value& item = (*it);
//      _currentSettings[it.name()] = item[kConfigDefaultValueKey];
//      settingsDirty = true;
//      LOG_INFO("SettingsManager.InitDependent.AddDefaultItem", "Adding setting with key %s and default value %s",
//               it.name().c_str(), item[kConfigDefaultValueKey].asString().c_str());
//    }
//  }
//
//  // Now look through current settings, and remove any item
//  // that is no longer defined in the config
//  std::vector<std::string> keysToRemove;
//  for (Json::ValueConstIterator it = _currentSettings.begin(); it != _currentSettings.end(); ++it)
//  {
//    if (!_settingsConfig->isMember(it.name()))
//    {
//      keysToRemove.push_back(it.name());
//    }
//  }
//  for (const auto& key : keysToRemove)
//  {
//    LOG_INFO("SettingsManager.InitDependent.RemoveItem",
//             "Removing setting with key %s", key.c_str());
//    _currentSettings.removeMember(key);
//    settingsDirty = true;
//  }
//
//  if (settingsDirty)
//  {
//    static const bool kSaveToCloudImmediately = false;
//    static const bool kSetCloudDirtyIfNotImmediate = !jdocNeedsCreation;
//    UpdateSettingsJdoc(kSaveToCloudImmediately, kSetCloudDirtyIfNotImmediate);
//  }

//  // Register the actual setting application methods, for those settings that want to execute code when changed:
//  _settingSetters[RobotSetting::master_volume] = { false, &SettingsManager::ValidateSettingMasterVolume, &SettingsManager::ApplySettingMasterVolume };
//  _settingSetters[RobotSetting::eye_color]     = { true,  &SettingsManager::ValidateSettingEyeColor,     &SettingsManager::ApplySettingEyeColor     };
//  _settingSetters[RobotSetting::locale]        = { false, nullptr,                                       &SettingsManager::ApplySettingLocale       };
//  _settingSetters[RobotSetting::time_zone]     = { false, nullptr,                                       &SettingsManager::ApplySettingTimeZone     };
//
//  _jdocsManager->RegisterOverwriteNotificationCallback(external_interface::JdocType::ROBOT_SETTINGS, [this]() {
//    _currentSettings = _jdocsManager->GetJdocBody(external_interface::JdocType::ROBOT_SETTINGS);
//    ApplyAllCurrentSettings();
//  });
//
//  // Finally, set a flag so we will apply all of the settings
//  // we just loaded and/or set, in the first update
//  _applySettingsNextTick = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserEntitlementsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
//  if (_applySettingsNextTick)
//  {
//    _applySettingsNextTick = false;
//    ApplyAllCurrentSettings();
//
//    auto& settingsCommManager = _robot->GetComponent<SettingsCommManager>();
//    settingsCommManager.RefreshConsoleVars();
//  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//bool UserEntitlementsManager::SetAccountSetting(/*const RobotSetting robotSetting,*/
//                                                const Json::Value& valueJson,
//                                                const bool updateSettingsJdoc)
//{
//  const std::string key = RobotSettingToString(robotSetting);
//  if (!_currentSettings.isMember(key))
//  {
//    LOG_ERROR("SettingsManager.SetRobotSetting.InvalidKey", "Invalid key %s; ignoring", key.c_str());
//    return false;
//  }
//
//  const Json::Value prevValue = _currentSettings[key];
//  _currentSettings[key] = valueJson;
//
//  bool success = ApplyRobotSetting(robotSetting, false);
//  if (!success)
//  {
//    _currentSettings[key] = prevValue;  // Restore previous good value
//    return false;
//  }
//
//  if (updateSettingsJdoc)
//  {
//    const bool saveToCloudImmediately = DoesSettingUpdateCloudImmediately(robotSetting);
//    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
//    success = UpdateSettingsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);
//  }
//
//  return success;
//}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//std::string UserEntitlementsManager::GetRobotSettingAsString(const RobotSetting key) const
//{
//  const std::string& keyString = EnumToString(key);
//  if (!_currentSettings.isMember(keyString))
//  {
//    LOG_ERROR("UserEntitlementsManager.GetRobotSettingAsString.InvalidKey", "Invalid key %s", keyString.c_str());
//    return "Invalid";
//  }
//
//  return _currentSettings[keyString].asString();
//}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//bool UserEntitlementsManager::GetRobotSettingAsBool(const RobotSetting key) const
//{
//  const std::string& keyString = EnumToString(key);
//  if (!_currentSettings.isMember(keyString))
//  {
//    LOG_ERROR("UserEntitlementsManager.GetRobotSettingAsBool.InvalidKey", "Invalid key %s", keyString.c_str());
//    return false;
//  }
//
//  return _currentSettings[keyString].asBool();
//}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//uint32_t UserEntitlementsManager::GetRobotSettingAsUInt(const RobotSetting key) const
//{
//  const std::string& keyString = EnumToString(key);
//  if (!_currentSettings.isMember(keyString))
//  {
//    LOG_ERROR("UserEntitlementsManager.GetRobotSettingAsUInt.InvalidKey", "Invalid key %s", keyString.c_str());
//    return false;
//  }
//
//  return _currentSettings[keyString].asUInt();
//}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//bool UserEntitlementsManager::DoesSettingUpdateCloudImmediately(const RobotSetting key) const
//{
//  const std::string& keyString = EnumToString(key);
//  const auto& config = (*_settingsConfig)[keyString];
//  const bool saveToCloudImmediately = config[kConfigUpdateCloudOnChangeKey].asBool();
//  return saveToCloudImmediately;
//}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::UpdateUserEntitlementsJdoc(const bool saveToCloudImmediately,
                                                         const bool setCloudDirtyIfNotImmediate)
{
//  static const bool saveToDiskImmediately = true;
//  const bool success = _jdocsManager->UpdateJdoc(external_interface::JdocType::ACCOUNT_SETTINGS,
//                                                 &_currentAccountSettings,
//                                                 saveToDiskImmediately,
//                                                 saveToCloudImmediately,
//                                                 setCloudDirtyIfNotImmediate);
//  return success;
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//void UserEntitlementsManager::ApplyAllCurrentSettings()
//{
//  LOG_INFO("SettingsManager.ApplyAllCurrentSettings", "Applying all current robot settings");
//  for (Json::ValueConstIterator it = _currentSettings.begin(); it != _currentSettings.end(); ++it)
//  {
//    ApplyRobotSetting(RobotSettingFromString(it.name()));
//  }
//}


} // namespace Vector
} // namespace Anki
