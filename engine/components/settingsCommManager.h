/**
 * File: settingsCommManager.h
 *
 * Author: Paul Terry
 * Created: 6/15/18
 *
 * Description: Communicates settings with App and Cloud; calls into SettingsManager
 * (for robot settings), AccountSettingsManager and UserEntitlementsManager
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Cozmo_Basestation_Components_settingsCommManager_H__
#define __Cozmo_Basestation_Components_settingsCommManager_H__

#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include "proto/external_interface/settings.pb.h"

#include "clad/types/robotSettingsTypes.h"

namespace Anki {
namespace Vector {

template <typename T>
class AnkiEvent;
class IGatewayInterface;
class SettingsManager;
class AccountSettingsManager;
class UserEntitlementsManager;
class JdocsManager;
namespace external_interface {
  class GatewayWrapper;
  class PullJdocsRequest;
  class UpdateSettingsRequest;
}

class SettingsCommManager : public IDependencyManagedComponent<RobotComponentID>,
                            private Anki::Util::noncopyable
{
public:
  SettingsCommManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::SettingsManager);
    dependencies.insert(RobotComponentID::JdocsManager);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  bool HandleRobotSettingChangeRequest   (const RobotSetting robotSetting,
                                          const Json::Value& settingJson,
                                          const bool updateSettingsJdoc = false);
  bool ToggleRobotSettingHelper          (const RobotSetting robotSetting);

  bool HandleAccountSettingChangeRequest (const external_interface::AccountSetting accountSetting,
                                          const Json::Value& settingJson,
                                          const bool updateSettingsJdoc = false);
  bool ToggleAccountSettingHelper        (const external_interface::AccountSetting accountSetting);

  bool HandleUserEntitlementChangeRequest(const external_interface::UserEntitlement userEntitlement,
                                          const Json::Value& settingJson,
                                          const bool updateUserEntitlementsJdoc = false);
  bool ToggleUserEntitlementHelper       (const external_interface::UserEntitlement userEntitlement);

  void RefreshConsoleVars();

private:

  void HandleEvents                   (const AnkiEvent<external_interface::GatewayWrapper>& event);
  void OnRequestPullJdocs             (const external_interface::PullJdocsRequest& pullJdocsRequest);
  void OnRequestUpdateSettings        (const external_interface::UpdateSettingsRequest& updateSettingsRequest);
  void OnRequestUpdateAccountSettings (const external_interface::UpdateAccountSettingsRequest& updateAccountSettingsRequest);
  void OnRequestUpdateUserEntitlements(const external_interface::UpdateUserEntitlementsRequest& updateUserEntitlementsRequest);

  SettingsManager*         _settingsManager = nullptr;
  AccountSettingsManager*  _accountSettingsManager = nullptr;
  UserEntitlementsManager* _userEntitlementsManager = nullptr;
  JdocsManager*            _jdocsManager = nullptr;
  IGatewayInterface*       _gatewayInterface = nullptr;

  std::vector<Signal::SmartHandle> _signalHandles;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Components_settingsCommManager_H__
