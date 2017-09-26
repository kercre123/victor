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

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cliffSensorComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/events/ankiEvent.h"
#include "engine/faceWorld.h"
#include "engine/petWorld.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

CONSOLE_VAR(f32, kMinTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 3.0f);
CONSOLE_VAR(f32, kMaxTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 6.0f);
CONSOLE_VAR(f32, kRepeatAnimMultIncrease, "Behavior.ReactToPickup", 0.33f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPickup::BehaviorReactToPickup(const Json::Value& config)
: IBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPickup::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToPickup::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _repeatAnimatingMultiplier = 1;
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Delay introduced since cozmo can be marked as "In air" in robot.cpp while we
  // wait for the cliffDetect sensor to confirm he's on the ground
  const f32 bufferDelay_s = .5f;
  const f32 wait_s = CLIFF_EVENT_DELAY_MS/1000 + bufferDelay_s;
  StartActing(new WaitAction(robot, wait_s), &BehaviorReactToPickup::StartAnim);
  return Result::RESULT_OK;
}
 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::StartAnim(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // If we're carrying anything, the animation will likely cause us to throw it, so unattach
  // and set as unknown pose
  if(robot.GetCarryingComponent().IsCarryingObject())
  {
    const bool clearCarriedObjects = true; // to mark as unknown, not just dirty
    robot.GetCarryingComponent().SetCarriedObjectAsUnattached(clearCarriedObjects);
  }
  
  // Don't respond to people or pets during hard spark
  const BehaviorManager& behaviorMgr = robot.GetBehaviorManager();
  const bool isHardSpark = behaviorMgr.IsActiveSparkHard();
  
  // If we're seeing a human or pet face, react to that, otherwise, react to being picked up
  const TimeStamp_t lastImageTimeStamp = robot.GetLastImageTimeStamp();
  const TimeStamp_t kObsFaceTimeWindow_ms = 500;
  const TimeStamp_t obsTimeCutoff = (lastImageTimeStamp > kObsFaceTimeWindow_ms ? lastImageTimeStamp - kObsFaceTimeWindow_ms : 0);

  auto faceIDsObserved = robot.GetFaceWorld().GetFaceIDsObservedSince(obsTimeCutoff);
  
  auto const kTracksToLock = Util::EnumToUnderlying(AnimTrackFlag::BODY_TRACK);
  if(!faceIDsObserved.empty() && !isHardSpark)
  {
    // Get name of first named face, if there is one
    std::string name;
    for(auto faceID : faceIDsObserved)
    {
      const Vision::TrackedFace* face = robot.GetFaceWorld().GetFace(faceID);
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
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::AcknowledgeFaceUnnamed, 1, true, kTracksToLock));
    }
    else
    {
      SayTextAction* sayText = new SayTextAction(robot, name, SayTextIntent::Name_Normal);
      sayText->SetAnimationTrigger(AnimationTrigger::AcknowledgeFaceNamed, kTracksToLock);
      StartActing(sayText);
    }
  }
  else
  {
    auto currentPets = robot.GetPetWorld().GetAllKnownPets();
    if(!currentPets.empty() && !isHardSpark)
    {
      AnimationTrigger animTrigger = AnimationTrigger::PetDetectionShort_Dog;
      if(Vision::PetType::Cat == currentPets.begin()->second.GetType())
      {
        animTrigger = AnimationTrigger::PetDetectionShort_Cat;
      }
      
      // React, but without using body track, since we're picked up
      StartActing(new TriggerAnimationAction(robot, animTrigger, 1, true, kTracksToLock));
    }
    else
    {
      AnimationTrigger anim = AnimationTrigger::ReactToPickup;
      
      if(behaviorExternalInterface.GetAIComponent().GetWhiteboard().HasHiccups())
      {
        anim = AnimationTrigger::HiccupRobotPickedUp;
      }
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      StartActing(new TriggerAnimationAction(robot, anim));
    }
  }
  
  const double minTime = _repeatAnimatingMultiplier * kMinTimeBetweenPickupAnims_sec;
  const double maxTime = _repeatAnimatingMultiplier * kMaxTimeBetweenPickupAnims_sec;
  const float nextInterval = Util::numeric_cast<float>(GetRNG().RandDblInRange(minTime, maxTime));
  _nextRepeatAnimationTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + nextInterval;
  _repeatAnimatingMultiplier += kRepeatAnimMultIncrease;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorReactToPickup::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  const bool isActing = IsControlDelegated();
  if( !isActing && behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::InAir ) {
    return Status::Complete;
  }
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  if( robot.IsOnCharger() && !isActing ) {
    PRINT_NAMED_INFO("BehaviorReactToPickup.OnCharger", "Stopping behavior because we are on the charger");
    return Status::Complete;
  }
  // If we aren't acting, it might be time to play another reaction
  if (!isActing)
  {
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (currentTime > _nextRepeatAnimationTime)
    {
      const auto cliffDataRaw = robot.GetCliffSensorComponent().GetCliffDataRaw();
      if (cliffDataRaw < CLIFF_SENSOR_DROP_LEVEL) {
        StartAnim(behaviorExternalInterface);
      } else {
        LOG_EVENT("BehaviorReactToPickup.CalibratingHead", "%d", cliffDataRaw);
        StartActing(new CalibrateMotorAction(robot, true, false));
      }
    }
  }
  
  return Status::Running;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPickup::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
