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
#include "anki/cozmo/basestation/actions/visuallyVerifyActions.h"
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
const float kSleepingTimeout_s = 30.f;
const float kMovementScoreThreshold_stir = 5.f;
const float kMovementScoreMax = 50.f;
  
} // end anonymous namespace
  
  
BehaviorGuardDog::BehaviorGuardDog(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _connectedCubesOnlyFilter(new BlockWorldFilter)
{
  SetDefaultName("GuardDog");
  
  // subscribe to some E2G messages:
  SubscribeToTags({EngineToGameTag::ObjectAccel,
                   EngineToGameTag::ObjectMoved,
                   EngineToGameTag::ObjectConnectionState});
  
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
  //   3. The blocks are gathered together closely enough (within a specified radius or bounding box)
  
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  // Ensure there are three located and connected blocks:
  if (locatedBlocks.size() != 3) {
    return false;
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
  _cubesData.clear();
  _firstSleepingStartTime_s = 0.f;
  _nCubesMoved = 0;
  _monitoringCubeMotion = false;
  _cubePoseToVerify = Pose3d();

  // Grab cube starting locations (ideally would have done this in IsRunnable(), but IsRunnable() is const)
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.InitInternal.WrongNumLocatedBlocks");
  
  // Initialize stuff for each block:
  for (const auto& block : locatedBlocks) {
    _cubesData.emplace_back(block->GetID());
  }
  
  SET_STATE(Init);
  
  return Result::RESULT_OK;
}
  
  
BehaviorGuardDog::Status BehaviorGuardDog::UpdateInternal(Robot& robot)
{
  // Monitor for cube motion if appropriate:
  if (_monitoringCubeMotion) {
    for (auto& block : _cubesData) {
      // If any cube has been moved above the crazy high threshold,
      //  then need to wake up right away.
      if (block.movementScore >= kMovementScoreMax) {
        StopActing();
        SET_STATE(Busted);
      } else if (!block.hasBeenMoved && block.movementScore > kMovementScoreThreshold_stir) {
        block.hasBeenMoved = true;
        ++_nCubesMoved;
        if (_nCubesMoved < 3) {
          StopActing();
          SET_STATE(Fakeout);
        } else {
          StopActing();
          SET_STATE(Busted);
        }
      }
      
      // Play the appropriate light cube animation (depending on cube movement):
      if (block.movementScore > kMovementScoreThreshold_stir) {
        if (block.lastCubeAnimTrigger != CubeAnimationTrigger::GuardDogBeingMoved) {
          StartLightCubeAnim(robot, block.objectId, CubeAnimationTrigger::GuardDogBeingMoved);
        }
      } else if (block.lastCubeAnimTrigger == CubeAnimationTrigger::GuardDogBeingMoved) {
        StartLightCubeAnim(robot, block.objectId, CubeAnimationTrigger::GuardDogOff);
      }
    }
  }
  
  // Monitor for timeout if we are currently 'sleeping':
  const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((_state == State::Sleeping) &&
      (now - _firstSleepingStartTime_s > kSleepingTimeout_s)) {
    StopActing();
    StartMonitoringCubeMotion(robot, false);
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
      // Stop pulsing light cube animation and go to "lights off":
      StartLightCubeAnims(robot, CubeAnimationTrigger::GuardDogOff);
      
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
                                     SET_STATE(StartSleeping);
                                     _firstSleepingStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                                     StartMonitoringCubeMotion(robot);
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
      // TODO: React differently based on how many cubes have sensed movement at this point?
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogTimeout), [this]() { SET_STATE(DriveToCheckCubes); });
      break;
    }
    case State::DriveToCheckCubes:
    {
      // Drive to a pose from which we can verify if there are any cubes remaining.
      
      // Ask blockworld for the closest cube:
      const ObservableObject* closestBlock = robot.GetBlockWorld().FindLocatedObjectClosestTo(robot.GetPose(), *_connectedCubesOnlyFilter);
      if (!ANKI_VERIFY(closestBlock, "BehaviorGuardDog.UpdateInternal.NoClosestBlock", "No closest block returned by blockworld!")) {
        SET_STATE(Complete);
        break;
      }
      _cubePoseToVerify = closestBlock->GetPose();
      
      auto turnToPoseAction = new TurnTowardsPoseAction(robot, _cubePoseToVerify, Radians(M_PI_F));
      auto driveBackwardAction = new DriveStraightAction(robot, -120.f, 150.f);
      
      auto action = new CompoundActionSequential(robot, {turnToPoseAction, driveBackwardAction});
      
      StartActing(action, [this]() { SET_STATE(VisuallyCheckCubes); });
      break;
    }
    case State::VisuallyCheckCubes:
    {
      // See if there are any blocks remaining:
      const Point3f threshold_mm {100.f, 100.f, 100.f};
      StartActing(new VisuallyVerifyNoObjectAtPoseAction(robot, _cubePoseToVerify, threshold_mm),
                  [this] (ActionResult res)
                  {
                    const bool objSeen = (res == ActionResult::VISUAL_OBSERVATION_FAILED);
                    if (objSeen) {
                      SET_STATE(BlocksRemaining);
                    } else {
                      SET_STATE(BlocksMissing);
                    }
                  });
      break;
    }
    case State::BlocksRemaining:
    {
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogCubeRemaining), [this]() { SET_STATE(Complete); });
      break;
    }
    case State::BlocksMissing:
    {
      // TODO: Need an animation for this state: this is just a placeholder:
      StartActing(new TriggerAnimationAction(robot, AnimationTrigger::GuardDogBusted), [this]() { SET_STATE(Complete); });
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
    default:
      PRINT_NAMED_WARNING("BehaviorGuardDog.HandleWhileRunning",
                          "Received an unhandled E2G event: %s",
                          MessageEngineToGameTagToString(event.GetData().GetTag()));
      break;
  }
}


void BehaviorGuardDog::HandleObjectAccel(const Robot& robot, const ObjectAccel& msg)
{
  // Retrieve a reference to the objectData struct corresponding to this message's object ID:
  const ObjectID objId = msg.objectID;
  auto it = std::find_if(_cubesData.begin(), _cubesData.end(), [&objId](sCubeData objData) { return objData.objectId == objId; });
  if (it == _cubesData.end()) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.HandleObjectAccel.UnknownObject",
                "Received an ObjectAccel message from an unknown object (objectId = %d)",
                msg.objectID);
    return;
  }
  
  sCubeData& block = *it;
  
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
  
  // Cap the movement score:
  block.movementScore = std::min(kMovementScoreMax, block.movementScore);
  
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
  // TODO: Figure out what to do if an object is moved
  //   before Cozmo starts 'guarding' his blocks
  
  if (!_monitoringCubeMotion) {
    PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectMoved.ObjectMoved",
                     "Received ObjectMoved message for Object with ID %d even though we're not currently monitoring for movement!",
                     msg.objectID);
  }
}

  
void BehaviorGuardDog::ComputeStartingPose(const Robot& robot,  Pose3d& startingPose)
{
  // TODO: Note - there is an open ticket to make this initial pose computation
  //  'smarter' (COZMO-10699)
  
  startingPose.SetParent(robot.GetWorldOrigin());
  
  // Create a polygon of the block centers
  Poly2f blocksPoly;
  std::vector<const ObservableObject*> locatedBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.InitInternal.WrongNumLocatedBlocks");

  for (const auto& block : locatedBlocks) {
    const float x = block->GetPose().GetTranslation().x();
    const float y = block->GetPose().GetTranslation().y();
    blocksPoly.push_back(Point2f(x, y));
  }
  Pose2d blockCentroid(Radians(0.f), blocksPoly.ComputeCentroid());
  const float largestBoundBoxEdge_mm = std::max(blocksPoly.GetMaxX() - blocksPoly.GetMinX(),
                                                blocksPoly.GetMaxY() - blocksPoly.GetMinY());
  
  // Compute a pose for the robot to start the behavior at (ideally facing a person):
  Pose3d lastObservedFace;
  const bool haveLastObservedFace = (0 != robot.GetFaceWorld().GetLastObservedFace(lastObservedFace, false));
  if (haveLastObservedFace)
  {
    // Then there is a last observed face.
    Pose2d face2d(lastObservedFace);
    Radians centroidToFaceAngle(atan2f(face2d.GetY() - blockCentroid.GetY(),
                                       face2d.GetX() - blockCentroid.GetX()));
    
    // Create a goal pose next to the blocks and facing the last known face.
    Pose2d goalPose = blockCentroid;
    goalPose.SetRotation(centroidToFaceAngle + M_PI_2_F);
    goalPose.TranslateBy(1.4f * largestBoundBoxEdge_mm);
    goalPose.SetRotation(centroidToFaceAngle);
    
    startingPose.SetTranslation(Vec3f(goalPose.GetX(), goalPose.GetY(), 0.f));
    startingPose.SetRotation(goalPose.GetAngle(), Z_AXIS_3D());
  } else {
    // No last observed face available. Just find the closest point between the robot and the cubes
    Pose2d robotPose(robot.GetPose());
    Radians centroidToRobotAngle(atan2f(robotPose.GetY() - blockCentroid.GetY(),
                                        robotPose.GetX() - blockCentroid.GetX()));
    
    // Create the goal pose for the robot to get to
    Pose2d goalPose = blockCentroid;
    goalPose.SetRotation(centroidToRobotAngle);
    goalPose.TranslateBy(1.4f * largestBoundBoxEdge_mm);
    goalPose.SetRotation(centroidToRobotAngle + M_PI_2_F);
    
    startingPose.SetTranslation(Vec3f(goalPose.GetX(), goalPose.GetY(), 0.f));
    startingPose.SetRotation(goalPose.GetAngle(), Z_AXIS_3D());
  }
  
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
  DEV_ASSERT(_cubesData.size() == 3, "BehaviorGuardDog.EnableCubeAccelStreaming.WrongNumCubesDataEntries");
  
  for (const auto& block : _cubesData) {
    const auto activeBlock = robot.GetBlockWorld().GetConnectedActiveObjectByID(block.objectId);
    if (ANKI_VERIFY(activeBlock,
                    "BehaviorGuardDog.EnableCubeAccelStreaming.NullActiveBlock",
                    "Null pointer returned from blockworld for block with ID %d",
                    block.objectId.GetValue())) {
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
    for (auto& block : _cubesData) {
      block.ResetAccelData();
    }
  }
  
  EnableCubeAccelStreaming(robot, enable);
  _monitoringCubeMotion = enable;
}
  
  
bool BehaviorGuardDog::StartLightCubeAnim(Robot& robot, const ObjectID& objId, const CubeAnimationTrigger& cubeAnimTrigger)
{
  // Retrieve a reference to the objectData struct corresponding to this message's object ID:
  auto it = std::find_if(_cubesData.begin(), _cubesData.end(), [&objId](sCubeData block) { return block.objectId == objId; });
  if (it == _cubesData.end()) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.StartLightCubeAnim.InvalidObjectId",
                "Could not find object with ID %d in cubesData! Trying to play %s",
                objId.GetValue(),
                EnumToString(cubeAnimTrigger));
    return false;
  }
  sCubeData& block = *it;
  
  PRINT_NAMED_INFO("BehaviorGuardDog.StartLightCubeAnim.StartingAnim",
                   "Object %d: Playing light cube animation %s (previous was %s)",
                   block.objectId.GetValue(),
                   EnumToString(cubeAnimTrigger),
                   EnumToString(block.lastCubeAnimTrigger));

  bool success = false;
  if (block.lastCubeAnimTrigger == CubeAnimationTrigger::Count) {
    // haven't played any light cube anim yet, just play the new one:
    success = robot.GetCubeLightComponent().PlayLightAnim(block.objectId, cubeAnimTrigger);
  } else {
    // we've already played a light cube anim - cancel it and play the new one:
    success = robot.GetCubeLightComponent().StopAndPlayLightAnim(block.objectId, block.lastCubeAnimTrigger, cubeAnimTrigger);
  }
    
  if (!success) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.StartLightCubeAnim.PlayAnimFailed",
                "Failed to play light cube anim trigger %s on object with ID %d",
                EnumToString(cubeAnimTrigger),
                block.objectId.GetValue());
    return false;
  }
  
  block.lastCubeAnimTrigger = cubeAnimTrigger;
  return success;
}
  
bool BehaviorGuardDog::StartLightCubeAnims(Robot& robot, const CubeAnimationTrigger& cubeAnimTrigger)
{
  // Should have all three cubes in the cubesData container if this function is being called:
  DEV_ASSERT(_cubesData.size() == 3, "BehaviorGuardDog.StartLightCubeAnims.WrongNumCubesDataEntries");
  
  for (auto& block : _cubesData) {
    if(!StartLightCubeAnim(robot, block.objectId, cubeAnimTrigger)) {
      return false;
    }
  }

  return true;
}
  

  
} // namespace Cozmo
} // namespace Anki
