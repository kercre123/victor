/**
 * File: userEntitlementsManager.h
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

#ifndef __Victor_Basestation_Components_UserEntitlementsManager_H__
#define __Victor_Basestation_Components_UserEntitlementsManager_H__

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

class UserEntitlementsManager : public IDependencyManagedComponent<RobotComponentID>,
                                private Anki::Util::noncopyable,
                                private Util::SignalHolder
{
public:
  UserEntitlementsManager();

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
  bool SetUserEntitlement(const external_interface::UserEntitlement userEntitlement,
                          const Json::Value& valueJson,
                          const bool updateUserEntitlementsJdoc,
                          bool& ignoredDueToNoChange);

  // Return the user entitlement value (currently strings, bools, uints supported)
  std::string GetUserEntitlementAsString(const external_interface::UserEntitlement key) const;
  bool        GetUserEntitlementAsBool  (const external_interface::UserEntitlement key) const;
  uint32_t    GetUserEntitlementAsUInt  (const external_interface::UserEntitlement key) const;

  bool DoesUserEntitlementUpdateCloudImmediately(const external_interface::UserEntitlement key) const;

  bool UpdateUserEntitlementsJdoc(const bool saveToCloudImmediately,
                                  const bool setCloudDirtyIfNotImmediate);

private:

  void ApplyAllCurrentUserEntitlements();
  bool ApplyUserEntitlement(const external_interface::UserEntitlement key);
  bool ApplyUserEntitlementKickstarterEyes();

  void DoJdocFormatMigration();

  Json::Value               _currentUserEntitlements;
  Robot*                    _robot = nullptr;
  const Json::Value*        _userEntitlementsConfig;
  bool                      _applyUserEntitlementsNextTick = false;

  using SettingFunction = bool (UserEntitlementsManager::*)();
  struct SettingSetter
  {
    SettingFunction         validationFunction;
    SettingFunction         applicationFunction;
  };
  using SettingSetters = std::map<external_interface::UserEntitlement, SettingSetter>;
  SettingSetters            _settingSetters;

  JdocsManager*             _jdocsManager = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // __Victor_Basestation_Components_UserEntitlementsManager_H__
