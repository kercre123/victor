/**
 * File: behaviorDriveOffCharger.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to the edge off a charger and deal with the firmware cliff stop
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/charger.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/moodSystem/moodManager.h"

#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* const kExtraDriveDistKey = "extraDistanceToDrive_mm";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::InstanceConfig::InstanceConfig()
{
  distToDrive_mm = 0.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::DynamicVariables::DynamicVariables()
{
  pushedIdleAnimation = false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveOffCharger::BehaviorDriveOffCharger(const Json::Value& config)
: ICozmoBehavior(config)
{
  float extraDist_mm = config.get(kExtraDriveDistKey, 0.0f).asFloat();
  _iConfig.distToDrive_mm = Charger::GetLength() + extraDist_mm;
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorDriveOffCharger.DriveDist",
                 "Driving %fmm off the charger (%f length + %f extra)",
                 _iConfig.distToDrive_mm,
                 Charger::GetLength(),
                 extraDist_mm);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kExtraDriveDistKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveOffCharger::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // assumes it's not possible to be OnCharger without being OnChargerPlatform
  DEV_ASSERT(robotInfo.IsOnChargerPlatform() || !robotInfo.IsOnChargerContacts(),
             "BehaviorDriveOffCharger.WantsToBeActivatedBehavior.InconsistentChargerFlags");

  // can run any time we are on the charger platform
  
  // bn: this used to be "on charger platform", but that caused weird issues when the robot thought it drove
  // over the platform later due to mis-localization. Then this changed to "on charger" to avoid those issues,
  // but that caused other issues (if the robot was bumped during wakeup, it wouldn't drive off the
  // charger). Now, we've gone back to OnChargerPlatofrm but fixed it to work better (see the comments in
  // robot.h)
  const bool onChargerPlatform = robotInfo.IsOnChargerPlatform();
  return onChargerPlatform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::OnBehaviorActivated()
{
  
  _dVars.pushedIdleAnimation = false;
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().PushDrivingAnimations(
          {AnimationTrigger::DriveStartLaunch,
          AnimationTrigger::DriveLoopLaunch,
          AnimationTrigger::DriveEndLaunch},
          GetDebugLabel());
  _dVars.pushedIdleAnimation = true;

  const bool onTreads = GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads;
  if( onTreads ) {
    TransitionToDrivingForward();
  }
  else {
    // otherwise we'll just wait in Update until we are on the treads (or removed from the charger)
    DEBUG_SET_STATE(WaitForOnTreads);
  }
  
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::OnBehaviorDeactivated()
{
  if(_dVars.pushedIdleAnimation){
    auto& robotInfo = GetBEI().GetRobotInfo();
    robotInfo.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());
  }
}
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  if( robotInfo.IsOnChargerPlatform() ) {
    const bool onTreads = GetBEI().GetOffTreadsState() == OffTreadsState::OnTreads;
    if( !onTreads ) {
      // if we aren't on the treads anymore, but we are on the charger, then the user must be holding us
      // inside the charger.... so just sit in this behavior doing nothing until we get put back down
      CancelDelegates(false);
      DEBUG_SET_STATE(WaitForOnTreads);
      // TODO:(bn) play an idle here?
    }
    else if( ! IsControlDelegated() ) {
      // if we finished the last action, but are still on the charger, queue another one
      TransitionToDrivingForward();
    }
    
    return;
  }

  if( !IsControlDelegated() ) {
    // store in whiteboard our success
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    GetAIComp<AIWhiteboard>().GotOffChargerAtTime( curTime );
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveOffCharger::TransitionToDrivingForward()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  DEBUG_SET_STATE(DrivingForward);
  if( robotInfo.IsOnChargerPlatform() )
  {
    // probably interrupted by getting off the charger platform
    DriveStraightAction* action = new DriveStraightAction(_iConfig.distToDrive_mm);
    DelegateIfInControl(action,[this](ActionResult res){
      if(res == ActionResult::SUCCESS){
        BehaviorObjectiveAchieved(BehaviorObjective::DroveAsIntended);
        
        if(GetBEI().HasMoodManager()){
          auto& moodManager = GetBEI().GetMoodManager();
          moodManager.TriggerEmotionEvent("DriveOffCharger",
                                          MoodManager::GetCurrentTimeInSeconds());
        }
      }
    });
    // the Update function will transition back to this state (or out of the behavior) as appropriate
  }
}

} // namespace Cozmo
} // namespace Anki

