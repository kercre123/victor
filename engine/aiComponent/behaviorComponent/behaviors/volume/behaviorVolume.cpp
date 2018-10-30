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


#include "engine/aiComponent/behaviorComponent/behaviors/volume/behaviorVolume.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/settingsManager.h"
#include "engine/actions/animActions.h"
#include "engine/robot.h"
#include "util/logging/logging.h"
#include "proto/external_interface/settings.pb.h"


#define LOG_CHANNEL "BehaviorVolume"

namespace Anki {
namespace Vector {


namespace {
  const std::map<std::string, EVolumeLevel> volumeLevelMap {//{"mute", EVolumeLevel::MUTE}, // don't ever actually use "mute"
                                                            {"minimum", EVolumeLevel::MIN},
                                                            {"min", EVolumeLevel::MIN},
                                                            {"low", EVolumeLevel::LOW},
                                                            {"medium", EVolumeLevel::MED},
                                                            {"high", EVolumeLevel::HIGH},
                                                            {"maximum", EVolumeLevel::MAX},
                                                            {"max", EVolumeLevel::MAX}};
  const std::map<EVolumeLevel, AnimationTrigger> volumeLevelAnimMap {{EVolumeLevel::MIN, AnimationTrigger::VolumeLevel1},
                                                                    {EVolumeLevel::LOW, AnimationTrigger::VolumeLevel2},
                                                                    {EVolumeLevel::MED, AnimationTrigger::VolumeLevel3},
                                                                    {EVolumeLevel::HIGH, AnimationTrigger::VolumeLevel4},
                                                                    {EVolumeLevel::MAX, AnimationTrigger::VolumeLevel5}};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVolume::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVolume::DynamicVariables::DynamicVariables()
{
}

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
  
  return volumeLevelPending || volumeUpPending || volumeDownPending;
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
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // to get the identity of the pending intent I basically have to do this check again
  UserIntentPtr intentData = nullptr;
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  EVolumeLevel desiredVolume;
  if(uic.IsUserIntentPending(USER_INTENT(imperative_volumelevel))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumelevel));
    try {
      desiredVolume = ComputeDesiredVolumeFromLevelIntent(intentData);
    } catch (std::out_of_range) {
      // warning already issued in method
      return;
    }
  } else if(uic.IsUserIntentPending(USER_INTENT(imperative_volumeup))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumeup));
    desiredVolume = ComputeDesiredVolumeFromIncrement(true);
  } else if(uic.IsUserIntentPending(USER_INTENT(imperative_volumedown))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumedown));
    desiredVolume = ComputeDesiredVolumeFromIncrement(false);
  } else {
    LOG_WARNING("BehaviorVolume.OnBehaviorActivated.NoRecognizedPendingIntent",
                "No recognized pending intent for volume.");
    return;
  }

  // set desired volume
  SetVolume(desiredVolume);

  // delegate to play an animation (sequence)
  const AnimationTrigger animTrigger = volumeLevelAnimMap.at(desiredVolume);
  TriggerLiftSafeAnimationAction* animation = new TriggerLiftSafeAnimationAction(animTrigger);
  DelegateIfInControl(animation);
                     
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
  // cast the enum to uint32_t. Bonus! It's already a uint32_t, this is just to be really explicit
  uint32_t desiredVolume = static_cast<uint32_t>(desiredVolumeEnum);

  // Let's do some range checking here
  // should really be unnecessary now that we're using the enum, but let's be safe
  if (desiredVolume < 1 || desiredVolume > 5) {
    LOG_WARNING("BehaviorVolume.SetVolume.OutsidePermittedRange",
                "Requested volume %u outside permitted range of [1,5].", desiredVolume);
    return false;
  }

  // check whether there's actually a change, if not, don't bother setting?
  // Not bothering for now--SettingsManager handles that gracefully

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
EVolumeLevel BehaviorVolume::ComputeDesiredVolumeFromLevelIntent(UserIntentPtr intentData)
{
  const std::string levelRequest = intentData->intent.Get_imperative_volumelevel().volume_level;
  EVolumeLevel desiredVol;
  try {
    desiredVol = volumeLevelMap.at(levelRequest);
    LOG_DEBUG("BehaviorVolume.ComputeDesiredVolumeFromLevelIntent.desiredVol",
                "mapped level request %s to desiredVol %u", levelRequest.c_str(), desiredVol);
  } catch (std::out_of_range) {
    LOG_WARNING("BehaviorVolume.ComputeDesiredVolumeFromLevelIntent.out_of_range",
                "unexpected user intent volume_level: %s", levelRequest.c_str());
    throw;
  }
  
  return desiredVol;
}

EVolumeLevel BehaviorVolume::ComputeDesiredVolumeFromIncrement(bool positiveIncrement) {
  // read volume from settings
  SettingsManager& settings = GetBEI().GetSettingsManager();
  uint32_t vol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);

  // apply increment
  int increment = positiveIncrement ? 1 : -1;
  uint32_t newVol = vol + increment;
  // check bounds
  if (newVol < 1) {
    newVol = 1;
    LOG_INFO("BehaviorVolume.ComputeDesiredVolumeFromIncrement.out_of_range",
              "volume is already at minimum level");
  }
  if (newVol > 5) {
    newVol = 5;
    LOG_INFO("BehaviorVolume.ComputeDesiredVolumeFromIncrement.out_of_range",
              "volume is already at maximum level");
  }

  // cast to EVolumeLevel. The enum is already a uint32_t, this is just to be really explicit
  EVolumeLevel desiredVolume = static_cast<EVolumeLevel>(newVol);
  return desiredVolume;
}


}
}
