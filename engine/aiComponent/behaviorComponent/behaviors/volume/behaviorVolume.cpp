/**
 * File: BehaviorVolume.cpp
 *
 * Author: Andrew Stout
 * Created: 2018-10-11
 *
 * Description: Respond to volume change requests.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "behaviorVolume.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/settingsCommManager.h"
#include "engine/components/settingsManager.h"
#include "engine/robot.h"
#include "proto/external_interface/settings.pb.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"


#define LOG_CHANNEL "BehaviorVolume"

namespace Anki {
namespace Vector {


namespace {
  // confusingly, "low" and "high" were chosen to represent intermediate
  // levels in the enum coming from the cloud, and then later it was
  // decided that "low" and "high" would continue to represent extrema
  // in the app and in verbal semantics.
  // update: this interface is a moving target. now changed to VOLUME_1 through VOLUME_5...
  // (once it's stabilized we could remove the older ones...)
  const std::map<std::string, EVolumeLevel> kVolumeLevelMap {//{"mute", EVolumeLevel::MUTE}, // don't ever actually use "mute"
                                                            {"minimum", EVolumeLevel::MIN},
                                                            {"min", EVolumeLevel::MIN},
                                                            {"VOLUME_1", EVolumeLevel::MIN},
                                                            {"low", EVolumeLevel::MEDLOW}, 
                                                            {"VOLUME_2", EVolumeLevel::MEDLOW},
                                                            {"medium", EVolumeLevel::MED},
                                                            {"VOLUME_3", EVolumeLevel::MED},
                                                            {"high", EVolumeLevel::MEDHIGH},
                                                            {"VOLUME_4", EVolumeLevel::MEDHIGH},
                                                            {"maximum", EVolumeLevel::MAX},
                                                            {"max", EVolumeLevel::MAX},
                                                            {"VOLUME_5", EVolumeLevel::MAX}};
  const std::map<EVolumeLevel, AnimationTrigger> kVolumeLevelAnimMap {{EVolumeLevel::MIN, AnimationTrigger::VolumeLevel1},
                                                                    {EVolumeLevel::MEDLOW, AnimationTrigger::VolumeLevel2},
                                                                    {EVolumeLevel::MED, AnimationTrigger::VolumeLevel3},
                                                                    {EVolumeLevel::MEDHIGH, AnimationTrigger::VolumeLevel4},
                                                                    {EVolumeLevel::MAX, AnimationTrigger::VolumeLevel5}};
  const float kVolumeChangeNotificationAgeLimit_s = 2.0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVolume::DynamicVariables::DynamicVariables()
  : lastVolumeChangeNotification_s(0.0)
  , volumeChangeReactorId(0)
{}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVolume::BehaviorVolume(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVolume::~BehaviorVolume()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVolume::WantsToBeActivatedBehavior() const
{
  // check the different intents that should be handled here
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool volumeLevelPending = uic.IsUserIntentPending(USER_INTENT(imperative_volumelevel));
  const bool volumeUpPending = uic.IsUserIntentPending(USER_INTENT(imperative_volumeup));
  const bool volumeDownPending = uic.IsUserIntentPending(USER_INTENT(imperative_volumedown));
  // we don't want to respond to an external notification if there's another user intent pending
  const bool externalNotificationPending = ( ExternalNotificationPending() && !uic.IsAnyUserIntentPending() );
  
  return volumeLevelPending || volumeUpPending || volumeDownPending || externalNotificationPending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  /*
  const char* list[] = {
    // TODO: insert any possible root-level json keys that this class is expecting.
    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::OnBehaviorActivated() 
{
  // to get the identity of the pending intent I basically have to do this check again
  UserIntentPtr intentData = nullptr;
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  SettingsManager& settings = GetBEI().GetSettingsManager();
  EVolumeLevel newVolumeLevel;

  if ( ExternalNotificationPending()) {
    // a little quirk: we don't use the volume from the volume change notification.
    // instead, we read it again from settings, to get the most up to date volume
    // (one less piece of state to keep around in a potentially invalid state...)
    const uint32_t newVol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
    newVolumeLevel = static_cast<EVolumeLevel>(newVol);
  } else {
    EVolumeLevel desiredVolume;
    if (uic.IsUserIntentPending(USER_INTENT(imperative_volumelevel))) {
      intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumelevel));
      bool valid = false;
      desiredVolume = ComputeDesiredVolumeFromLevelIntent(intentData, valid);
      // if it's invalid, don't set the volume or do the animation
      if (!valid) {
        return;
      }
    } else if (uic.IsUserIntentPending(USER_INTENT(imperative_volumeup))) {
      intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumeup));
      desiredVolume = ComputeDesiredVolumeFromIncrement(true); // increment
    } else if (uic.IsUserIntentPending(USER_INTENT(imperative_volumedown))) {
      intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumedown));
      desiredVolume = ComputeDesiredVolumeFromIncrement(false); // decrement
    } else {
      LOG_WARNING("BehaviorVolume.OnBehaviorActivated.NoRecognizedPendingIntent",
                  "No recognized pending intent for volume. (Or maybe an external notification expired.)");
      return;
    }

    const uint32_t oldVol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
    if (static_cast<uint32_t>(desiredVolume) != oldVol){
      // set desired volume
      SetVolume(desiredVolume);
    }
    const uint32_t newVol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
    newVolumeLevel = static_cast<EVolumeLevel>(newVol);
    if (newVolumeLevel != desiredVolume) {
      LOG_WARNING("BehaviorVolume.OnBehavrioActivated.setFailed",
                  "New volume level %u does not match desired volume level %u", newVolumeLevel, desiredVolume);
    }
    // issue DAS event
    DASMSG(robot_settings_volume, "robot.settings.volume", "The robot's volume setting was changed");
    DASMSG_SET(i1, oldVol, "Old volume");
    DASMSG_SET(i2, newVol, "New volume");
    // NOTE: we only do this for voice-sourced intents. DAS for app-sourced happens in SettingsCommManager
    // and we only need to do the animation (below)
    DASMSG_SET(s1, "voice", "Source of the change (app, voice, or SDK)");
    DASMSG_SEND();
  } // close else (externalVolumeChangeNotificationPending)

  // delegate to play an animation (sequence)
  const auto it = kVolumeLevelAnimMap.find(newVolumeLevel);
  if(it == kVolumeLevelAnimMap.end()){
    // we don't have an anim designated for this volume level (probably because it's MUTE)
    LOG_WARNING("BehaviorVolume.OnBehaviorActivated.NoAnimForLevel",
                "No animation mapped for volume level %u", newVolumeLevel);
    return;
  }
  const AnimationTrigger animTrigger = it->second;
  TriggerLiftSafeAnimationAction* animation = new TriggerLiftSafeAnimationAction(animTrigger);
  DelegateIfInControl(animation);
                     
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::OnBehaviorDeactivated()
{
  // clear lastVolumeChangeNotification_s here! that will clear any pending notifications
  _dVars.lastVolumeChangeNotification_s = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::OnBehaviorEnteredActivatableScope()
{
  // register volume change notification callback here
  SettingsCommManager& settingsCommManager = GetBEI().GetSettingsCommManager();
  _dVars.volumeChangeReactorId = settingsCommManager.RegisterVolumeChangeReactor(
      std::bind( &BehaviorVolume::OnVolumeChange, this) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::OnBehaviorLeftActivatableScope()
{
  // unregister volume change notification callback here
  SettingsCommManager& settingsCommManager = GetBEI().GetSettingsCommManager();
  settingsCommManager.UnRegisterVolumeChangeReactor(_dVars.volumeChangeReactorId);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
    
  }
  // TODO: delete this function if you don't need it
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorVolume::SetVolume(EVolumeLevel desiredVolumeEnum)
{
  // Let's do some range checking here
  // should really be unnecessary now that we're using the enum, but let's be safe
  if ( (desiredVolumeEnum < EVolumeLevel::MIN) || (desiredVolumeEnum > EVolumeLevel::MAX) ) {
    LOG_WARNING("BehaviorVolume.SetVolume.OutsidePermittedRange",
                "Requested volume %u outside permitted range of [%u,%u].", desiredVolumeEnum, EVolumeLevel::MIN, EVolumeLevel::MAX);
    return false;
  }

  // check whether there's actually a change, if not, don't bother setting?
  // Not checking for now--SettingsManager handles that gracefully

  // cast the enum to uint32_t. Bonus! It's already a uint32_t, this is just to be really explicit
  uint32_t desiredVolume = static_cast<uint32_t>(desiredVolumeEnum);

  SettingsManager& settings = GetBEI().GetSettingsManager();
  bool ignoredDueToNoChange;
  const bool success = settings.SetRobotSetting(external_interface::RobotSetting::master_volume,
                            desiredVolume,
                            true,
                            ignoredDueToNoChange);
  if(success) {
    LOG_INFO("BehaviorVolume.SetVolume.Success",
                "Successfully changed volume to %u.", desiredVolume);
  } else {
    LOG_INFO("BehaviorVolume.SetVolume.Failure",
                "Failed to change volume.");
  }
  if(ignoredDueToNoChange) {
    LOG_INFO("BehaviorVolume.SetVolume.NoChange",
                "SettingsManager reported no change in volume setting.");
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
EVolumeLevel BehaviorVolume::ComputeDesiredVolumeFromLevelIntent(UserIntentPtr intentData, bool& valid)
{
  const std::string levelRequest = intentData->intent.Get_imperative_volumelevel().volume_level;
  EVolumeLevel desiredVol;
  const auto it = kVolumeLevelMap.find(levelRequest);
  if (it != kVolumeLevelMap.end()){
    valid = true;
    desiredVol = it->second;
    LOG_DEBUG("BehaviorVolume.ComputeDesiredVolumeFromLevelIntent.desiredVol",
                "mapped level request %s to desiredVol %u", levelRequest.c_str(), desiredVol);
  } else {
    // invalid level request
    valid = false;
    LOG_WARNING("BehaviorVolume.ComputeDesiredVolumeFromLevelIntent.invalid",
                "unexpected user intent volume_level: %s", levelRequest.c_str());
    // what should we return in this case? (since we don't want to throw an exception)
    // let's not return invalid data, just in case it gets used. 
    // Rather, get the current volume and return that. A bit roundabout, but oh well...
    SettingsManager& settings = GetBEI().GetSettingsManager();
    uint32_t vol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
    desiredVol = static_cast<EVolumeLevel>(vol);
  }  
  return desiredVol;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EVolumeLevel BehaviorVolume::ComputeDesiredVolumeFromIncrement(bool positiveIncrement)
{
  // read volume from settings
  SettingsManager& settings = GetBEI().GetSettingsManager();
  uint32_t vol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
  if ( (vol < static_cast<uint32_t>(EVolumeLevel::MIN) ) || (vol > static_cast<uint32_t>(EVolumeLevel::MAX)) ){
    LOG_WARNING("BehaviorVolume.ComputeDesiredVolumeFromIncrement.unexpectedCurrentVolume",
                "Volume read from settings was outside expected range: %u", vol);
    if (vol < 1 && !positiveIncrement){
      // we're about to try to decrement an unsigned zero, which would underflow, which would be bad
      // so let's not do that. Instead, since we're already (unexpectedly) at MUTE, return MUTE
      return EVolumeLevel::MUTE;
    }
  }

  // apply increment
  int increment = positiveIncrement ? 1 : -1;
  uint32_t newVol = vol + increment; 
  // check bounds
  if (newVol < static_cast<uint32_t>(EVolumeLevel::MIN) ) {
    newVol = static_cast<uint32_t>(EVolumeLevel::MIN);
    LOG_INFO("BehaviorVolume.ComputeDesiredVolumeFromIncrement.out_of_range",
              "volume is already at minimum level");
  }
  if (newVol > static_cast<uint32_t>(EVolumeLevel::MAX) ) {
    newVol = static_cast<uint32_t>(EVolumeLevel::MAX);
    LOG_INFO("BehaviorVolume.ComputeDesiredVolumeFromIncrement.out_of_range",
              "volume is already at maximum level");
  }

  // cast to EVolumeLevel. The enum is already a uint32_t, this is just to be really explicit
  EVolumeLevel desiredVolume = static_cast<EVolumeLevel>(newVol);
  return desiredVolume;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::OnVolumeChange()
{
  _dVars.lastVolumeChangeNotification_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVolume::ExternalNotificationPending() const
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return ( _dVars.lastVolumeChangeNotification_s + kVolumeChangeNotificationAgeLimit_s ) > currentTime_s;
}

}
}
