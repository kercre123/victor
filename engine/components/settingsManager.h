/**
* File: settingsManager.h
*
* Author: Paul Terry
* Created: 6/8/18
*
* Description: Stores robot settings on robot; accepts and sets new settings
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_Components_SettingsManager_H__
#define __Cozmo_Basestation_Components_SettingsManager_H__

#include "engine/audio/engineRobotAudioClient.h"
#include "engine/cozmoContext.h"
#include "engine/robotComponents_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "clad/types/robotSettingsTypes.h"

#include <map>

namespace Anki {
namespace Vector {

class JdocsManager;

class SettingsManager : public IDependencyManagedComponent<RobotComponentID>,
                        private Anki::Util::noncopyable
{
public:
  SettingsManager();

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
  bool SetRobotSetting(const RobotSetting robotSetting,
                       const Json::Value& valueJson,
                       const bool updateSettingsJdoc);

  // Return the setting value (currently only strings or bools supported)
  std::string GetRobotSettingAsString(const RobotSetting key) const;
  bool        GetRobotSettingAsBool  (const RobotSetting key) const;

  bool UpdateSettingsJdoc();
  
private:

  void ApplyAllCurrentSettings();
  bool ApplyRobotSetting(const RobotSetting robotSetting);
  bool ApplySettingMasterVolume();
  bool ApplySettingEyeColor();
  bool ApplySettingLocale();
  bool ApplySettingTimeZone();
  bool ExecCommand(const std::vector<std::string>& args);

  Json::Value               _currentSettings;
  Robot*                    _robot = nullptr;
  Util::Data::DataPlatform* _platform = nullptr;
  bool                      _applySettingsNextTick = false;
  using SettingSetter = bool (SettingsManager::*)();
  using SettingSetters = std::map<RobotSetting, SettingSetter>;
  SettingSetters            _settingSetters;

  JdocsManager*                  _jdocsManager = nullptr;
  Audio::EngineRobotAudioClient* _audioClient = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Components_SettingsManager_H__
