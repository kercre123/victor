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
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class SettingsManager;


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

  bool HandleRobotSettingChangeRequest(const std::string& settingKey, const std::string& settingValue);

private:

  SettingsManager*    _settingsManager = nullptr;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_settingsCommManager_H__
