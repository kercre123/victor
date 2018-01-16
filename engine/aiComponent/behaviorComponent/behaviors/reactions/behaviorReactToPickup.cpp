/**
 * File: behaviorReactToPickup.cpp
 *
 * Author: Lee
 * Created: 08/26/15
 *
 * Description: Behavior for immediately responding to being picked up.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPickup.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/events/ankiEvent.h"
#include "engine/faceWorld.h"
#include "engine/petWorld.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

CONSOLE_VAR(f32, kMinTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 3.0f);
CONSOLE_VAR(f32, kMaxTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 6.0f);
CONSOLE_VAR(f32, kRepeatAnimMultIncrease, "Behavior.ReactToPickup", 0.33f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPickup::BehaviorReactToPickup(const Json::Value& config)
: ICozmoBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPickup::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _repeatAnimatingMultiplier = 1;
  
  // Delay introduced since cozmo can be marked as "In air" in robot.cpp while we
  // wait for the cliffDetect sensor to confirm he's on the ground
  const f32 bufferDelay_s = .5f;
  const f32 wait_s = CLIFF_EVENT_DELAY_MS/1000 + bufferDelay_s;
  DelegateIfInControl(new WaitAction(wait_s), &BehaviorReactToPickup::StartAnim);
  
}
 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::StartAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  // If we're carrying anything, the animation will likely cause us to throw it, so unattach
  // and set as unknown pose
  if(robotInfo.GetCarryingComponent().IsCarryingObject())
  {
    const bool clearCarriedObjects = true; // to mark as unknown, not just dirty
    robotInfo.GetCarryingComponent().SetCarriedObjectAsUnattached(clearCarriedObjects);
  }
  
  const bool isHardSpark = false;
  
  // If we're seeing a human or pet face, react to that, otherwise, react to being picked up
  const TimeStamp_t lastImageTimeStamp = robotInfo.GetLastImageTimeStamp();
  const TimeStamp_t kObsFaceTimeWindow_ms = 500;
  const TimeStamp_t obsTimeCutoff = (lastImageTimeStamp > kObsFaceTimeWindow_ms ? lastImageTimeStamp - kObsFaceTimeWindow_ms : 0);

  auto faceIDsObserved = behaviorExternalInterface.GetFaceWorld().GetFaceIDsObservedSince(obsTimeCutoff);
  
  auto const kTracksToLock = Util::EnumToUnderlying(AnimTrackFlag::BODY_TRACK);
  if(!faceIDsObserved.empty() && !isHardSpark)
  {
    // Get name of first named face, if there is one
    std::string name;
    for(auto faceID : faceIDsObserved)
    {
      const Vision::TrackedFace* face = behaviorExternalInterface.GetFaceWorld().GetFace(faceID);
      if(face != nullptr && face->HasName())
      {
        name = face->GetName();
        break;
      }
    }
    
    // Say the name if we have it, otherwise just acknowledge the (unknown) face
    // TODO: Set triggers in Json config?
    if(name.empty())
    {
      // Just react to unnamed face, but without using treads
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::AcknowledgeFaceUnnamed, 1, true, kTracksToLock));
    }
    else
    {
      SayTextAction* sayText = new SayTextAction(name, SayTextIntent::Name_Normal);
      sayText->SetAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed, kTracksToLock);
      DelegateIfInControl(sayText);
    }
  }
  else
  {
    auto currentPets = behaviorExternalInterface.GetPetWorld().GetAllKnownPets();
    if(!currentPets.empty() && !isHardSpark)
    {
      AnimationTrigger animTrigger = AnimationTrigger::PetDetectionShort_Dog;
      if(Vision::PetType::Cat == currentPets.begin()->second.GetType())
      {
        animTrigger = AnimationTrigger::PetDetectionShort_Cat;
      }
      
      // React, but without using body track, since we're picked up
      DelegateIfInControl(new TriggerAnimationAction(animTrigger, 1, true, kTracksToLock));
    }
    else
    {
      AnimationTrigger anim = AnimationTrigger::ReactToPickup;
      
      if(behaviorExternalInterface.GetAIComponent().GetWhiteboard().HasHiccups())
      {
        anim = AnimationTrigger::HiccupRobotPickedUp;
      }
      DelegateIfInControl(new TriggerAnimationAction(anim));
    }
  }
  
  const double minTime = _repeatAnimatingMultiplier * kMinTimeBetweenPickupAnims_sec;
  const double maxTime = _repeatAnimatingMultiplier * kMaxTimeBetweenPickupAnims_sec;
  const float nextInterval = Util::numeric_cast<float>(GetRNG().RandDblInRange(minTime, maxTime));
  _nextRepeatAnimationTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + nextInterval;
  _repeatAnimatingMultiplier += kRepeatAnimMultIncrease;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::BehaviorUpdate(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!IsActivated()){
    return;
  }

  const bool isControlDelegated = IsControlDelegated();
  if( !isControlDelegated && behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::InAir ) {
    CancelSelf();
    return;
  }

  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  if( robotInfo.IsOnCharger() && !isControlDelegated ) {
    PRINT_NAMED_INFO("BehaviorReactToPickup.OnCharger", "Stopping behavior because we are on the charger");
    CancelSelf();
    return;
  }
  // If we are in control, it might be time to play another reaction
  if (!isControlDelegated)
  {
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (currentTime > _nextRepeatAnimationTime)
    {
      if (robotInfo.GetCliffSensorComponent().IsCliffDetected()) {
        StartAnim(behaviorExternalInterface);
      } else {
        const auto cliffs = robotInfo.GetCliffSensorComponent().GetCliffDataRaw();
        LOG_EVENT("BehaviorReactToPickup.CalibratingHead", "%d %d %d %d", cliffs[0], cliffs[1], cliffs[2], cliffs[3]);
        DelegateIfInControl(new CalibrateMotorAction(true, false));
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
