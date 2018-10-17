/**
 * File: accountSettingsManager.cpp
 *
 * Author: Paul Terry
 * Created: 8/22/2018
 *
 * Description: Stores account settings on robot; accepts new account settings from app
 * or cloud; provides access to these settings for other components
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/components/accountSettingsManager.h"

#include "engine/components/jdocsManager.h"
#include "engine/components/settingsCommManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/console/consoleInterface.h"
#include "util/environment/locale.h"
#include "util/logging/DAS.h"

#define LOG_CHANNEL "AccountSettingsManager"

namespace Anki {
namespace Vector {

namespace
{
  static const char* kConfigDefaultValueKey = "defaultValue";
  static const char* kConfigUpdateCloudOnChangeKey = "updateCloudOnChange";

}

//
// Static methods to publish magic DAS events
//
static void EnableDataCollection(bool enable)
{
  DASMSG(das_allow_upload, DASMSG_DAS_ALLOW_UPLOAD, "Allow DAS upload");
  DASMSG_SET(i1, (enable ? 1 : 0), "0/1")
  DASMSG_SEND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AccountSettingsManager::AccountSettingsManager()
: IDependencyManagedComponent(this, RobotComponentID::AccountSettingsManager)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AccountSettingsManager::InitDependent(Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  _jdocsManager = &_robot->GetComponent<JdocsManager>();

  _accountSettingsConfig = &_robot->GetContext()->GetDataLoader()->GetAccountSettingsConfig();

  // Call the JdocsManager to see if our account settings jdoc file exists
  bool settingsDirty = false;
  const bool jdocNeedsCreation = _jdocsManager->JdocNeedsCreation(external_interface::JdocType::ACCOUNT_SETTINGS);
  _currentAccountSettings.clear();
  if (jdocNeedsCreation)
  {
    LOG_INFO("AccountSettingsManager.InitDependent.NoAccountSettingsJdocFile",
             "Account settings jdoc file not found; one will be created shortly");
  }
  else
  {
    _currentAccountSettings = _jdocsManager->GetJdocBody(external_interface::JdocType::ACCOUNT_SETTINGS);

    if (_jdocsManager->JdocNeedsMigration(external_interface::JdocType::ACCOUNT_SETTINGS))
    {
      DoJdocFormatMigration();
      settingsDirty = true;
    }
  }

  // Ensure current settings has each of the defined settings;
  // if not, initialize each missing setting to default value
  for (Json::ValueConstIterator it = _accountSettingsConfig->begin(); it != _accountSettingsConfig->end(); ++it)
  {
    if (!_currentAccountSettings.isMember(it.name()))
    {
      const Json::Value& item = (*it);
      _currentAccountSettings[it.name()] = item[kConfigDefaultValueKey];
      settingsDirty = true;
      LOG_INFO("AccountSettingsManager.InitDependent.AddDefaultItem", "Adding setting with key %s and default value %s",
               it.name().c_str(), item[kConfigDefaultValueKey].asString().c_str());
    }
  }

  // Now look through current settings, and remove any item
  // that is no longer defined in the config
  std::vector<std::string> keysToRemove;
  for (Json::ValueConstIterator it = _currentAccountSettings.begin(); it != _currentAccountSettings.end(); ++it)
  {
    if (!_accountSettingsConfig->isMember(it.name()))
    {
      keysToRemove.push_back(it.name());
    }
  }
  for (const auto& key : keysToRemove)
  {
    LOG_INFO("AccountSettingsManager.InitDependent.RemoveItem",
             "Removing setting with key %s", key.c_str());
    _currentAccountSettings.removeMember(key);
    settingsDirty = true;
  }

  if (settingsDirty)
  {
    static const bool kSaveToCloudImmediately = false;
    static const bool kSetCloudDirtyIfNotImmediate = !jdocNeedsCreation;
    UpdateAccountSettingsJdoc(kSaveToCloudImmediately, kSetCloudDirtyIfNotImmediate);
  }

  // Register the actual setting application methods, for those settings that want to execute code when changed:
  _settingSetters[external_interface::AccountSetting::DATA_COLLECTION] = { nullptr, &AccountSettingsManager::ApplyAccountSettingDataCollection };
  _settingSetters[external_interface::AccountSetting::APP_LOCALE]      = { &AccountSettingsManager::ValidateAccountSettingAppLocale, nullptr   };

  _jdocsManager->RegisterOverwriteNotificationCallback(external_interface::JdocType::ACCOUNT_SETTINGS, [this]() {
    _currentAccountSettings = _jdocsManager->GetJdocBody(external_interface::JdocType::ACCOUNT_SETTINGS);
    ApplyAllCurrentAccountSettings();
  });

  _jdocsManager->RegisterFormatMigrationCallback(external_interface::JdocType::ACCOUNT_SETTINGS, [this]() {
    DoJdocFormatMigration();
  });

  // Finally, set a flag so we will apply all of the settings
  // we just loaded and/or set, in the first update
  _applyAccountSettingsNextTick = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AccountSettingsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if (_applyAccountSettingsNextTick)
  {
    _applyAccountSettingsNextTick = false;
    ApplyAllCurrentAccountSettings();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::SetAccountSetting(const external_interface::AccountSetting accountSetting,
                                               const Json::Value& valueJson,
                                               const bool updateSettingsJdoc,
                                               bool& ignoredDueToNoChange)
{
  ignoredDueToNoChange = false;

  const std::string& key = AccountSetting_Name(accountSetting);
  if (!_currentAccountSettings.isMember(key))
  {
    LOG_ERROR("AccountSettingsManager.SetAccountSetting.InvalidKey", "Invalid key %s; ignoring", key.c_str());
    return false;
  }

  const Json::Value prevValue = _currentAccountSettings[key];
  if (prevValue == valueJson)
  {
    // If the value is not actually changing, don't do anything.
    ignoredDueToNoChange = true;
    return false;
  }
  _currentAccountSettings[key] = valueJson;

  bool success = ApplyAccountSetting(accountSetting);
  if (!success)
  {
    _currentAccountSettings[key] = prevValue;  // Restore previous good value
    return false;
  }

  if (updateSettingsJdoc)
  {
    const bool saveToCloudImmediately = DoesSettingUpdateCloudImmediately(accountSetting);
    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
    success = UpdateAccountSettingsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string AccountSettingsManager::GetAccountSettingAsString(const external_interface::AccountSetting key) const
{
  const std::string& keyString = AccountSetting_Name(key);
  if (!_currentAccountSettings.isMember(keyString))
  {
    LOG_ERROR("AccountSettingsManager.GetRobotSettingAsString.InvalidKey", "Invalid key %s", keyString.c_str());
    return "Invalid";
  }

  return _currentAccountSettings[keyString].asString();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::GetAccountSettingAsBool(const external_interface::AccountSetting key) const
{
  const std::string& keyString = AccountSetting_Name(key);
  if (!_currentAccountSettings.isMember(keyString))
  {
    LOG_ERROR("AccountSettingsManager.GetRobotSettingAsBool.InvalidKey", "Invalid key %s", keyString.c_str());
    return false;
  }

  return _currentAccountSettings[keyString].asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t AccountSettingsManager::GetAccountSettingAsUInt(const external_interface::AccountSetting key) const
{
  const std::string& keyString = AccountSetting_Name(key);
  if (!_currentAccountSettings.isMember(keyString))
  {
    LOG_ERROR("AccountSettingsManager.GetRobotSettingAsUInt.InvalidKey", "Invalid key %s", keyString.c_str());
    return 0;
  }

  return _currentAccountSettings[keyString].asUInt();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::DoesSettingUpdateCloudImmediately(const external_interface::AccountSetting key) const
{
  const std::string& keyString = AccountSetting_Name(key);
  const auto& config = (*_accountSettingsConfig)[keyString];
  const bool saveToCloudImmediately = config[kConfigUpdateCloudOnChangeKey].asBool();
  return saveToCloudImmediately;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::UpdateAccountSettingsJdoc(const bool saveToCloudImmediately,
                                                       const bool setCloudDirtyIfNotImmediate)
{
  static const bool saveToDiskImmediately = true;
  const bool success = _jdocsManager->UpdateJdoc(external_interface::JdocType::ACCOUNT_SETTINGS,
                                                 &_currentAccountSettings,
                                                 saveToDiskImmediately,
                                                 saveToCloudImmediately,
                                                 setCloudDirtyIfNotImmediate);
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AccountSettingsManager::ApplyAllCurrentAccountSettings()
{
  LOG_INFO("AccountSettingsManager.ApplyAllCurrentAccountSettings", "Applying all current account settings");
  for (Json::ValueConstIterator it = _currentAccountSettings.begin(); it != _currentAccountSettings.end(); ++it)
  {
    external_interface::AccountSetting whichSetting;
    AccountSetting_Parse(it.name(), &whichSetting);
    ApplyAccountSetting(whichSetting);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::ApplyAccountSetting(const external_interface::AccountSetting setting)
{
  // Actually apply the setting; note that some things don't need to be "applied"
  bool success = true;
  const auto it = _settingSetters.find(setting);
  if (it != _settingSetters.end())
  {
    const auto validationFunc = it->second.validationFunction;
    if (validationFunc != nullptr)
    {
      success = (this->*(validationFunc))();
      if (!success)
      {
        LOG_ERROR("AccountSettingsManager.ApplyAccountSetting.ValidateFunctionFailed", "Error attempting to apply %s setting",
                  AccountSetting_Name(setting).c_str());
        return false;
      }
    }

    const auto applicationFunc = it->second.applicationFunction;
    if (applicationFunc != nullptr)
    {
      LOG_DEBUG("AccountSettingsManager.ApplyAccountSetting", "Applying Account Setting '%s'",
                AccountSetting_Name(setting).c_str());
      success = (this->*(applicationFunc))();
      
      if (!success)
      {
        LOG_ERROR("AccountSettingsManager.ApplyAccountSetting.ApplyFunctionFailed", "Error attempting to apply %s setting",
                  AccountSetting_Name(setting).c_str());
      }
    }
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::ApplyAccountSettingDataCollection()
{
  static const std::string& key = AccountSetting_Name(external_interface::AccountSetting::DATA_COLLECTION);
  const bool value = _currentAccountSettings[key].asBool();
  LOG_INFO("AccountSettingsManager.ApplyAccountSettingDataCollection.Apply",
           "Setting data collection flag to %s", value ? "true" : "false");

  // Publish choice to DAS manager
  EnableDataCollection(value);
  // Send message to mic system in anim process
  ASSERT_NAMED(_robot != nullptr, "AccountSettingsManager.ApplyAccountSettingDataCollection._robot.IsNull");
  if (_robot != nullptr) {
    using namespace RobotInterface;
    const auto msg = EngineToRobot::CreateupdatedAccountSettings(UpdatedAccountSettings(value));
    _robot->SendMessage(std::move(msg));
  }
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AccountSettingsManager::ValidateAccountSettingAppLocale()
{
  static const std::string& key = AccountSetting_Name(external_interface::AccountSetting::APP_LOCALE);
  const auto& value = _currentAccountSettings[key].asString();
  if (!Anki::Util::Locale::IsValidLocaleString(value))
  {
    LOG_ERROR("AccountSettingsManager.ValidateAccountSettingAppLocale.Invalid",
              "Invalid locale: %s", value.c_str());
    return false;
  }
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AccountSettingsManager::DoJdocFormatMigration()
{
  const auto jdocType = external_interface::JdocType::ACCOUNT_SETTINGS;
  const auto docFormatVersion = _jdocsManager->GetJdocFmtVersion(jdocType);
  const auto curFormatVersion = _jdocsManager->GetCurFmtVersion(jdocType);
  LOG_INFO("AccountSettingsManager.DoJdocFormatMigration",
           "Migrating account settings jdoc from format version %llu to %llu",
           docFormatVersion, curFormatVersion);
  if (docFormatVersion > curFormatVersion)
  {
    LOG_ERROR("AccountSettingsManager.DoJdocFormatMigration.Error",
              "Jdoc format version is newer than what victor code can handle; no migration possible");
    return;
  }

  // When we change 'format version' on this jdoc, migration
  // to a newer format version is performed here

  // Now update the format version of this jdoc to the current format version
  _jdocsManager->SetJdocFmtVersionToCurrent(jdocType);
}


} // namespace Vector
} // namespace Anki
