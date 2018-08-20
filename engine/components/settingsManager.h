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
#include "util/signals/signalHolder.h"

#include "clad/types/robotSettingsTypes.h"

#include "json/json.h"
#include <map>

namespace Anki {
namespace Vector {

class JdocsManager;

class SettingsManager : public IDependencyManagedComponent<RobotComponentID>,
                        private Anki::Util::noncopyable,
                        private Util::SignalHolder
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
    dependencies.insert(RobotComponentID::AIComponent);
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

  // Return the setting value (currently strings, bools, uints supported)
  std::string GetRobotSettingAsString(const RobotSetting key) const;
  bool        GetRobotSettingAsBool  (const RobotSetting key) const;
  uint32_t    GetRobotSettingAsUInt  (const RobotSetting key) const;
  
  bool DoesSettingUpdateCloudImmediately(const RobotSetting key) const;

  bool UpdateSettingsJdoc(const bool saveToCloudImmediately);

  //////
  // Some user settings need to be triggered from behaviors; these function help in dealing with latent setting change
  // requests.

  // test whether or not we have a pending request for a latent settings update
  bool IsSettingsUpdateRequestPending() const;
  bool IsSettingsUpdateRequestPending(RobotSetting setting) const;
  RobotSetting GetPendingSettingsUpdate() const;

  // claiming the pending update request tells the system that "somebody" is going to take care of this update so
  // there's no need to force the update if it's not set in time
  // ** note: if you claim a pending event, it is up to you to clear it when you're done!
  bool ClaimPendingSettingsUpdate(RobotSetting setting);
  // applying the update actually applies the settings change that was requested
  // note: does nothing if the requested setting was not formerly pending
  bool ApplyPendingSettingsUpdate(RobotSetting setting, bool clearRequest = true);
  // this will clear out the pending request to allow new requests to be made
  // ** note: if you claim a pending event, it is up to you to clear it when you're done!
  void ClearPendingSettingsUpdate();
  //////

private:

  void ApplyAllCurrentSettings();
  bool ApplyRobotSetting(const RobotSetting robotSetting, bool force = true);
  bool ApplySettingMasterVolume();
  bool ApplySettingEyeColor();
  bool ApplySettingLocale();
  bool ApplySettingTimeZone();
  bool ValidateSettingMasterVolume();
  bool ValidateSettingEyeColor();
  bool ExecCommand(const std::vector<std::string>& args);

  // request that the specified RobotSetting is not immediately applied, but some other source will apply it
  bool RequestLatentSettingsUpdate( RobotSetting setting );
  void OnSettingsUpdateNotClaimed(RobotSetting setting);

  Json::Value               _currentSettings;
  Robot*                    _robot = nullptr;
  Util::Data::DataPlatform* _platform = nullptr;
  const Json::Value*        _settingsConfig;
  bool                      _applySettingsNextTick = false;

  using SettingFunction = bool (SettingsManager::*)();
  struct SettingSetter
  {
    bool                    isLatentApplication;
    SettingFunction         validationFunction;
    SettingFunction         applicationFunction;
  };
  using SettingSetters = std::map<RobotSetting, SettingSetter>;
  SettingSetters            _settingSetters;

  struct LatentSettingsRequest
  {
    RobotSetting            setting;
    size_t                  tickRequested;
    bool                    isClaimed;
  };

  bool                      _hasPendingSettingsRequest;
  LatentSettingsRequest     _settingsUpdateRequest;

  JdocsManager*                  _jdocsManager = nullptr;
  Audio::EngineRobotAudioClient* _audioClient = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Components_SettingsManager_H__
