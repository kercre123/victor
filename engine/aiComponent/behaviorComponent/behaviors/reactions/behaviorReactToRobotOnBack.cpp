/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnBack.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnBack::BehaviorReactToRobotOnBack(const Json::Value& config)
  : ICozmoBehavior(config)
  , _offTreadsCondition(OffTreadsState::OnBack)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnBack::WantsToBeActivatedBehavior() const
{
  return _offTreadsCondition.AreConditionsMet(GetBEI());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::InitBehavior()
{
  _offTreadsCondition.Init(GetBEI());
  _offTreadsCondition.SetActive(GetBEI(), true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorActivated()
{
  FlipDownIfNeeded(); 
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::FlipDownIfNeeded()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnBack ) {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    // Check if cliff detected
    // If not, then calibrate head because we're not likely to be on back if no cliff detected.
    if (robotInfo.GetCliffSensorComponent().IsCliffDetectedStatusBitOn()) {
      AnimationTrigger anim = AnimationTrigger::ANTICIPATED_FlipDownFromBack;
      
      if(GetAIComp<AIWhiteboard>().HasHiccups())
      {
        anim = AnimationTrigger::DEPRECATED_HiccupRobotOnBack;
      }
    
      DelegateIfInControl(new TriggerAnimationAction(anim),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    } else {
      const auto cliffs = robotInfo.GetCliffSensorComponent().GetCliffDataRaw();
      LOG_EVENT("BehaviorReactToRobotOnBack.FlipDownIfNeeded.CalibratingHead", "%d %d %d %d", cliffs[0], cliffs[1], cliffs[2], cliffs[3]);
      DelegateIfInControl(new CalibrateMotorAction(true, false),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    }
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::DelayThenFlipDown()
{
  if( GetBEI().GetOffTreadsState() == OffTreadsState::OnBack ) {
    DelegateIfInControl(new WaitAction(kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorDeactivated()
{
}

} // namespace Cozmo
} // namespace Anki
