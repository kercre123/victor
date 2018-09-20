/**
 * File: accountSettingsManager.h
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

#ifndef __Victor_Basestation_Components_AccountSettingsManager_H__
#define __Victor_Basestation_Components_AccountSettingsManager_H__

#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"

#include "proto/external_interface/settings.pb.h"

#include "json/json.h"
#include <map>

namespace Anki {
namespace Vector {

class JdocsManager;

class AccountSettingsManager : public IDependencyManagedComponent<RobotComponentID>,
                               private Anki::Util::noncopyable,
                               private Util::SignalHolder
{
public:
  AccountSettingsManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::JdocsManager);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  // Returns true if successful
  bool SetAccountSetting(const external_interface::AccountSetting accountSetting,
                         const Json::Value& valueJson,
                         const bool updateSettingsJdoc,
                         bool& ignoredDueToNoChange);

  // Return the account setting value (currently strings, bools, uints supported)
  std::string GetAccountSettingAsString(const external_interface::AccountSetting key) const;
  bool        GetAccountSettingAsBool  (const external_interface::AccountSetting key) const;
  uint32_t    GetAccountSettingAsUInt  (const external_interface::AccountSetting key) const;
  
  bool DoesSettingUpdateCloudImmediately(const external_interface::AccountSetting key) const;

  bool UpdateAccountSettingsJdoc(const bool saveToCloudImmediately,
                                 const bool setCloudDirtyIfNotImmediate);

private:

  void ApplyAllCurrentAccountSettings();
  bool ApplyAccountSetting(const external_interface::AccountSetting key);
  bool ApplyAccountSettingDataCollection();
  bool ValidateAccountSettingAppLocale();

  void DoJdocFormatMigration();

  Json::Value               _currentAccountSettings;
  const Json::Value*        _accountSettingsConfig;
  bool                      _applyAccountSettingsNextTick = false;
  
  using SettingFunction = bool (AccountSettingsManager::*)();
  struct SettingSetter
  {
    SettingFunction         validationFunction;
    SettingFunction         applicationFunction;
  };
  using SettingSetters = std::map<external_interface::AccountSetting, SettingSetter>;
  SettingSetters            _settingSetters;

  JdocsManager*             _jdocsManager = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // __Victor_Basestation_Components_AccountSettingsManager_H__
