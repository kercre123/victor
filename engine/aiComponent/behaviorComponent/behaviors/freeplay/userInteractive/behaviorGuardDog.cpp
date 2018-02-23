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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorGuardDog.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/retryWrapperAction.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeAccelComponent.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

// Transition to a new state and update the debug name for logging/debugging
#define SET_STATE(s) do{ _state = State::s; DEBUG_SET_STATE(s); } while(0)
  
namespace {
  
// Constants/thresholds:
CONSOLE_VAR_RANGED(float, kSleepingMinDuration_s, "Behavior.GuardDog",  60.f, 20.f, 120.f); // minimum time Cozmo will stay asleep before waking up
CONSOLE_VAR_RANGED(float, kSleepingMaxDuration_s, "Behavior.GuardDog", 120.f, 60.f, 300.f); // maximum time Cozmo will stay asleep before waking up
CONSOLE_VAR_RANGED(float, kSleepingTimeoutAfterCubeMotion_s, "Behavior.GuardDog", 20.f, 0.f, 60.f); // minimum time Cozmo will stay asleep after last cube motion was detected (if SleepingMaxDuration not exceeded)
CONSOLE_VAR_RANGED(float, kMovementScoreDetectionThreshold, "Behavior.GuardDog",  40.f,  5.f,  80.f);
CONSOLE_VAR_RANGED(float, kMovementScoreMax, "Behavior.GuardDog", 80.f, 30.f, 200.f);
CONSOLE_VAR_RANGED(float, kMaxMovementBustedGracePeriod_s, "Behavior.GuardDog", 10.f, 0.f, 120.f); // amount of time to wait (after falling asleep) before 'Busted' can happen due to movement above 'max' threshold
  
// Constants for the CubeAccelComponent MovementListener:
const float kHighPassFiltCoef = 0.4f;      // high-pass filter coeff
const float kMaxMovementScoreToAdd = 5.f;  // max movement score to add
const float kMovementScoreDecay = 0.5f;    // movement score decay
  
} // end anonymous namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGuardDog::BehaviorGuardDog(const Json::Value& config)
: ICozmoBehavior(config)
, _connectedCubesOnlyFilter(std::make_unique<BlockWorldFilter>())
{
  
  // subscribe to some E2G messages:
  SubscribeToTags({EngineToGameTag::ObjectConnectionState,
                   EngineToGameTag::ObjectMoved,
                   EngineToGameTag::ObjectUpAxisChanged
                   });
  
  // Set up the "connected light cubes only" blockworld filter:
  _connectedCubesOnlyFilter->AddAllowedFamily(ObjectFamily::LightCube);
  _connectedCubesOnlyFilter->AddFilterFcn([](const ObservableObject* obj) { return obj->GetActiveID() >= 0; });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGuardDog::WantsToBeActivatedBehavior() const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  // Is this feature enabled?
  if (!robotInfo.GetContext()->GetFeatureGate()->IsFeatureEnabled(Anki::Cozmo::FeatureType::GuardDog)) {
    return false;
  }
  
  // Don't run if we currently have the hiccups:
  if (GetBEI().GetAIComponent().GetWhiteboard().HasHiccups()) {
    return false;
  }
  
  // This behavior is activatable only if the following are all true:
  //   1. We know the locations 3 blocks
  //   2. We are connected to those 3 blocks
  //   3. The blocks are all upright
  //   4. The blocks are gathered together closely enough (within a specified radius or bounding box)
  
  std::vector<const ObservableObject*> locatedBlocks;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::OnBehaviorActivated()
{  
  // Reset some members in case this is running again:
  _cubesDataMap.clear();
  _firstSleepingStartTime_s = 0.f;
  _sleepingDuration_s = 0.f;
  _lastCubeMovementTime_s = 0.f;
  _nCubesMoved = 0;
  _nCubesFlipped = 0;
  _monitoringCubeMotion = false;
  _currPublicBehaviorStage = GuardDogStage::Count;
  _result = "None";

  // Grab cube starting locations (ideally would have done this in WantsToBeActivated(), but WantsToBeActivated() is const)
  std::vector<const ObservableObject*> locatedBlocks;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.InitInternal.WrongNumLocatedBlocks");
  
  // Initialize the map of objectId to cubeData struct:
  for (const auto& block : locatedBlocks) {
    _cubesDataMap.emplace(block->GetID(), sCubeData());
  }
  
  {
    // Stop light cube animations:
    GetBEI().GetCubeLightComponent().StopAllAnims();
  }
  
  SET_STATE(Init);
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  // Check to see if all cubes were flipped or moved:
  if (_monitoringCubeMotion) {
    if (_nCubesFlipped == 3) {
      CancelDelegates();
      SET_STATE(PlayerSuccess);
    } else if (_nCubesMoved == 3) {
      CancelDelegates();
      SET_STATE(Busted);
    }
  }
  
  // Monitor for timeout if we are currently 'sleeping',
  //  and check to see if music state should be updated.
  if (_state == State::Sleeping) {
    const float now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool pastMinDuration = now - _firstSleepingStartTime_s > kSleepingMinDuration_s;
    const bool pastMaxDuration = now - _firstSleepingStartTime_s > kSleepingMaxDuration_s;
    const bool pastCubeMotionTimeout = now - _lastCubeMovementTime_s > kSleepingTimeoutAfterCubeMotion_s;
    // Trigger the timeout if appropriate
    if (pastMaxDuration ||
       (pastMinDuration && pastCubeMotionTimeout)) {
      CancelDelegates();
      SET_STATE(Timeout);
    }
  }
  
  // Check if any blocks are being moved (in order to trigger the proper music)
  if (_state == State::Sleeping ||
      _state == State::Fakeout) {
    bool anyBlocksMoving = false;
    for (const auto& mapEntry : _cubesDataMap) {
      if (mapEntry.second.movementScore > kMovementScoreDetectionThreshold) {
        anyBlocksMoving = true;
      }
    }
    const auto newStage = anyBlocksMoving ? GuardDogStage::CubeBeingMoved : GuardDogStage::Sleeping;
    if (newStage != _currPublicBehaviorStage) {
      UpdatePublicBehaviorStage(newStage);
    }
  }
  
  // Only run the state machine if we're in control:
  if (IsControlDelegated()) {
    return;
  }
  
  switch (_state) {
    case State::Init:
    {
      // Play the 'soothing' pulse animation
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogPulse), [this]() { SET_STATE(DriveToBlocks); });
      
      // Set cube lights to 'setup'
      StartLightCubeAnims(CubeAnimationTrigger::GuardDogSetup);
      
      break;
    }
    case State::SetupInterrupted:
    {
      RecordResult("SetupInterrupted");
      CancelSelf();
      return;
    }
    case State::DriveToBlocks:
    {
      // Compute a goal starting pose near the blocks and drive there.
      Pose3d startingPose;
      ComputeStartingPose(startingPose);
      const bool kForceHeadDown = false;
      auto driveToStartingPoseAction = new DriveToPoseAction(startingPose, kForceHeadDown);
      
      // Wrap this in a retry action in case it fails.
      const u8 kNumRetries = 2;
      auto retryAction = new RetryWrapperAction(driveToStartingPoseAction, AnimationTrigger::Count, kNumRetries);
      DelegateIfInControl(retryAction, [this](ActionResult res)
                               {
                                 if (res != ActionResult::SUCCESS) {
                                   PRINT_NAMED_WARNING("BehaviorGuardDog.UpdateInternal_Legacy.DriveToStartingPoseFailed",
                                                       "Failed to drive to starting pose (result = %s)! Continuing behavior from current pose.",
                                                       EnumToString(res));
                                 }
                                 SET_STATE(SettleIn);
                               });
      break;
    }
    case State::SettleIn:
    {
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogSettle),
                  [this]() {
                                     // Stop setup light cube animation and go to "sleeping" lights:
                                     StartLightCubeAnims(CubeAnimationTrigger::GuardDogSleeping);
                                     StartMonitoringCubeMotion();
                                     _firstSleepingStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                    
                                     using namespace ExternalInterface;
                                     //robot.Broadcast( MessageEngineToGame( GuardDogStart() ) );
                                     SET_STATE(StartSleeping);
                                   });
      break;
    }
    case State::StartSleeping:
    {
      // Start the sleeping loop animation (override default timeout)
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogSleepLoop,
                                                     0,                                   // numLoops (0 = infinite)
                                                     true,                                // interruptRunning
                                                     (u8)AnimTrackFlag::NO_TRACKS,        // tracksToLock
                                                     kSleepingMaxDuration_s));            // timeout_sec
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
      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogFakeout), [this]() { SET_STATE(StartSleeping); });
      break;
    }
    case State::Busted:
    {
      RecordResult("Busted");
      StartLightCubeAnims(CubeAnimationTrigger::GuardDogBusted);
      StartMonitoringCubeMotion(false);

      using namespace ExternalInterface;
      //robot.Broadcast( MessageEngineToGame( GuardDogEnd(false) ) );

      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogBusted), [this]() { SET_STATE(Complete); });
      UpdatePublicBehaviorStage(GuardDogStage::Busted);
      break;
    }
    case State::BlockDisconnected:
    {
      RecordResult("BlockDisconnected");
      using namespace ExternalInterface;
      //robot.Broadcast( MessageEngineToGame( GuardDogEnd(false) ) );

      DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::GuardDogCubeDisconnect), [this]() { SET_STATE(Complete); });
      break;
    }
    case State::Timeout:
    {
      StartMonitoringCubeMotion(false);
      auto action = new CompoundActionSequential();
      
      // Timeout wake-up animation:
      action->AddAction(new TriggerAnimationAction(AnimationTrigger::GuardDogTimeout));

      // Ask blockworld for the closest cube and turn toward it.
      // Note: We ignore failures for the TurnTowardsPose and DriveStraight actions, since these are
      //  purely for aesthetics and we do not want failures to prevent the animations afterward from
      //  being played.

      auto& robotInfo = GetBEI().GetRobotInfo();
      const ObservableObject* closestBlock = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotInfo.GetPose(), *_connectedCubesOnlyFilter);
      if (ANKI_VERIFY(closestBlock, "BehaviorGuardDog.UpdateInternal_Legacy.Timeout.NoClosestBlock", "No closest block returned by blockworld!")) {
        action->AddAction(new TurnTowardsPoseAction(closestBlock->GetPose()), true);
      }

      action->AddAction(new DriveStraightAction(-80.f, 150.f), true); // ignore failures (see note above)
      
      // Play the CubesRemaining music and animation at the same time:
      auto updateStageCubesRemainingLambda = [this] (Robot& robot) { UpdatePublicBehaviorStage(GuardDogStage::CubesRemaining); return true; };
      action->AddAction(new WaitForLambdaAction(updateStageCubesRemainingLambda));
      
      if ((_nCubesMoved == 0) && (_nCubesFlipped == 0)) {
        RecordResult("TimeoutCubesUntouched");
        action->AddAction(new TriggerAnimationAction(AnimationTrigger::GuardDogTimeoutCubesUntouched));
      } else {
        RecordResult("TimeoutCubesTouched");
        action->AddAction(new TriggerAnimationAction(AnimationTrigger::GuardDogTimeoutCubesTouched));
      }
      
      using namespace ExternalInterface;
      //robot.Broadcast( MessageEngineToGame( GuardDogEnd(false) ) );

      DelegateIfInControl(action, [this]() { SET_STATE(Complete); });
      break;
    }
    case State::PlayerSuccess:
    {
      RecordResult("PlayerSuccess");
      
      StartMonitoringCubeMotion(false);
     
      // Switch to the 'sleeping' music (in case a cube was being moved
      //   when transitioned into this state, so that the 'tension' music
      //   doesn't keep playing during the following actions)
      UpdatePublicBehaviorStage(GuardDogStage::Sleeping);
      auto action = new CompoundActionSequential();
      
      // Timeout wake-up animation:
      action->AddAction(new TriggerAnimationAction(AnimationTrigger::GuardDogTimeout));
      
      // Ask blockworld for the closest cube and turn toward it.
      // Note: We ignore failures for the TurnTowardsPose and DriveStraight actions, since these are
      //  purely for aesthetics and we do not want failures to prevent the animations afterward from
      //  being played.
      auto& robotInfo = GetBEI().GetRobotInfo();
      const ObservableObject* closestBlock = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotInfo.GetPose(), *_connectedCubesOnlyFilter);
      if (ANKI_VERIFY(closestBlock, "BehaviorGuardDog.UpdateInternal_Legacy.PlayerSuccess.NoClosestBlock", "No closest block returned by blockworld!")) {
        action->AddAction(new TurnTowardsPoseAction(closestBlock->GetPose()), true);
      }

      action->AddAction(new DriveStraightAction(-80.f, 150.f), true); // ignore failures (see note above)
      
      // Update the music stage
      auto updateStageAllCubesGoneLambda = [this] (Robot& robot) { UpdatePublicBehaviorStage(GuardDogStage::AllCubesGone); return true; };
      action->AddAction(new WaitForLambdaAction(updateStageAllCubesGoneLambda));
      
      // Play the PlayerSuccess animation
      action->AddAction(new TriggerAnimationAction(AnimationTrigger::GuardDogPlayerSuccess));
      
      using namespace ExternalInterface;
      //robot.Broadcast( MessageEngineToGame( GuardDogEnd(true) ) );

      DelegateIfInControl(action, [this]() { SET_STATE(Complete); });
      break;
    }
    case State::Complete:
    {
      CancelSelf();
      return;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::OnBehaviorDeactivated()
{
  // Stop streaming accelerometer data from each block:
  StartMonitoringCubeMotion(false);
  
  // Stop light cube animations:
  GetBEI().GetCubeLightComponent().StopAllAnims();
  
  if(GetBEI().HasPublicStateBroadcaster()){
    auto& publicStateBroadcaster = GetBEI().GetRobotPublicStateBroadcaster();
    // Update the public state broadcaster to indicate that Guard Dog is no longer active
    publicStateBroadcaster.UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  }
  _currPublicBehaviorStage = GuardDogStage::Count;
  
  // Log some DAS Events:
  LogDasEvents();
  
  // In case we are being interrupted, make sure we send the GuardDogEnd message to unity to close the modal
  using namespace ExternalInterface;
  // robot.Broadcast( MessageEngineToGame( GuardDogEnd(_state == State::Complete) ) );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::HandleWhileActivated(const EngineToGameEvent& event)
{
  // Handle messages:
  switch( event.GetData().GetTag() ) {
    case EngineToGameTag::ObjectConnectionState:
      HandleObjectConnectionState(event.GetData().Get_ObjectConnectionState());
      break;
    case EngineToGameTag::ObjectMoved:
      HandleObjectMoved(event.GetData().Get_ObjectMoved());
      break;
    case EngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(event.GetData().Get_ObjectUpAxisChanged());
      break;
    default:
      PRINT_NAMED_WARNING("BehaviorGuardDog.HandleWhileRunning",
                          "Received an unhandled E2G event: %s",
                          MessageEngineToGameTagToString(event.GetData().GetTag()));
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::HandleObjectConnectionState(const ObjectConnectionState& msg)
{
  if (!msg.connected) {
    PRINT_NAMED_WARNING("BehaviorGuardDog.HandleObjectConnectionState",
                        "Object with ID %d and type %s has disconnected",
                        msg.objectID,
                        EnumToString(msg.object_type));
    CancelDelegates();
    SET_STATE(BlockDisconnected);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::HandleObjectMoved(const ObjectMoved& msg)
{
  // If an object has been moved before we start monitoring for motion
  //  and before we fall asleep, jump right to 'SetupInterrupted'.
  if (!_monitoringCubeMotion &&
      (_firstSleepingStartTime_s == 0.f)) {
    PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectMoved.UnexpectedBlockMovement",
                     "Received ObjectMoved message for Object with ID %d even though we're not currently monitoring for movement and not yet sleeping!",
                     msg.objectID);
    CancelDelegates();
    SET_STATE(SetupInterrupted);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::HandleObjectUpAxisChanged(const ObjectUpAxisChanged& msg)
{
  // If an object has a new UpAxis, it must have been moved. If this happens before
  //  monitoring for motion and before we fall asleep, jump right to 'Busted' reaction.
  if (!_monitoringCubeMotion) {
    if (_firstSleepingStartTime_s == 0.f) {
      PRINT_NAMED_INFO("BehaviorGuardDog.HandleObjectUpAxisChanged.UnexpectedBlockMovement",
                       "Received ObjectUpAxisChanged message for Object with ID %d even though we're not currently monitoring for movement and not yet sleeping!",
                       msg.objectID);
      CancelDelegates();
      SET_STATE(SetupInterrupted);
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
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    auto& block = it->second;
    block.upAxis = msg.upAxis;
    
    // Check if the block has been flipped over successfully:
    if (!block.hasBeenFlipped) {
      if (block.upAxis == UpAxis::ZNegative) {
        block.hasBeenFlipped = true;
        block.flippedTime_s = now;
        ++_nCubesFlipped;
        StartLightCubeAnim(objId, CubeAnimationTrigger::GuardDogSuccessfullyFlipped);
      }
      
      // Record that a block was moved at this time
      _lastCubeMovementTime_s = now;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::CubeMovementHandler(const ObjectID& objId, const float movementScore)
{
  // Ignore if we're not monitoring for cube motion right now
  if (!_monitoringCubeMotion) {
    return;
  }
  
  // Retrieve a reference to the cubeData struct corresponding to this message's object ID:
  auto it = _cubesDataMap.find(objId);
  if (it == _cubesDataMap.end()) {
    ANKI_VERIFY(false,
                "BehaviorGuardDog.HandleObjectAccel.UnknownObject",
                "Received an ObjectAccel message from an unknown object (objectId = %d)",
                objId.GetValue());
    return;
  }
  sCubeData& block = it->second;
  
  // No need to continue monitoring cube accel if the cube
  //  has been successfully flipped over, so just bail out:
  if (block.hasBeenFlipped) {
    block.movementScore = 0.f; // clear this since this block should no longer be considered 'moving'
    return;
  }
  
  block.movementScore = movementScore;
  
  block.maxMovementScore = std::max(block.maxMovementScore, block.movementScore);
  
  // Set behavior states based on cube movement and play the
  //  appropriate light cube anim:
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasStartedSleeping = (_firstSleepingStartTime_s > 0.f);
  const bool pastMaxMovementGracePeriod = hasStartedSleeping && (now - _firstSleepingStartTime_s > kMaxMovementBustedGracePeriod_s);
  // Play 'Busted' reaction if cube is moved too much while
  //   sleeping (if we're past the "grace period")
  if (block.movementScore >= kMovementScoreMax &&
      pastMaxMovementGracePeriod) {
    CancelDelegates();
    SET_STATE(Busted);
  } else if (block.movementScore > kMovementScoreDetectionThreshold) {
    if (!block.hasBeenMoved) {
      block.hasBeenMoved = true;
      block.firstMovedTime_s = now;
      ++_nCubesMoved;
      CancelDelegates();
      SET_STATE(Fakeout);
    }
    // Play the "being moved" light cube anim:
    if (block.lastCubeAnimTrigger != CubeAnimationTrigger::GuardDogBeingMoved) {
      StartLightCubeAnim(objId, CubeAnimationTrigger::GuardDogBeingMoved);
    }
    // Record that a block was moved at this time
    _lastCubeMovementTime_s = now;
  } else if (block.lastCubeAnimTrigger != CubeAnimationTrigger::GuardDogSleeping) {
    StartLightCubeAnim(objId, CubeAnimationTrigger::GuardDogSleeping);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::ComputeStartingPose(Pose3d& startingPose)
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
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_connectedCubesOnlyFilter, locatedBlocks);
  
  DEV_ASSERT(locatedBlocks.size() == 3, "BehaviorGuardDog.ComputeStartingPose.WrongNumLocatedBlocks");

  for (const auto& block : locatedBlocks) {
    const float x = block->GetPose().GetTranslation().x();
    const float y = block->GetPose().GetTranslation().y();
    blocksPoly.push_back(Point2f(x, y));
  }
  Pose2d blockCentroid(Radians(0.f), blocksPoly.ComputeCentroid());

  const auto& robotInfo = GetBEI().GetRobotInfo();
  Pose2d robotPose(robotInfo.GetPose());
  Radians centroidToRobotAngle(atan2f(robotPose.GetY() - blockCentroid.GetY(),
                                      robotPose.GetX() - blockCentroid.GetX()));
  
  Pose3d lastObservedFace;
  const bool haveLastObservedFace = (0 != GetBEI().GetFaceWorld().GetLastObservedFace(lastObservedFace, false));
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
    goalPose.TranslateForward(stepIncrement_mm);
    
    // Stop condition just in case:
    totalTranslation_mm += stepIncrement_mm;
    if (!ANKI_VERIFY(totalTranslation_mm < maxTranslation_mm,
                     "BehaviorGuardDog.ComputeStartingPose.TranslatedTooFar",
                     "Candidate starting pose has been translated way too far (%.1f mm, max is %.1f mm)",
                     totalTranslation_mm,
                     maxTranslation_mm)) {
      break;
    }
    
    const auto candidatePoseBoundingQuad = robotInfo.GetBoundingQuadXY(Pose3d(goalPose));
    intersectingObjects.clear();
    
    GetBEI().GetBlockWorld().FindLocatedIntersectingObjects(candidatePoseBoundingQuad,
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
  startingPose.SetParent(robotInfo.GetWorldOrigin());
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::StartMonitoringCubeMotion(const bool enable)
{
  if (enable == _monitoringCubeMotion) {
    return;
  }
 
  PRINT_NAMED_INFO("BehaviorGuardDog.StartMonitoringCubeMotion",
                   "%s monitoring of cube accelerometer data.",
                   enable ? "Enabling" : "Disabling");
  
  // Register/de-register cube accel listeners
  for (auto& mapEntry : _cubesDataMap) {
    auto objId = mapEntry.first;
    if (enable) {
      // generic lambda closure for cube accel listeners
      auto movementDetectedCallback = [this, objId] (const float movementScore) {
        CubeMovementHandler(objId, movementScore);
      };

      auto listener = std::make_shared<CubeAccelListeners::MovementListener>(kHighPassFiltCoef,
                                                                             kMaxMovementScoreToAdd,
                                                                             kMovementScoreDecay,
                                                                             kMovementScoreMax, // max allowed movement score
                                                                             movementDetectedCallback);

      GetBEI().GetCubeAccelComponent().AddListener(objId, listener);
      // keep a pointer to this listener around so that we can remove it later:
      mapEntry.second.cubeMovementListener = listener;
    } else {
      const bool res = GetBEI().GetCubeAccelComponent().RemoveListener(objId, mapEntry.second.cubeMovementListener);
      DEV_ASSERT(res, "BehaviorGuardDog.StartMonitoringCubeMotion.FailedRemovingListener");
    }
  }

  _monitoringCubeMotion = enable;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGuardDog::StartLightCubeAnim(const ObjectID& objId, const CubeAnimationTrigger& cubeAnimTrigger)
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
    success = GetBEI().GetCubeLightComponent().PlayLightAnim(objId, cubeAnimTrigger);
  } else {
    // we've already played a light cube anim - cancel it and play the new one:
    success = GetBEI().GetCubeLightComponent().StopAndPlayLightAnim(objId, block.lastCubeAnimTrigger, cubeAnimTrigger);
  }
    
  if (!success) {
    PRINT_NAMED_WARNING("BehaviorGuardDog.StartLightCubeAnim.PlayAnimFailed",
                        "Failed to play light cube anim trigger %s on object with ID %d",
                        EnumToString(cubeAnimTrigger),
                        objId.GetValue());
    return false;
  }
  
  block.lastCubeAnimTrigger = cubeAnimTrigger;
  return success;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorGuardDog::StartLightCubeAnims(const CubeAnimationTrigger& cubeAnimTrigger)
{
  // Should have all three cubes in the cubesData container if this function is being called:
  DEV_ASSERT(_cubesDataMap.size() == 3, "BehaviorGuardDog.StartLightCubeAnims.WrongNumCubesDataEntries");
  
  for (const auto& mapEntry : _cubesDataMap) {
    if(!StartLightCubeAnim(mapEntry.first, cubeAnimTrigger)) {
      return false;
    }
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::UpdatePublicBehaviorStage(const GuardDogStage& stage)
{
  PRINT_NAMED_INFO("GuardDog.UpdatePublicBehaviorStage", "Updating public behavior stage to %s", EnumToString(stage));
  if(GetBEI().HasPublicStateBroadcaster()){
    auto& publicStateBroadcaster = GetBEI().GetRobotPublicStateBroadcaster();
    publicStateBroadcaster.UpdateBroadcastBehaviorStage(BehaviorStageTag::GuardDog,
                                                        static_cast<uint8_t>(stage));
  }
  _currPublicBehaviorStage = stage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::RecordResult(std::string&& result)
{
  _result = std::move(result);
  
  // Compute the duration from when sleeping started til now
  const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (_firstSleepingStartTime_s == 0.f) {
    _sleepingDuration_s = 0.f;
  } else {
    _sleepingDuration_s = now - _firstSleepingStartTime_s;
  }
  
  PRINT_CH_INFO("Behaviors",
                "GuardDog.RecordResult",
                "GuardDog result = %s, sleepingDuration = %.2f",
                _result.c_str(),
                _sleepingDuration_s);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorGuardDog::LogDasEvents() const
{
  // DAS Event: "robot.guarddog.result"
  // s_val: Result of the game as a string (e.g. "PlayerSuccess", "Busted", etc.)
  // data: Total duration that Cozmo was sleeping (milliseconds)
  const int sleepingDuration_ms = std::round(_sleepingDuration_s * 1000.f);
  Anki::Util::sEvent("robot.guarddog.result",
                     {{DDATA, std::to_string(sleepingDuration_ms).c_str()}},
                     _result.c_str());
  
  // DAS Event: "robot.guarddog.cubes_moved"
  // s_val: Number of cubes that were moved
  // data: Number of cubes that were flipped successfully
  Anki::Util::sEvent("robot.guarddog.cubes_moved",
                     {{DDATA, std::to_string(_nCubesFlipped).c_str()}},
                     std::to_string(_nCubesMoved).c_str());
  
  // Loop over the cube data map to gather cube-specific data
  //   for the DAS events that follow
  std::string cubeMovedTimes, cubeFlippedTimes, cubeMaxMovementScores;
  for (auto it = _cubesDataMap.begin() ; it != _cubesDataMap.end() ; ++it) {
    const auto& block = it->second;
    
    int cubeMovedTime_ms = 0;
    if (block.firstMovedTime_s != 0.f) {
      cubeMovedTime_ms = std::round((block.firstMovedTime_s - _firstSleepingStartTime_s) * 1000.f);
    }
    
    int cubeFlippedTime_ms = 0;
    if (block.flippedTime_s != 0.f) {
      cubeFlippedTime_ms = std::round((block.flippedTime_s - _firstSleepingStartTime_s) * 1000.f);
    }
    
    const int maxMovementScore_int = std::round(block.maxMovementScore);
    
    // Append to strings and comma-separate if appropriate
    cubeMovedTimes        += std::to_string(cubeMovedTime_ms);
    cubeFlippedTimes      += std::to_string(cubeFlippedTime_ms);
    cubeMaxMovementScores += std::to_string(maxMovementScore_int);
    
    if (std::next(it) != _cubesDataMap.end()) {
      cubeMovedTimes        += ",";
      cubeFlippedTimes      += ",";
      cubeMaxMovementScores += ",";
    }
  }
  
  // DAS Event: "robot.guarddog.cubes_moved_times"
  // s_val: Time in milliseconds (from when sleeping started) that the cubes were first moved
  // data: Time in milliseconds (from when sleeping started) that the cubes were flipped over
  // Note: The two fields contain data for all three cubes, comma-separated. A value of
  //   0 means the cube was not moved/flipped
  Anki::Util::sEvent("robot.guarddog.cubes_moved_times",
                     {{DDATA, cubeFlippedTimes.c_str()}},
                     cubeMovedTimes.c_str());
  
  // DAS Event: "robot.guarddog.cube_movement_scores"
  // s_val: Max "movement score" encountered for each of the cubes
  // data: (empty - available for future use)
  Anki::Util::sEvent("robot.guarddog.cube_movement_scores",
                     {{DDATA, ""}},
                     cubeMaxMovementScores.c_str());
}

  
} // namespace Cozmo
} // namespace Anki
