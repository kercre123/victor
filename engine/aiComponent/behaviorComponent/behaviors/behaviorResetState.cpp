/**
 * File: behaviorResetState.cpp
 *
 * Author: ross
 * Created: 2018-08-12
 *
 * Description: Tries to reset things that may have caused behavior cycles
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/behaviorResetState.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/pathComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Vector {

namespace {
  const float kTimeWaitWhenOnTreads_s = 0.5f;
  const float kTimeWaitWhenOffTreads_s = 3.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorResetState::BehaviorResetState(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorResetState::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::OnBehaviorActivated()
{
  // freeze all motor functions
  GetBEI().GetRobotInfo().GetMoveComponent().StopAllMotors();
  
  // stop any planning or path following
  auto& pathComponent = GetBEI().GetRobotInfo().GetPathComponent();
  pathComponent.Abort();
  
  // delocalize robot. this also resets blockworld, unexpected movement, and the navmap
  ExternalInterface::ForceDelocalizeRobot delocMsg;
  GetBEI().GetRobotInfo().GetExternalInterface()->Broadcast( ExternalInterface::MessageGameToEngine{std::move(delocMsg)} );
  
  // don't do anything for a short while. If the robot thinks it's off treads, wait even longer,
  // in case it thrashing was causing it to not realize it's on treads
  const bool offTreads = (GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads);
  float timeToWait_s = offTreads ? kTimeWaitWhenOffTreads_s : kTimeWaitWhenOnTreads_s;
  auto* waitAction = new WaitAction{ timeToWait_s };
  DelegateIfInControl( waitAction, &BehaviorResetState::PutDownCubeIfNeeded );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::PutDownCubeIfNeeded()
{
  // put down cube
  const bool carrying = GetBEI().GetRobotInfo().IsCarryingObject();
  if( carrying ) {
    auto* putDownCube = new PlaceObjectOnGroundAction();
    DelegateIfInControl( putDownCube, [this](const ActionResult& res) {
      if( res == ActionResult::SUCCESS ) {
        ResetComponents();
      } else {
        // try just putting the lift down
        auto* moveLiftDown = new MoveLiftToHeightAction{ MoveLiftToHeightAction::Preset::LOW_DOCK };
        DelegateIfInControl( moveLiftDown, &BehaviorResetState::ResetComponents );
      }
    });
  } else {
    ResetComponents();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorResetState::ResetComponents()
{
  // ForceDelocalizeRobot already handled blockworld, unexpected movement, and the navmap because of
  // resulting calls to OnRobotDelocalized(). Now take care of other components:
  
  // In case the last known face was causing the robot to turn towards something...? Might not be necessary
  GetBEI().GetFaceWorldMutable().ClearAllFaces();
  
  // and then the behavior ends
}

}
}
