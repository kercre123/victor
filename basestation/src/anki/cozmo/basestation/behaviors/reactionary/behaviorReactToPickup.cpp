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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPickup.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/sayTextAction.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

CONSOLE_VAR(f32, kMinTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 3.0f);
CONSOLE_VAR(f32, kMaxTimeBetweenPickupAnims_sec, "Behavior.ReactToPickup", 6.0f);
CONSOLE_VAR(f32, kRepeatAnimMultIncrease, "Behavior.ReactToPickup", 0.33f);

BehaviorReactToPickup::BehaviorReactToPickup(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToPickup");
}

  
bool BehaviorReactToPickup::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}
  

Result BehaviorReactToPickup::InitInternal(Robot& robot)
{
  _repeatAnimatingMultiplier = 1;
  
  // Delay introduced since cozmo can be marked as "In air" in robot.cpp while we
  // wait for the cliffDetect sensor to confirm he's on the ground
  const f32 bufferDelay_s = .5f;
  const f32 wait_s = CLIFF_EVENT_DELAY_MS/1000 + bufferDelay_s;
  StartActing(new WaitAction(robot, wait_s), &BehaviorReactToPickup::StartAnim);
  return Result::RESULT_OK;
}
  
void BehaviorReactToPickup::StartAnim(Robot& robot)
{
  // If we're carrying anything, the animation will likely cause us to throw it, so unattach
  // and set as unknown pose
  if(robot.IsCarryingObject())
  {
    const bool clearCarriedObjects = true; // to mark as unknown, not just dirty
    robot.SetCarriedObjectAsUnattached(clearCarriedObjects);
  }
  
  // Don't respond to people or pets during hard spark
  const BehaviorManager& behaviorMgr = robot.GetBehaviorManager();
  const bool isHardSpark = behaviorMgr.IsActiveSparkHard();
  
  // If we're seeing a human or pet face, react to that, otherwise, react to being picked up
  const TimeStamp_t lastImageTimeStamp = robot.GetLastImageTimeStamp();
  const TimeStamp_t kObsFaceTimeWindow_ms = 500;
  const TimeStamp_t obsTimeCutoff = (lastImageTimeStamp > kObsFaceTimeWindow_ms ? lastImageTimeStamp - kObsFaceTimeWindow_ms : 0);

  auto faceIDsObserved = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(obsTimeCutoff);
  
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
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToPickup));
    }
  }
  
  const double minTime = _repeatAnimatingMultiplier * kMinTimeBetweenPickupAnims_sec;
  const double maxTime = _repeatAnimatingMultiplier * kMaxTimeBetweenPickupAnims_sec;
  const double nextInterval = GetRNG().RandDblInRange(minTime, maxTime);
  _nextRepeatAnimationTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + nextInterval;
  _repeatAnimatingMultiplier += kRepeatAnimMultIncrease;
}
  
IBehavior::Status BehaviorReactToPickup::UpdateInternal(Robot& robot)
{
  const bool isActing = IsActing();
  if( !isActing && robot.GetOffTreadsState() != OffTreadsState::InAir ) {
    return Status::Complete;
  }

  if( robot.IsOnCharger() && !isActing ) {
    PRINT_NAMED_INFO("BehaviorReactToPickup.OnCharger", "Stopping behavior because we are on the charger");
    return Status::Complete;
  }
  // If we aren't acting, it might be time to play another reaction
  if (!isActing)
  {
    const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if (currentTime > _nextRepeatAnimationTime)
    {
      if (robot.GetCliffDataRaw() < CLIFF_SENSOR_DROP_LEVEL) {
        StartAnim(robot);
      } else {
        LOG_EVENT("BehaviorReactToPickup.CalibratingHead", "%d", robot.GetCliffDataRaw());
        StartActing(new CalibrateMotorAction(robot, true, false));
      }
    }
  }
  
  return Status::Running;
}
  
void BehaviorReactToPickup::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
