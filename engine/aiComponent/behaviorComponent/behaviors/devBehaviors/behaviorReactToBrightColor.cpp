/**
 * File: BehaviorReactToBrightColor.cpp
 *
 * Author: Patrick Doran
 * Created: 2018-10-16
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "behaviorReactToBrightColor.h"

#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/components/settingsManager.h"

#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::InstanceConfig::InstanceConfig(const Json::Value& config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::BehaviorReactToBrightColor(const Json::Value& config)
 : ICozmoBehavior(config), _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::~BehaviorReactToBrightColor()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBrightColor::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActivatableScope->insert({VisionMode::DetectingBrightColors, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // TODO: insert any behaviors this will delegate to into delegates.
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
#if 0
  const char* list[] = {
    // TODO: insert any possible root-level json keys that this class is expecting.
    // TODO: replace this method with a simple {} in the header if this class doesn't use the ctor's "config" argument.
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // TODO: the behavior is active now, time to do something!
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","PDORAN 1");

//  u32 color = GetBEI().GetSettingsManager().GetRobotSettingAsUInt(external_interface::RobotSetting::eye_color);
//  u32 color = 0xFF0000FF;
//  bool ignored = false;
//  GetBEI().GetSettingsManager().SetRobotSetting(external_interface::RobotSetting::eye_color, Json::Value(color), true, ignored);

//  DelegateIfInControl(_iConfig.InstanceConfig.)
//  DEBUG_SET_STATE(Completed);

//  GetBEI().GetSettingsManager().ClaimPendingSettingsUpdate( external_interface::RobotSetting::eye_color );
//  GetBEI().GetSettingsManager().SetRobotSetting(external_interface::RobotSetting::eye_color, Json::Value(color), true, ignored);
//
//  CompoundActionSequential* animationSequence = new CompoundActionSequential();
//  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetIn ), true );
//  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorSwitch ), true );
//  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetOut ), true );
//
//  DelegateIfInControl( animationSequence );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorDeactivated()
{
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorDeactivated","PDORAN 3");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
  }
  // TODO: delete this function if you don't need it
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.BehaviorUpdate","PDORAN 2");
}

}
}
