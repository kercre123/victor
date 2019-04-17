/**
 * File: BehaviorPutDownBlockAtPose.cpp
 *
 * Author: Sam Russell
 * Created: 2018-10-16
 *
 * Description: If holding a cube, take it to the defined pose (current pose if none is set) and set it down using the 
 *              prescribed animation trigger (AnimationTrigger::PutDownBlockPutDown if none is set)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPutDownBlockAtPose.h"

#include "engine/actions/animActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/carryingComponent.h"
#include "engine/actions/basicActions.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
                          _dVars.state = PutDownCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
const float kBackupVerifyHeadAngle_deg = -20.0f;
const float kBackupVerifyDist_mm = -30.0f;
const int kBackupVerifyNumFrames = 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// External Interface
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::SetDestinationPose(const Pose3d& pose)
{
  _dVars.persistent.destinationPose = pose;
  _dVars.persistent.customDestPending = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::SetPutDownAnimTrigger(const AnimationTrigger& animTrigger)
{
  _dVars.persistent.putDownAnimTrigger = animTrigger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Internal Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlockAtPose::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlockAtPose::DynamicVariables::DynamicVariables()
: state(PutDownCubeState::DriveToDestinationPose)
, persistent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlockAtPose::DynamicVariables::Persistent::Persistent()
: destinationPose()
, putDownAnimTrigger(AnimationTrigger::PutDownBlockPutDown)
, customDestPending(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlockAtPose::BehaviorPutDownBlockAtPose(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlockAtPose::~BehaviorPutDownBlockAtPose()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPutDownBlockAtPose::WantsToBeActivatedBehavior() const
{
  return GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject() || IsControlDelegated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.visionModesForActiveScope->insert({ VisionMode::Markers, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::OnBehaviorActivated() 
{
  // reset dynamic variables
  auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;

  TransitionToDriveToDestinationPose();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::OnBehaviorDeactivated()
{
  // Clear out the persistent settings, they should be reset before activation if needed
  _dVars.persistent = DynamicVariables::Persistent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::TransitionToDriveToDestinationPose()
{
  SET_STATE(DriveToDestinationPose);

  if(_dVars.persistent.customDestPending) {
    DriveToPoseAction* driveToPoseAction = new DriveToPoseAction(_dVars.persistent.destinationPose);
    DelegateIfInControl(driveToPoseAction, &BehaviorPutDownBlockAtPose::TransitionToPutDownBlock);
  }
  else {
    TransitionToPutDownBlock(); 
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::TransitionToPutDownBlock()
{
  SET_STATE(PutDownBlock);

  // Place the cube using the Animation 
  DelegateIfInControl(new TriggerAnimationAction(_dVars.persistent.putDownAnimTrigger),
    [this](ActionResult result) {
      const auto& robotInfo = GetBEI().GetRobotInfo();
      if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
        LOG_INFO("BehaviorPlaceCubeByCharger.ObjectStillHeld",
                 "Robot still thinks its carrying an object after the anim finished");
        TransitionToLookDownAtBlock();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlockAtPose::TransitionToLookDownAtBlock()
{
  SET_STATE(LookDownAtBlock)
  auto callback = [this]() {
    auto& robotInfo = GetBEI().GetRobotInfo();
    if(robotInfo.GetCarryingComponent().IsCarryingObject()) {
      // No matter what, even if we didn't see the object we were
      // putting down for some reason, mark the robot as not carrying
      // anything so we don't get stuck in a loop of trying to put
      // something down (i.e. assume the object is no longer on our lift)
      // TODO Carried over from BehaviorPutDownBlock: We should really be using 
      // some kind of PlaceOnGroundAction instead of raw animation 
      LOG_INFO("BehaviorPutDownBlock.LookDownAtBlock.DidNotSeeBlock",
               "Forcibly setting carried objects as unattached");
      robotInfo.GetCarryingComponent().SetCarriedObjectAsUnattached();
    }
  };

  if(GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()){
    DelegateIfInControl(CreateLookAfterPlaceAction(), callback);
  }
  else{
    callback();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorPutDownBlockAtPose::CreateLookAfterPlaceAction()
{
  CompoundActionSequential* action = new CompoundActionSequential();

  // glance down to see if we see the cube if we still think we are carrying
  CompoundActionParallel* parallel = 
    new CompoundActionParallel({new MoveHeadToAngleAction(DEG_TO_RAD(kBackupVerifyHeadAngle_deg)),
                                new DriveStraightAction(kBackupVerifyDist_mm)});

  action->AddAction(parallel);
  action->AddAction(new WaitForImagesAction(kBackupVerifyNumFrames, VisionMode::Markers));

  return action;
}

} // namespace Vector
} // namespace Anki
