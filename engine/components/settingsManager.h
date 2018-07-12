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
namespace Cozmo {

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
                       const bool saveSettingsFile = true);

  // Return the setting value (currently only strings or bools supported)
  std::string GetRobotSettingAsString(const RobotSetting key) const;
  bool        GetRobotSettingAsBool  (const RobotSetting key) const;

private:

  // Load/save settings file from/to persistent robot storage
  bool LoadSettingsFile();
  void SaveSettingsFile() const;
  
  void ApplyAllCurrentSettings();
  bool ApplyRobotSetting(const RobotSetting robotSetting);
  bool ApplySettingMasterVolume();
  bool ApplySettingEyeColor();
  bool ApplySettingLocale();

  Json::Value               _currentSettings;
  Robot*                    _robot = nullptr;
  Util::Data::DataPlatform* _platform = nullptr;
  std::string               _savePath = "";
  std::string               _fullPathSettingsFile = "";
  bool                      _applySettingsNextTick = false;
  using SettingSetter = bool (SettingsManager::*)();
  using SettingSetters = std::map<RobotSetting, SettingSetter>;
  //  bool (SettingsManager::*_settingSetter)(const std::string& newValue) = nullptr;
  SettingSetters            _settingSetters;

  Audio::EngineRobotAudioClient* _audioClient = nullptr;
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Components_SettingsManager_H__
