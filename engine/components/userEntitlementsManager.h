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
//  bool SetAccountSetting(/*const RobotSetting robotSetting,*/
//                         const Json::Value& valueJson,
//                         const bool updateSettingsJdoc);

  // Return the account setting value (currently strings, bools, uints supported)
//  std::string GetAccountSettingAsString(const RobotSetting key) const;
//  bool        GetAccountSettingAsBool  (const RobotSetting key) const;
//  uint32_t    GetAccountSettingAsUInt  (const RobotSetting key) const;
  
//  bool DoesSettingUpdateCloudImmediately(const RobotSetting key) const;

  bool UpdateUserEntitlementsJdoc(const bool saveToCloudImmediately,
                                  const bool setCloudDirtyIfNotImmediate);

private:

  Json::Value               _currentUserEntitlements;
  Robot*                    _robot = nullptr;
//  const Json::Value*        _userEntitlementsConfig;

  JdocsManager*                  _jdocsManager = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // __Victor_Basestation_Components_UserEntitlementsManager_H__
