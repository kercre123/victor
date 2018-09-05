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

#define LOG_CHANNEL "UserEntitlementsManager"

namespace Anki {
namespace Vector {

namespace
{
  static const char* kConfigDefaultValueKey = "defaultValue";
  static const char* kConfigUpdateCloudOnChangeKey = "updateCloudOnChange";
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

  _userEntitlementsConfig = &robot->GetContext()->GetDataLoader()->GetUserEntitlementsConfig();

  // Call the JdocsManager to see if our user entitlements jdoc file exists
  bool userEntitlementsDirty = false;
  const bool jdocNeedsCreation = _jdocsManager->JdocNeedsCreation(external_interface::JdocType::USER_ENTITLEMENTS);
  _currentUserEntitlements.clear();
  if (jdocNeedsCreation)
  {
    LOG_INFO("UserEntitlementsManager.InitDependent.NoUserEntitlementsJdocFile",
             "User entitlements jdoc file not found; one will be created shortly");
  }
  else
  {
    _currentUserEntitlements = _jdocsManager->GetJdocBody(external_interface::JdocType::USER_ENTITLEMENTS);

    if (_jdocsManager->JdocNeedsMigration(external_interface::JdocType::USER_ENTITLEMENTS))
    {
      DoJdocFormatMigration();
      userEntitlementsDirty = true;
    }
  }

  // Ensure current user entitlements has each of the defined user entitlements;
  // if not, initialize each missing user entitlement to default value
  for (Json::ValueConstIterator it = _userEntitlementsConfig->begin(); it != _userEntitlementsConfig->end(); ++it)
  {
    if (!_currentUserEntitlements.isMember(it.name()))
    {
      const Json::Value& item = (*it);
      _currentUserEntitlements[it.name()] = item[kConfigDefaultValueKey];
      userEntitlementsDirty = true;
      LOG_INFO("UserEntitlementsManager.InitDependent.AddDefaultItem", "Adding user entitlement with key %s and default value %s",
               it.name().c_str(), item[kConfigDefaultValueKey].asString().c_str());
    }
  }

  // Now look through current user entitlements, and remove any item
  // that is no longer defined in the config
  std::vector<std::string> keysToRemove;
  for (Json::ValueConstIterator it = _currentUserEntitlements.begin(); it != _currentUserEntitlements.end(); ++it)
  {
    if (!_userEntitlementsConfig->isMember(it.name()))
    {
      keysToRemove.push_back(it.name());
    }
  }
  for (const auto& key : keysToRemove)
  {
    LOG_INFO("UserEntitlementsManager.InitDependent.RemoveItem",
             "Removing user entitlement with key %s", key.c_str());
    _currentUserEntitlements.removeMember(key);
    userEntitlementsDirty = true;
  }

  if (userEntitlementsDirty)
  {
    static const bool kSaveToCloudImmediately = false;
    static const bool kSetCloudDirtyIfNotImmediate = !jdocNeedsCreation;
    UpdateUserEntitlementsJdoc(kSaveToCloudImmediately, kSetCloudDirtyIfNotImmediate);
  }

  // Register the actual user entitlement application methods, for those user entitlements that want to execute code when changed:
  _settingSetters[external_interface::UserEntitlement::KICKSTARTER_EYES] = { nullptr, &UserEntitlementsManager::ApplyUserEntitlementKickstarterEyes };

  _jdocsManager->RegisterOverwriteNotificationCallback(external_interface::JdocType::USER_ENTITLEMENTS, [this]() {
    _currentUserEntitlements = _jdocsManager->GetJdocBody(external_interface::JdocType::USER_ENTITLEMENTS);
    ApplyAllCurrentUserEntitlements();
  });

  _jdocsManager->RegisterFormatMigrationCallback(external_interface::JdocType::USER_ENTITLEMENTS, [this]() {
    DoJdocFormatMigration();
  });

  // Finally, set a flag so we will apply all of the user entitlements
  // we just loaded and/or set, in the first update
  _applyUserEntitlementsNextTick = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserEntitlementsManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  if (_applyUserEntitlementsNextTick)
  {
    _applyUserEntitlementsNextTick = false;
    ApplyAllCurrentUserEntitlements();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::SetUserEntitlement(const external_interface::UserEntitlement userEntitlement,
                                                 const Json::Value& valueJson,
                                                 const bool updateUserEntitlementsJdoc)
{
  const std::string& key = external_interface::UserEntitlement_Name(userEntitlement);
  if (!_currentUserEntitlements.isMember(key))
  {
    LOG_ERROR("UserEntitlementsManager.SetUserEntitlement.InvalidKey", "Invalid key %s; ignoring", key.c_str());
    return false;
  }

  const Json::Value prevValue = _currentUserEntitlements[key];
  _currentUserEntitlements[key] = valueJson;

  bool success = ApplyUserEntitlement(userEntitlement);
  if (!success)
  {
    _currentUserEntitlements[key] = prevValue;  // Restore previous good value
    return false;
  }

  if (updateUserEntitlementsJdoc)
  {
    const bool saveToCloudImmediately = DoesUserEntitlementUpdateCloudImmediately(userEntitlement);
    const bool setCloudDirtyIfNotImmediate = saveToCloudImmediately;
    success = UpdateUserEntitlementsJdoc(saveToCloudImmediately, setCloudDirtyIfNotImmediate);
  }

  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string UserEntitlementsManager::GetUserEntitlementAsString(const external_interface::UserEntitlement key) const
{
  const std::string& keyString = external_interface::UserEntitlement_Name(key);
  if (!_currentUserEntitlements.isMember(keyString))
  {
    LOG_ERROR("UserEntitlementsManager.GetUserEntitlementAsString.InvalidKey", "Invalid key %s", keyString.c_str());
    return "Invalid";
  }

  return _currentUserEntitlements[keyString].asString();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::GetUserEntitlementAsBool(const external_interface::UserEntitlement key) const
{
  const std::string& keyString = external_interface::UserEntitlement_Name(key);
  if (!_currentUserEntitlements.isMember(keyString))
  {
    LOG_ERROR("UserEntitlementsManager.GetUserEntitlementAsBool.InvalidKey", "Invalid key %s", keyString.c_str());
    return false;
  }

  return _currentUserEntitlements[keyString].asBool();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t UserEntitlementsManager::GetUserEntitlementAsUInt(const external_interface::UserEntitlement key) const
{
  const std::string& keyString = external_interface::UserEntitlement_Name(key);
  if (!_currentUserEntitlements.isMember(keyString))
  {
    LOG_ERROR("UserEntitlementsManager.GetUserEntitlementAsUInt.InvalidKey", "Invalid key %s", keyString.c_str());
    return 0;
  }

  return _currentUserEntitlements[keyString].asUInt();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::DoesUserEntitlementUpdateCloudImmediately(const external_interface::UserEntitlement key) const
{
  const std::string& keyString = external_interface::UserEntitlement_Name(key);
  const auto& config = (*_userEntitlementsConfig)[keyString];
  const bool saveToCloudImmediately = config[kConfigUpdateCloudOnChangeKey].asBool();
  return saveToCloudImmediately;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::UpdateUserEntitlementsJdoc(const bool saveToCloudImmediately,
                                                         const bool setCloudDirtyIfNotImmediate)
{
  static const bool saveToDiskImmediately = true;
  const bool success = _jdocsManager->UpdateJdoc(external_interface::JdocType::USER_ENTITLEMENTS,
                                                 &_currentUserEntitlements,
                                                 saveToDiskImmediately,
                                                 saveToCloudImmediately,
                                                 setCloudDirtyIfNotImmediate);
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserEntitlementsManager::ApplyAllCurrentUserEntitlements()
{
  LOG_INFO("UserEntitlementsManager.ApplyAllCurrentUserEntitlements", "Applying all current user entitlements");
  for (Json::ValueConstIterator it = _currentUserEntitlements.begin(); it != _currentUserEntitlements.end(); ++it)
  {
    external_interface::UserEntitlement whichUserEntitlement;
    UserEntitlement_Parse(it.name(), &whichUserEntitlement);
    ApplyUserEntitlement(whichUserEntitlement);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::ApplyUserEntitlement(const external_interface::UserEntitlement key)
{
  // Actually apply the user entitlement; note that some things don't need to be "applied"
  bool success = true;
  const auto it = _settingSetters.find(key);
  if (it != _settingSetters.end())
  {
    const auto validationFunc = it->second.validationFunction;
    if (validationFunc != nullptr)
    {
      success = (this->*(validationFunc))();
      if (!success)
      {
        LOG_ERROR("UserEntitlementsManager.ApplyUserEntitlement.ValidateFunctionFailed", "Error attempting to apply %s user entitlement",
                  external_interface::UserEntitlement_Name(key).c_str());
        return false;
      }
    }

    LOG_DEBUG("UserEntitlementsManager.ApplyUserEntitlement", "Applying user entitlement '%s'",
              external_interface::UserEntitlement_Name(key).c_str());
    success = (this->*(it->second.applicationFunction))();

    if (!success)
    {
      LOG_ERROR("UserEntitlementsManager.ApplyUserEntitlement.ApplyFunctionFailed", "Error attempting to apply %s user entitlement",
                external_interface::UserEntitlement_Name(key).c_str());
    }
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserEntitlementsManager::ApplyUserEntitlementKickstarterEyes()
{
  static const std::string& key = external_interface::UserEntitlement_Name(external_interface::UserEntitlement::KICKSTARTER_EYES);
  const auto& value = _currentUserEntitlements[key].asBool();
  LOG_INFO("UserEntitlementsManager.ApplyUserEntitlementKickstarterEyes.Apply",
           "Setting kickstarter eyes flag to %s", value ? "true" : "false");
  
  // TODO:  Can call whereever here
  
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserEntitlementsManager::DoJdocFormatMigration()
{
  const auto jdocType = external_interface::JdocType::USER_ENTITLEMENTS;
  const auto docFormatVersion = _jdocsManager->GetJdocFmtVersion(jdocType);
  const auto curFormatVersion = _jdocsManager->GetCurFmtVersion(jdocType);
  LOG_INFO("UserEntitlementsManager.DoJdocFormatMigration",
           "Migrating user entitlements jdoc from format version %llu to %llu",
           docFormatVersion, curFormatVersion);
  if (docFormatVersion > curFormatVersion)
  {
    LOG_ERROR("UserEntitlementsManager.DoJdocFormatMigration.Error",
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
