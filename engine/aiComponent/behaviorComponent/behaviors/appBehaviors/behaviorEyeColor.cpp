/***********************************************************************************************************************
 *
 *  BehaviorEyeColor
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 8/01/2018
 *
 **********************************************************************************************************************/

// #include "engine/aiComponent/behaviorComponent/behaviors/appBehaviors/behaviorEyeColor.h"
#include "behaviorEyeColor.h"

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
BehaviorEyeColor::InstanceConfig::InstanceConfig()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEyeColor::DynamicVariables::DynamicVariables()
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEyeColor::BehaviorEyeColor( const Json::Value& config ) :
  ICozmoBehavior( config )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::InitBehavior()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::GetAllDelegates( std::set<IBehavior*>& delegates ) const
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = true;
  modifiers.wantsToBeActivatedWhenOnCharger       = true;
  modifiers.wantsToBeActivatedWhenOffTreads       = true;
  modifiers.behaviorAlwaysDelegates               = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEyeColor::WantsToBeActivatedBehavior() const
{
  return GetBEI().GetSettingsManager().IsSettingsUpdateRequestPending( external_interface::RobotSetting::eye_color );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::OnBehaviorActivated()
{
  _dVars = DynamicVariables();

  GetBEI().GetSettingsManager().ClaimPendingSettingsUpdate( external_interface::RobotSetting::eye_color );

  CompoundActionSequential* animationSequence = new CompoundActionSequential();
  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetIn ), true );
  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorSwitch ), true );
  animationSequence->AddAction( new TriggerLiftSafeAnimationAction( AnimationTrigger::EyeColorGetOut ), true );

  DelegateIfInControl( animationSequence );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::OnBehaviorDeactivated()
{
  // if we still have the pending request, let's force apply the setting
  SettingsManager& settings = GetBEI().GetSettingsManager();
  if ( settings.IsSettingsUpdateRequestPending( external_interface::RobotSetting::eye_color ))
  {
    settings.ApplyPendingSettingsUpdate( external_interface::RobotSetting::eye_color, true );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEyeColor::BehaviorUpdate()
{

}

} // namespace Cozmo
} // namespace Anki
