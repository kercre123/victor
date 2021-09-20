/**
 * File: BehaviorEyeColorVoiceCommand.cpp
 *
 * Author: Andrew Stout
 * Created: 2019-03-19
 *
 * Description: Voice command for changing Vector's eye color.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/eyeColor/behaviorEyeColorVoiceCommand.h"

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/settingsManager.h"
#include "proto/external_interface/settings.pb.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {
  const std::map<std::string, external_interface::EyeColor> kEyeColorMap {
      {"COLOR_TEAL", external_interface::EyeColor::TIP_OVER_TEAL},
      {"COLOR_ORANGE", external_interface::EyeColor::OVERFIT_ORANGE},
      {"COLOR_YELLOW", external_interface::EyeColor::UNCANNY_YELLOW},
      {"COLOR_GREEN", external_interface::EyeColor::NON_LINEAR_LIME},
      {"COLOR_BLUE", external_interface::EyeColor::SINGULARITY_SAPPHIRE},
      {"COLOR_PURPLE", external_interface::EyeColor::FALSE_POSITIVE_PURPLE}
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEyeColorVoiceCommand::BehaviorEyeColorVoiceCommand(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEyeColorVoiceCommand::~BehaviorEyeColorVoiceCommand()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEyeColorVoiceCommand::WantsToBeActivatedBehavior() const
{
  // check the different intents that should be handled here
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool eyecolorPending = uic.IsUserIntentPending(USER_INTENT(imperative_eyecolor));
  const bool eyecolorSpecificPending = uic.IsUserIntentPending(USER_INTENT(imperative_eyecolor_specific));

  return eyecolorPending || eyecolorSpecificPending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColorVoiceCommand::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColorVoiceCommand::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColorVoiceCommand::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColorVoiceCommand::OnBehaviorActivated() 
{
  // get the intent
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntentPtr intentData = nullptr;
  external_interface::EyeColor newEyeColor;
  if (uic.IsUserIntentPending(USER_INTENT(imperative_eyecolor_specific))){
    // in this case the user has requested a specific color
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_eyecolor_specific));
    // look at the entity to determine which specific color
    newEyeColor = GetDesiredColorFromIntent(intentData);
    // set the color through the settings manager
  } else if (uic.IsUserIntentPending(USER_INTENT(imperative_eyecolor))){
    // in this case the user has non-specifically requested a change--cycle or random
    intentData = SmartActivateUserIntent(USER_INTENT(imperative_eyecolor));
    // or choosing next in cycle or random
    newEyeColor = ChooseColorCyclic();
  } else {
    LOG_WARNING("BehaviorEyeColorVoiceCommand.OnBehaviorActivated.NoRecognizedPendingIntent",
        "No recognized pending intent.");
    return;
  }

  SetEyeColor(newEyeColor);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColorVoiceCommand::BehaviorUpdate() 
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
external_interface::EyeColor BehaviorEyeColorVoiceCommand::GetDesiredColorFromIntent(UserIntentPtr intentData)
{
  const std::string colorRequest = intentData->intent.Get_imperative_eyecolor_specific().eye_color;
  external_interface::EyeColor desiredEyeColor;
  const auto it = kEyeColorMap.find(colorRequest);
  if (it != kEyeColorMap.end()){
    desiredEyeColor = it->second;
    LOG_DEBUG("BehaviorEyeColorVoiceCommand.GetDesiredColorFromIntent",
              "mapped eye color request %s to desiredEyeColor %u", colorRequest.c_str(), desiredEyeColor);
  } else {
    // invalid. print a warning, return the current color.
    LOG_WARNING("BehaviorEyeColorVoiceCommand.GetDesiredColorFromIntent.InvalidColorRequest",
        "Unrecognized user intent eye color: %s", colorRequest.c_str());
    SettingsManager& settings = GetBEI().GetSettingsManager();
    desiredEyeColor = static_cast<external_interface::EyeColor>(
        settings.GetRobotSettingAsUInt(external_interface::RobotSetting::eye_color) );
  }
  return desiredEyeColor;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
external_interface::EyeColor BehaviorEyeColorVoiceCommand::ChooseColorCyclic()
{
  // get current eye color,   // increment it
  SettingsManager& settings = GetBEI().GetSettingsManager();
  external_interface::EyeColor newEyeColor = static_cast<external_interface::EyeColor>(
      settings.GetRobotSettingAsUInt(external_interface::RobotSetting::eye_color) + 1 );
  // roll over if needed
  if (newEyeColor >= external_interface::EyeColor_MAX) {
    newEyeColor = external_interface::EyeColor_MIN;
  }
  return newEyeColor;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEyeColorVoiceCommand::SetEyeColor(external_interface::EyeColor desiredEyeColor)
{
  SettingsManager& settings = GetBEI().GetSettingsManager();
  bool ignoredDueToNoChange;
  const bool success = settings.SetRobotEyeColorSetting(desiredEyeColor, true, ignoredDueToNoChange);
  if (success) {
    LOG_INFO("BehaviorEyeColorVoiceCommand.SetEyeColor.Success", "Successfully changed eye color to %u", desiredEyeColor);
  } else {
    LOG_INFO("BehaviorEyeColorVoiceCommand.SetEyeColor.Failure", "Failed to change eye color.");
  }
  if (ignoredDueToNoChange) {
    LOG_INFO("BehaviorEyeColorVoiceCommand.SetEyeColor.NoChange",
        "SettingsManager reported no change in eye color setting.");
    // play the eye color change animation anyway
    // (usually it's triggered in the eyeColor reaction behavior, but in this case it won't be)
    CompoundActionSequential* animationSequence = new CompoundActionSequential();
    animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetIn ), true );
    animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorSwitch ), true );
    animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetOut ), true );

    DelegateIfInControl( animationSequence );
  }
  return success;
}



} // namespace Vector
} // namespace Anki
