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
//#include "engine/actions/basicActions.h"
#include "util/logging/logging.h"
#include "engine/robot.h"
#include "engine/components/settingsManager.h"
#include "proto/external_interface/settings.pb.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/animActions.h"
//#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"



#define LOG_CHANNEL "BehaviorVolume"

namespace Anki {
namespace Vector {


namespace {
  const std::map<std::string, VolumeLevel> volumeLevelMap {//{"mute", VolumeLevel::MUTE}, // don't ever actually use "mute"
                                                            {"minimum", VolumeLevel::MIN},
                                                            {"min", VolumeLevel::MIN},
                                                            {"low", VolumeLevel::LOW},
                                                            {"medium", VolumeLevel::MED},
                                                            {"high", VolumeLevel::HIGH},
                                                            {"maximum", VolumeLevel::MAX},
                                                            {"max", VolumeLevel::MAX}};
  const std::map<VolumeLevel, AnimationTrigger> volumeLevelAnimMap {{VolumeLevel::MIN, AnimationTrigger::VolumeLevel1},
                                                                    {VolumeLevel::LOW, AnimationTrigger::VolumeLevel2},
                                                                    {VolumeLevel::MED, AnimationTrigger::VolumeLevel3},
                                                                    {VolumeLevel::HIGH, AnimationTrigger::VolumeLevel4},
                                                                    {VolumeLevel::MAX, AnimationTrigger::VolumeLevel5}};
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
  const bool volumeUpPending = false; // TODO: update when this is actually added
  const bool volumeDownPending = false; // TODO: update when this is actually added
  
  //LOG_WARNING("BehaviorVolume.WantsToBeActivatedBehavior.Result",
  //            "WantsToBeActivatedBehavior returning %s", volumeLevelPending || volumeUpPending || volumeDownPending ? "true" : "false");

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
void BehaviorVolume::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
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
  
  // to get the pending intent I basically have to do this check again anyway
  UserIntentPtr intentData = nullptr;
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  VolumeLevel desiredVolume;
  if(uic.IsUserIntentPending(USER_INTENT(imperative_volumelevel))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumelevel));
    try {
      desiredVolume = computeDesiredVolumeFromLevelIntent(intentData);
    } catch (std::out_of_range) {
      // warning already output in method
      return;
    }
    /*
  } else if(uic.IsUserIntentPending(USER_INTENT(imperative_volumeup))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumeup));
    desiredVolume = getDesiredVolumeFromUpIntent(intentData);
  } else if(uic.IsUserIntentPending(USER_INTENT(imperative_volumedown))){
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_volumedown));
    desiredVolume = getDesiredVolumeFromDownIntent(intentData);
    */
  } else {
    LOG_WARNING("BehaviorVolume.OnBehaviorActivated.NoRecognizedPendingIntent",
                "No recognized pending intent for volume.");
    return;
  }

  // set desired volume
  setVolume(desiredVolume);

  // read it again, make sure it's changed
  // TODO: remove this before PR. (useful for development)
  SettingsManager& settings = GetBEI().GetSettingsManager();
  uint32_t vol = settings.GetRobotSettingAsUInt(external_interface::RobotSetting::master_volume);
  LOG_WARNING("BehaviorVolume.OnBehaviorActivated.Volume",
              "Read volume from settings: %u", vol);

  // delegate to play an animation (sequence)
  AnimationTrigger animTrigger = volumeLevelAnimMap.at(desiredVolume);
  TriggerLiftSafeAnimationAction* animation = new TriggerLiftSafeAnimationAction(animTrigger);
  DelegateIfInControl(animation);
                     
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVolume::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  
  //LOG_WARNING("BehaviorVolume.BehaviorUpdate.Monitoring",
  //                   "Volume BehaviorUpdate ran.");
  
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
    
    //LOG_WARNING("BehaviorVolume.BehaviorUpdate.Active",
    //                 "Volume behavior is active.");
    
  }
  // TODO: delete this function if you don't need it
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorVolume::setVolume(VolumeLevel desiredVolumeEnum)
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
    LOG_WARNING("BehaviorVolume.SetVolume.Failure",
                "Failed to change volume.");
  }
  if(ignoredDueToNoChange) {
    LOG_INFO("BehaviorVolume.SetVolume.NoChange",
                "SettingsManager reported no change in volume setting.");
  }
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
VolumeLevel BehaviorVolume::computeDesiredVolumeFromLevelIntent(UserIntentPtr intentData)
{
  const std::string levelRequest = intentData->intent.Get_imperative_volumelevel().volume_level;
  VolumeLevel desiredVol;
  try {
    desiredVol = volumeLevelMap.at(levelRequest);
    LOG_WARNING("BehaviorVolume.computeDesiredVolumeFromLevelIntent.desiredVol",
                "mapped level request %s to desiredVol %u", levelRequest.c_str(), desiredVol);
  } catch (std::out_of_range) {
    LOG_WARNING("BehaviorVolume.computeDesiredVolumeFromLevelIntent.out_of_range",
                "unexpected user intent volume_level: %s", levelRequest.c_str());
    throw;
  }
  
  return desiredVol;
}


}
}
