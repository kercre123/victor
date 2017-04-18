/**
 * File: behaviorGuardDog.cpp
 *
 * Author: Matt Michini
 * Created: 2017-03-27
 *
 * Description: Cozmo acts as a sleepy guard dog, pretending to nap but still making sure no one tries to steal his cubes.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorGuardDog.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"

#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "clad/types/behaviorTypes.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _state = State::s; DEBUG_SET_STATE(s); } while(0)
  
namespace {
  
// Reaction triggers to disable:
constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersGuardDogArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::PyramidInitialDetection,      true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           false}
};
  
static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersGuardDogArray),
              "Reaction triggers duplicate or non-sequential");
  
// Constants/thresholds:
CONSOLE_VAR_RANGED(float, kSleepingTimeout_s, "Behavior.GuardDog", 60.f, 20.f, 300.f);
CONSOLE_VAR_RANGED(float, kMovementScoreDetectionThreshold, "Behavior.GuardDog",  8.f,  5.f,  30.f);
CONSOLE_VAR_RANGED(float, kMovementScoreMax, "Behavior.GuardDog", 50.f, 30.f, 200.f);
  
} // end anonymous namespace
  
  
BehaviorGuardDog::BehaviorGuardDog(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _connectedCubesOnlyFilter(new BlockWorldFilter)
{
  SetDefaultName("GuardDog");
  
  // subscribe to some E2G messages:
  SubscribeToTags({EngineToGameTag::ObjectAccel,
                   EngineToGameTag::ObjectConnectionState,
                   EngineToGameTag::ObjectMoved,
                   EngineToGameTag::ObjectUpAxisChanged
                   });
  
  // Set up the "connected light cubes only" blockworld filter:
  _connectedCubesOnlyFilter->AddAllowedFamily(ObjectFamily::LightCube);
  _connectedCubesOnlyFilter->AddFilterFcn([](const ObservableObject* obj) { return obj->GetActiveID() >= 0; });
}
  
bool BehaviorGuardDog::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();
  
  // Is this feature enabled?
  if (!robot.GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::GuardDog)) {
    return false;
  }
  
  // This behavior is runnable only if the following are all true:
  //   1. We know the locations 3 blocks
  //   2. We are connected to those 3 blocks
  //   3. The blocks are all upright
  //   4. The blocks are gathered together closely enough (within a specified radius or bounding box)
  
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  // Ensure there are three located and connected blocks:
  if (locatedBlocks.size() != 3) {
    return false;
  }
  
  // Ensure that the blocks are upright:
  for (const auto& block : locatedBlocks) {
    if (block->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS) {
      return false;
    }
  }
  
  // Now, ensure that they are grouped somewhat closely together.
  // TODO: This should probably be done with the BlockConfigurationManager,
  //  with a method for example BlockCfgMgr.GetCircumscribingRadius() or something.
  
  // Create a polygon of the block centers
  Poly2f blocksPoly;
  for (const auto& block : locatedBlocks) {
    const float x = block->GetPose().GetTranslation().x();
    const float y = block->GetPose().GetTranslation().y();
    blocksPoly.push_back(Point2f(x, y));
  }

  const float kMaxBoundingBoxEdge_mm = 100.f;
  const float largestBoundBoxEdge_mm = std::max(blocksPoly.GetMaxX() - blocksPoly.GetMinX(),
                                                blocksPoly.GetMaxY() - blocksPoly.GetMinY());
  
  return (largestBoundBoxEdge_mm < kMaxBoundingBoxEdge_mm);
}

  
Result BehaviorGuardDog::InitInternal(Robot& robot)
{
  // Disable reactionary behaviors that we don't want interrupting this:
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersGuardDogArray);
  
  // Disable idle animation (TODO: needed?)
  //robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::Count);
  
  // Reset some members in case this is running again:
  _cubesDataMap.clear();
  _firstSleepingStartTime_s = 0.f;
  _nCubesMoved = 0;
  _nCubesFlipped = 0;
  _monitoringCubeMotion = false;

  // Grab cube starting locations (ideally would have done this in IsRunnable(), but IsRunnable() is const)
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.InitInternal.WrongNumLocatedBlocks");
  
  // Initialize the map of objectId to cubeData struct:
  for (const auto& block : locatedBlocks) {
    _cubesDataMap.emplace(block->GetID(), sCubeData());
  }
  
  SET_STATE(Init);
  
  return Result::RESULT_OK;
}
  
  
BehaviorGuardDog::Status BehaviorGuardDog::UpdateInternal(Robot& robot)
{
  // Check to see if all cubes were flipped or moved:
  if (_monitoringCubeMotion) {
    if (_nCubesFlipped == 3) {
      StopActing();
      SET_STATE(PlayerSuccess);
    } else if (_nCubesMoved == 3) {
      StopActing();
      SET_STATE(Busted);
    }
  }
  
  // Monitor for timeout if we are currently 'sleeping':
  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((_state == State::Sleeping) &&
      (now - _firstSleepingStartTime_s > kSleepingTimeout_s)) {
    StopActing();
    SET_STATE(Timeout);
  }
  
  // Only run the state machine if we're not acting:
  if (IsActing()) {
    return Status::Running;
  }
  
  switch (_state) {
    case State::Init:
    {
      // Play the 'soothing' pulse animation
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogPulse), [this]() { SET_STATE(DriveToBlocks); });
      
      // Set cube lights to pulse along with animation
      StartLightCubeAnims(robot, CubeAnimationTrigger::GuardDogPulse);
      
      break;
    }
    case State::DriveToBlocks:
    {
      // Compute a goal starting pose near the blocks and drive there.
      Pose3d startingPose;
      ComputeStartingPose(robot, startingPose);
      auto driveToStartingPoseAction = new DriveToPoseAction(robot, startingPose);
      
      // Wrap this in a retry action in case it fails.
      const u8 kNumRetries = 2;
      auto retryAction = new RetryWrapperAction(robot, driveToStartingPoseAction, AnimationTrigger::Count, kNumRetries);
      StartActing(retryAction, [this](ActionResult res)
                               {
                                 if (res != ActionResult::SUCCESS) {
                                   PRINT_NAMED_WARNING("BehaviorGuardDog.UpdateInternal.DriveToStartingPoseFailed",
                                                       "Failed to drive to starting pose (result = %s)! Continuing behavior from current pose.",
                                                       EnumToString(res));
                                 }
                                 SET_STATE(SettleIn);
                               });
      break;
    }
    case State::SettleIn:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogSettle),
                  [this, &robot]() {
                                     // Stop pulsing light cube animation and go to "lights off":
                                     StartLightCubeAnims(robot, CubeAnimationTrigger::GuardDogOff);
                                     StartMonitoringCubeMotion(robot);
                                     _firstSleepingStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                                     SET_STATE(StartSleeping);
                                   });
      break;
    }
    case State::StartSleeping:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogSleepLoop, 0));
      SET_STATE(Sleeping);
      break;
    }
      
    case State::Sleeping:
    {
      // Nothing to do here (waiting for block movement
      //  or timeout to move us out of this state)
      break;
    }
    case State::Fakeout:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogFakeout), [this]() { SET_STATE(StartSleeping); });
      break;
    }
    case State::Busted:
    {
      StartLightCubeAnims(robot, CubeAnimationTrigger::GuardDogBusted);
      StartMonitoringCubeMotion(robot, false);
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogBusted), [this]() { SET_STATE(Complete); });
      break;
    }
    case State::BlockDisconnected:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogCubeDisconnect), [this]() { SET_STATE(Complete); });
      break;
    }
    case State::Timeout:
    {
      StartMonitoringCubeMotion(robot, false);
      auto action = new CompoundActionSequential(robot);
      
      // Timeout wake-up animation:
      action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogTimeout));

      // Ask blockworld for the closest cube and turn toward it.
      // Note: We ignore failures for the TurnTowardsPose and DriveStraight actions, since these are
      //  purely for aesthetics and we do not want failures to prevent the animations afterward from
      //  being played.
      const ObservableObject* closestBlock = robot.GetBlockWorld().FindLocatedObjectClosestTo(robot.GetPose(), *_connectedCubesOnlyFilter);
      if (ANKI_VERIFY(closestBlock, "BehaviorGuardDog.UpdateInternal.Timeout.NoClosestBlock", "No closest block returned by blockworld!")) {
        action->AddAction(new TurnTowardsPoseAction(robot, closestBlock->GetPose()), true);
      }

      action->AddAction(new DriveStraightAction(robot, -80.f, 150.f), true); // ignore failures (see note above)
      
      if ((_nCubesMoved == 0) && (_nCubesFlipped == 0)) {
        action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogTimeoutCubesUntouched));
      } else {
        action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogTimeoutCubesTouched));
      }
      
      StartActing(action, [this]() { SET_STATE(Complete); });
      break;
    }
    case State::PlayerSuccess:
    {
      StartMonitoringCubeMotion(robot, false);
      auto action = new CompoundActionSequential(robot);
      
      // Small delay here so that reaction to all 3 cubes being flipped doesn't
      //  trigger too quickly (triggering the animations right away felt weird)
      const float timeToWait_s = 2.0f;
      action->AddAction(new WaitAction(robot, timeToWait_s));
      
      // Ask blockworld for the closest cube and turn toward it.
      // Note: We ignore failures for the TurnTowardsPose and DriveStraight actions, since these are
      //  purely for aesthetics and we do not want failures to prevent the animations afterward from
      //  being played.
      const ObservableObject* closestBlock = robot.GetBlockWorld().FindLocatedObjectClosestTo(robot.GetPose(), *_connectedCubesOnlyFilter);
      if (ANKI_VERIFY(closestBlock, "BehaviorGuardDog.UpdateInternal.PlayerSuccess.NoClosestBlock", "No closest block returned by blockworld!")) {
        action->AddAction(new TurnTowardsPoseAction(robot, closestBlock->GetPose()), true);
      }

      action->AddAction(new DriveStraightAction(robot, -80.f, 150.f), true); // ignore failures (see note above)
      action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogPlayerSuccess));
      
      StartActing(action, [this]() { SET_STATE(Complete); });
      break;
    }
    case State::Complete:
    {
      return Status::Complete;
    }
  }
  
  return Status::Running;
}


void BehaviorGuardDog::StopInternal(Robot& robot)
{
  // Stop streaming accelerometer data from each block:
  EnableCubeAccelStreaming(robot, false);
  
  // Stop light cube animations:
  robot.GetCubeLightComponent().StopAllAnims();
  
  // TODO: Record relevant DAS events here.
  //   - how many cubes were moved?
  //   - when was each cube initially moved?
  //   - what was the max magnitude for each cube?
  //   - how many 'bad' messages did we get from the cubes?
  //   - ...
}
  
  
void BehaviorGuardDog::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  // Handle messages:
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::ObjectAccel:
      HandleObjectAccel(robot, event.GetData().Get_ObjectAccel());
      break;
    case EngineToGameTag::ObjectConnectionState:
      HandleObjectConnectionState(robot, event.GetData().Get_ObjectConnectionState());
      break;
    case EngineToGameTag::ObjectMoved:
      HandleObjectMoved(robot, event.GetData().Get_ObjectMoved());
      break;
    case EngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(robot, event.GetData().Get_ObjectUpAxisChanged());
      break;
    default:
      PRINT_NAMED_WARNING("BehaviorGuardDog.HandleWhileRunning",
                          "Received an unhandled E2G event: %s",
                          MessageEngineToGameTagToString(event.GetData().GetTag()));
      break;
  }
}


void BehaviorGuardDog::HandleObjectAccel(Robot& robot, const ObjectAccel& msg)
{
  // Ignore if we're not monitoring for cube motion right now (sometimes we send the message to stop
  //  ObjectAccel streaming, but still receive some stray packets from the robot in the brief time afterward)
  if (!_monitoringCubeMotion) {
    return;
  }
  
  // Retrieve a reference to the cubeData struct corresponding to this message's object ID:
  const ObjectID objId = msg.objectID; // convert raw u32 to ObjectID
  auto it = _cubesDataMap.find(objId);
  if (it == _cubesDataMap.end()) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.HandleObjectAccel.UnknownObject",
                "Received an ObjectAccel message from an unknown object (objectId = %d)",
                msg.objectID);
    return;
  }
  sCubeData& block = it->second;
  
  // No need to continue monitoring cube accel if the cube
  //  has been successfully flipped over, so just bail out:
  if (block.hasBeenFlipped) {
    return;
  }
  
  ++block.msgReceivedCnt;
  
  // compute magnitude of accelerometer readings:
  const float aMag = std::sqrt(msg.accel.x * msg.accel.x +
                               msg.accel.y * msg.accel.y +
                               msg.accel.z * msg.accel.z);
  
  // Sometimes the accel data is blank (i.e. 0.0f) or extremely high, either
  //  of which would eff up the highpass filter
  const float maxValidAccelMag = 100.f;
  if (!FLT_NEAR(aMag, 0.f) && aMag < maxValidAccelMag) {
    block.prevAccelMag = block.accelMag;
    block.accelMag = aMag;
    
    if (!block.filtInitialized) {
      // Initialize prev to current to avoid spikes when starting.
      block.prevAccelMag = block.accelMag;
      block.filtInitialized = true;
    }
    
    const float kFilt = 0.8;
    block.hpFiltAccelMag = kFilt * (block.accelMag - block.prevAccelMag + block.hpFiltAccelMag);
  } else {
    PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectAccel.BadMessage",
                     "Received a bad ObjectAccel message from objId = %d, accel = {%.1f, %.1f, %1.f}, timestamp = %d",
                     objId.GetValue(),
                     msg.accel.x, msg.accel.y, msg.accel.z,
                     msg.timestamp);
    ++block.badMsgCnt;
  }
  
  block.maxFiltAccelMag = std::max(block.maxFiltAccelMag, std::abs(block.hpFiltAccelMag));
  
  // Update the 'movement score' indicating how much movement is detected (limit amount to add in case of large spikes):
  const float maxAmountToAddToMovementScore = 5.0f;
  block.movementScore += std::min(maxAmountToAddToMovementScore, std::abs(block.hpFiltAccelMag));

  // Decay the movement score:
  const float amountToDecay = 0.5f;
  if (block.movementScore > amountToDecay) {
    block.movementScore -= amountToDecay;
  } else {
    block.movementScore = 0.f;
  }
  
  // Set behavior states based on cube movement and play the
  //  appropriate light cube anim if appropriate:
  if (block.movementScore > kMovementScoreMax) {
    StopActing();
    SET_STATE(Busted);
  } else if (block.movementScore > kMovementScoreDetectionThreshold) {
    if (!block.hasBeenMoved) {
      block.hasBeenMoved = true;
      ++_nCubesMoved;
      StopActing();
      SET_STATE(Fakeout);
    }
    // Play the "being moved" light cube anim:
    if (block.lastCubeAnimTrigger != CubeAnimationTrigger::GuardDogBeingMoved) {
      StartLightCubeAnim(robot, objId, CubeAnimationTrigger::GuardDogBeingMoved);
    }
  } else if (block.lastCubeAnimTrigger != CubeAnimationTrigger::GuardDogSleeping) {
      StartLightCubeAnim(robot, objId, CubeAnimationTrigger::GuardDogSleeping);
  }
  
//  PRINT_NAMED_WARNING("objAccel", "objId = %d, accelMag = %+8.1f, hpFiltAccelMag = %+8.1f, movementScore = %8.1f",
//                      objId.GetValue(),
//                      block.accelMag,
//                      block.hpFiltAccelMag,
//                      block.movementScore);
}
  
  
  
void BehaviorGuardDog::HandleObjectConnectionState(const Robot& robot, const ObjectConnectionState& msg)
{
  if (!msg.connected) {
    PRINT_NAMED_WARNING("BehaviorGuardDog.HandleObjectConnectionState",
                        "Object with ID %d and type %s has disconnected",
                        msg.objectID,
                        EnumToString(msg.object_type));
    StopActing();
    SET_STATE(BlockDisconnected);
  }
}
  
  
void BehaviorGuardDog::HandleObjectMoved(const Robot& robot, const ObjectMoved& msg)
{
  // If an object has been moved before we start monitoring for motion
  //  and before we fall asleep, jump right to 'Busted' reaction.
  if (!_monitoringCubeMotion &&
      (_firstSleepingStartTime_s == 0.f) &&
      (_state != State::Busted)) {
    PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectMoved.UnexpectedBlockMovement",
                     "Received ObjectMoved message for Object with ID %d even though we're not currently monitoring for movement and not yet sleeping!",
                     msg.objectID);
    StopActing();
    SET_STATE(Busted);
  }
}

void BehaviorGuardDog::HandleObjectUpAxisChanged(Robot& robot, const ObjectUpAxisChanged& msg)
{
  // If an object has a new UpAxis, it must have been moved. If this happens before
  //  monitoring for motion and before we fall asleep, jump right to 'Busted' reaction.
  if (!_monitoringCubeMotion) {
    if ((_firstSleepingStartTime_s == 0.f) &&
        (_state != State::Busted)) {
      PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectUpAxisChanged.UnexpectedBlockMovement",
                       "Received ObjectUpAxisChanged message for Object with ID %d even though we're not currently monitoring for movement and not yet sleeping!",
                       msg.objectID);
      StopActing();
      SET_STATE(Busted);
    }
    return;
  }
  
  // Retrieve a reference to the objectData struct corresponding to this message's object ID:
  const ObjectID objId = msg.objectID;
  auto it = _cubesDataMap.find(objId);
  if (ANKI_VERIFY(it != _cubesDataMap.end(),
                  "BehaviorGuardDog.HandleObjectUpAxisChanged.UnknownObject",
                  "Received an UpAxisChanged message from an unknown object (objectId = %d)",
                  msg.objectID)) {
    auto& block = it->second;
    block.upAxis = msg.upAxis;
    
    // Check if the block has been flipped over successfully:
    if (!block.hasBeenFlipped &&
        (block.upAxis == UpAxis::ZNegative)) {
      block.hasBeenFlipped = true;
      ++_nCubesFlipped;
      StartLightCubeAnim(robot, objId, CubeAnimationTrigger::GuardDogSuccessfullyFlipped);
    }
  }
}

  
void BehaviorGuardDog::ComputeStartingPose(const Robot& robot,  Pose3d& startingPose)
{
  // The robot's starting pose should be as follows:
  //   - If there is a last observed face, the starting pose should be next to the cubes
  //      such that the robot is facing toward the observed face. The final configuration
  //      should then look like this (A = face, B = goal robot pose, C = centroid of cubes):
  //
  //         (A) face
  //          |
  //          |
  //          |
  //          |  goal robot pose (facing toward face)
  //          | /
  //          |/
  //         (B)---(C) centroid of cubes
  //
  //   - If there is no last observed face, the robot should simply drive to the closest
  //      point next to the cubes, but turned 90 degrees away from the cubes (so he's not
  //      looking at the cubes but rather snuggled next to them).
  //
  // Achieve this by initializing the goal pose at the centroid of the 3 blocks and moving
  //   it outward in the desired direction until it no longer intersects the blocks.
  
  // Create a polygon of the block centers
  Poly2f blocksPoly;
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.ComputeStartingPose.WrongNumLocatedBlocks");

  for (const auto& block : locatedBlocks) {
    const float x = block->GetPose().GetTranslation().x();
    const float y = block->GetPose().GetTranslation().y();
    blocksPoly.push_back(Point2f(x, y));
  }
  Pose2d blockCentroid(Radians(0.f), blocksPoly.ComputeCentroid());

  Pose2d robotPose(robot.GetPose());
  Radians centroidToRobotAngle(atan2f(robotPose.GetY() - blockCentroid.GetY(),
                                      robotPose.GetX() - blockCentroid.GetX()));
  
  Pose3d lastObservedFace;
  const bool haveLastObservedFace = (0 != robot.GetFaceWorld().GetLastObservedFace(lastObservedFace, false));
  Radians centroidToFaceAngle(atan2f(lastObservedFace.GetTranslation().y() - blockCentroid.GetY(),
                                     lastObservedFace.GetTranslation().x() - blockCentroid.GetX()));
  
  // Create a 2D goal pose (initialize to block centroid pose)
  Pose2d goalPose = blockCentroid;
  
  if (haveLastObservedFace)
  {
    // Initially rotate the pose to be perpendicular to the line connecting
    //  the block centroid to the observed face (in a direction that will
    //  result in the shortest path from the robot's current pose)
    if ((centroidToRobotAngle - centroidToFaceAngle).ToFloat() > 0.f) {
      goalPose.SetRotation(centroidToFaceAngle + M_PI_2_F);
    } else {
      goalPose.SetRotation(centroidToFaceAngle - M_PI_2_F);
    }
  } else {
    // Initially rotate the pose to be parallel with the line connecting
    //  the block centroid to the current robot position.
    goalPose.SetRotation(centroidToRobotAngle);
  }
  
  // Incrementally move the goal pose outward from the block centroid
  //  until it no longer intersects any blocks.
  const float stepIncrement_mm = 10.f;
  float totalTranslation_mm = 0.f;
  const float maxTranslation_mm = 250.f;
  std::vector<const ObservableObject*> intersectingObjects;
  do {
    goalPose.TranslateBy(stepIncrement_mm);
    
    // Stop condition just in case:
    totalTranslation_mm += stepIncrement_mm;
    if (!ANKI_VERIFY(totalTranslation_mm < maxTranslation_mm,
                     "BehaviorGuardDog.ComputeStartingPose.TranslatedTooFar",
                     "Candidate starting pose has been translated way too far (%.1f mm, max is %.1f mm)",
                     totalTranslation_mm,
                     maxTranslation_mm)) {
      break;
    }
    
    const auto candidatePoseBoundingQuad = robot.GetBoundingQuadXY(Pose3d(goalPose), 0.f);
    intersectingObjects.clear();
    
    robot.GetBlockWorld().FindLocatedIntersectingObjects(candidatePoseBoundingQuad,
                                                         intersectingObjects,
                                                         0.0f,
                                                         *_connectedCubesOnlyFilter);
  } while(!intersectingObjects.empty());
    
  if (haveLastObservedFace) {
    // Rotate the pose back toward the face.
    goalPose.SetRotation(centroidToFaceAngle);
  } else {
    // Rotate the pose to be perpendicular to the centroidToRobot line.
    goalPose.SetRotation(centroidToRobotAngle + M_PI_2_F);
  }
  
  // Convert to full 3D pose in robot world origin:
  startingPose.SetParent(robot.GetWorldOrigin());
  startingPose.SetTranslation(Vec3f(goalPose.GetX(), goalPose.GetY(), 0.f));
  startingPose.SetRotation(goalPose.GetAngle(), Z_AXIS_3D());
  
  PRINT_NAMED_INFO("BehaviorGuardDog.ComputeStartingPose.Pose",
                   "Starting pose computed %s last observed face: (%.1f, %.1f, %.1f : %.2f deg)",
                   haveLastObservedFace ? "using" : "without using",
                   startingPose.GetTranslation().x(),
                   startingPose.GetTranslation().y(),
                   startingPose.GetTranslation().z(),
                   startingPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
}
  
Result BehaviorGuardDog::EnableCubeAccelStreaming(const Robot& robot, const bool enable) const
{
  // Should have all three cubes in the cubesData container if this function is being called:
  DEV_ASSERT(_cubesDataMap.size() == 3, "BehaviorGuardDog.EnableCubeAccelStreaming.WrongNumCubesDataEntries");
  
  for (const auto& mapEntry : _cubesDataMap) {
    const ObjectID objId = mapEntry.first;
    const auto activeBlock = robot.GetBlockWorld().GetConnectedActiveObjectByID(objId);
    if (ANKI_VERIFY(activeBlock,
                    "BehaviorGuardDog.EnableCubeAccelStreaming.NullActiveBlock",
                    "Null pointer returned from blockworld for block with ID %d",
                    objId.GetValue())) {
      const Result res = robot.SendMessage(RobotInterface::EngineToRobot(StreamObjectAccel(activeBlock->GetActiveID(), enable)));
      if (!ANKI_VERIFY(res == Result::RESULT_OK,
                       "BehaviorGuardDog.EnableCubeAccelStreaming.SendMsgFailed",
                       "Failed to send StreamObjectAccel message (result = 0x%0X) to robot for cube with activeID = %d",
                       res,
                       activeBlock->GetActiveID())) {
        return res;
      }
    }
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorGuardDog::StartMonitoringCubeMotion(Robot& robot, const bool enable)
{
  if (enable == _monitoringCubeMotion) {
    return;
  }
 
  PRINT_NAMED_INFO("BehaviorGuardDog.StartMonitoringCubeMotion",
                   "%s monitoring of cube accelerometer data.",
                   enable ? "Enabling" : "Disabling");
  
  // Clear out stale data for each cube:
  if (enable) {
    for (auto& mapEntry : _cubesDataMap) {
      mapEntry.second.ResetAccelData();
    }
  }
  
  EnableCubeAccelStreaming(robot, enable);
  _monitoringCubeMotion = enable;
}
  
  
bool BehaviorGuardDog::StartLightCubeAnim(Robot& robot, const ObjectID& objId, const CubeAnimationTrigger& cubeAnimTrigger)
{
  // Retrieve a reference to the objectData struct corresponding to this message's object ID:
  auto it = _cubesDataMap.find(objId);
  if (it == _cubesDataMap.end()) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.StartLightCubeAnim.InvalidObjectId",
                "Could not find object with ID %d in cubesData! Trying to play %s",
                objId.GetValue(),
                EnumToString(cubeAnimTrigger));
    return false;
  }
  sCubeData& block = it->second;
  
  PRINT_NAMED_INFO("BehaviorGuardDog.StartLightCubeAnim.StartingAnim",
                   "Object %d: Playing light cube animation %s (previous was %s)",
                   objId.GetValue(),
                   EnumToString(cubeAnimTrigger),
                   EnumToString(block.lastCubeAnimTrigger));

  bool success = false;
  if (block.lastCubeAnimTrigger == CubeAnimationTrigger::Count) {
    // haven't played any light cube anim yet, just play the new one:
    success = robot.GetCubeLightComponent().PlayLightAnim(objId, cubeAnimTrigger);
  } else {
    // we've already played a light cube anim - cancel it and play the new one:
    success = robot.GetCubeLightComponent().StopAndPlayLightAnim(objId, block.lastCubeAnimTrigger, cubeAnimTrigger);
  }
    
  if (!success) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.StartLightCubeAnim.PlayAnimFailed",
                "Failed to play light cube anim trigger %s on object with ID %d",
                EnumToString(cubeAnimTrigger),
                objId.GetValue());
    return false;
  }
  
  block.lastCubeAnimTrigger = cubeAnimTrigger;
  return success;
}
  
bool BehaviorGuardDog::StartLightCubeAnims(Robot& robot, const CubeAnimationTrigger& cubeAnimTrigger)
{
  // Should have all three cubes in the cubesData container if this function is being called:
  DEV_ASSERT(_cubesDataMap.size() == 3, "BehaviorGuardDog.StartLightCubeAnims.WrongNumCubesDataEntries");
  
  for (const auto& mapEntry : _cubesDataMap) {
    if(!StartLightCubeAnim(robot, mapEntry.first, cubeAnimTrigger)) {
      return false;
    }
  }

  return true;
}
  

  
} // namespace Cozmo
} // namespace Anki
