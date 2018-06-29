/**
* File: settingsCommManager.h
*
* Author: Paul Terry
* Created: 6/15/18
*
* Description: Communicates settings with App and Cloud; calls into SettingsManager
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Components_settingsCommManager_H__
#define __Cozmo_Basestation_Components_settingsCommManager_H__

#include "engine/cozmoContext.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include "clad/types/robotSettingsTypes.h"

namespace Anki {
namespace Cozmo {

class SettingsManager;
class IGatewayInterface;


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
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

  bool HandleRobotSettingChangeRequest(const RobotSetting robotSetting,
                                       const Json::Value& settingJson);
  bool HandleRobotSettingToggleRequest(const RobotSetting robotSetting);

  void RefreshConsoleVars();

private:

  void HandleEvents(const AnkiEvent<external_interface::GatewayWrapper>& event);
  void OnRequestPullSettings  (const external_interface::PullSettingsRequest&    pullSettingsRequest);
  void OnRequestPushSettings  (const external_interface::PushSettingsRequest&    pushSettingsRequest);
  void OnRequestUpdateSettings(const external_interface::UpdateSettingsRequest&  updateSettingsRequest);

  SettingsManager*    _settingsManager = nullptr;
  IGatewayInterface*  _gatewayInterface = nullptr;

  std::vector<Signal::SmartHandle> _signalHandles;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_settingsCommManager_H__
