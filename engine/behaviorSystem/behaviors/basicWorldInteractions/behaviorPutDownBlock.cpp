/**
 * File: behaviorPutDownBlock.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-23
 *
 * Description: Simple behavior which puts down a block (using an animation group)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/behaviorSystem/behaviors/basicWorldInteractions/behaviorPutDownBlock.h"

#include "engine/actions/animActions.h"
#include "engine/components/carryingComponent.h"
#include "engine/robot.h"
#include "engine/actions/basicActions.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace{
CONSOLE_VAR(f32, kBPDB_finalHeadAngle_deg,    "Behavior.PutDownBlock", -20.0f);
CONSOLE_VAR(f32, kBPDB_verifyBackupDist_mm,   "Behavior.PutDownBlock", -30.0f);
CONSOLE_VAR(f32, kBPDB_putDownBackupSpeed_mm, "Behavior.PutDownBlock", 100.f);
CONSOLE_VAR(f32, kBPDB_scoreIncreaseDuringPutDown,   "Behavior.PutDownBlock", 5.0);
CONSOLE_VAR(f32, kBPDB_scoreIncreasePostPutDown,     "Behavior.PutDownBlock", 5.0);
CONSOLE_VAR(f32, kBPDB_kBackupDistanceMin_mm,     "Behavior.PutDownBlock", -45.0);
CONSOLE_VAR(f32, kBPDB_kBackupDistanceMax_mm,     "Behavior.PutDownBlock", -75.0);

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPutDownBlockArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPutDownBlockArray),
              "Reaction triggers duplicate or non-sequential");
  
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPutDownBlock::BehaviorPutDownBlock(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPutDownBlock::IsRunnableInternal(const Robot& robot) const
{
  return robot.GetCarryingComponent().IsCarryingObject() || IsActing();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorPutDownBlock::InitInternal(Robot& robot)
{
  // Disable double tap so we don't try to turn to a tapped object while in the middle of putting the block
  // down
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersPutDownBlockArray);

  // Choose where to put the block down
  // TODO: Make this smarter and find a place away from other known objects
  // For now, just back up blindly and play animation.
  const float backupDistance = GetRNG().RandDblInRange(kBPDB_kBackupDistanceMin_mm, kBPDB_kBackupDistanceMax_mm);
  
  StartActingExtraScore(new CompoundActionSequential(robot, {
                new DriveStraightAction(robot, backupDistance, kBPDB_putDownBackupSpeed_mm),
                new TriggerAnimationAction(robot, AnimationTrigger::PutDownBlockPutDown),
              }),
              kBPDB_scoreIncreaseDuringPutDown,
              &BehaviorPutDownBlock::LookDownAtBlock);
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPutDownBlock::LookDownAtBlock(Robot& robot)
{
  StartActingExtraScore(CreateLookAfterPlaceAction(robot, true), kBPDB_scoreIncreasePostPutDown,
              [&robot]() {
                if(robot.GetCarryingComponent().IsCarryingObject()) {
                  // No matter what, even if we didn't see the object we were
                  // putting down for some reason, mark the robot as not carrying
                  // anything so we don't get stuck in a loop of trying to put
                  // something down (i.e. assume the object is no longer on our lift)
                  // TODO: We should really be using some kind of PlaceOnGroundAction instead of raw animation (see COZMO-2192)
                  PRINT_NAMED_WARNING("BehaviorPutDownBlock.LookDownAtBlock.DidNotSeeBlock",
                                      "Forcibly setting carried objects as unattached (See COZMO-2192)");
                  robot.GetCarryingComponent().SetCarriedObjectAsUnattached();
                }
              });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorPutDownBlock::CreateLookAfterPlaceAction(Robot& robot, bool doLookAtFaceAfter)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);
  if( robot.GetCarryingComponent().IsCarryingObject() ) {
    // glance down to see if we see the cube if we still think we are carrying
    static const int kNumFrames = 2;
    
    CompoundActionParallel* parallel = new CompoundActionParallel(robot,
                                                                  {new MoveHeadToAngleAction(robot, DEG_TO_RAD(kBPDB_finalHeadAngle_deg)),
                                                                   new DriveStraightAction(robot, kBPDB_verifyBackupDist_mm)});
    action->AddAction(parallel);
    action->AddAction(new WaitForImagesAction(robot, kNumFrames, VisionMode::DetectingMarkers));
  }

  if(doLookAtFaceAfter)
  {
    // in any case, look back at the last face after this is done (to give them a chance to show another cube)
    const bool sayName = false;
    action->AddAction(new TurnTowardsFaceWrapperAction(robot,
                                                       new TriggerAnimationAction(robot, AnimationTrigger::PutDownBlockKeepAlive),
                                                       true, false, M_PI_F, sayName));
  }
  
  return action;
}

} // namespace Cozmo
} // namespace Anki

