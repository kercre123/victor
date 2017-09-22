/**
 * File: behaviorAcknowledgeObject.cpp
 *
 * Author:  Andrew Stein
 * Created: 2016-06-14
 *
 * Description:  Simple quick reaction to a "new" block, just to show Cozmo has noticed it.
 *               Cozmo just turns towards the object and then plays a reaction animation if
 *               it still sees the object after the turn.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeObject.h"

#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/activeCube.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iReactToObjectListener.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageFromActiveObject.h"
#include "util/console/consoleInterface.h"


#include <limits>

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(bool, kVizPossibleStackCube, "BehaviorAcknowledgeObject", false);
CONSOLE_VAR(f32,  kBackupDistance_mm,    "BehaviorAcknowledgeObject", 25.f);
CONSOLE_VAR(f32,  kBackupSpeed_mmps,     "BehaviorAcknowledgeObject", 100.f);

static const char * const kLogChannelName = "Behaviors";

static const char * const kReactionAnimGroupKey   = "ReactionAnimGroup";
static const char * const kMaxTurnAngleKey        = "MaxTurnAngle_deg";
static const char * const kPanToleranceKey        = "PanTolerance_deg";
static const char * const kTiltToleranceKey       = "TiltTolerance_deg";
static const char * const kNumImagesToWaitForKey  = "NumImagesToWaitFor";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAcknowledgeObject::BehaviorAcknowledgeObject(const Json::Value& config)
: IBehavior(config)
, _ghostStackedObject(new Block_Cube1x1(ObjectType::Block_LIGHTCUBE_GHOST))
, _shouldCheckBelowTarget(true)
{
  LoadConfig(config);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  // give the ghost object and ID so we can visualize it
  _ghostStackedObject->SetVizManager(robot.GetContext()->GetVizManager());
  _ghostStackedObject->InitPose(Pose3d(), PoseState::Dirty); // we rely on restoring poses, so just set a valid one
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorAcknowledgeObject::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // don't actually init until the first Update call. This gives other messages that came in this tick a
  // chance to be processed, in case we see multiple objects in the same tick.
  _shouldStart = true;

  // update the id of the temporary cube so it has a valid id (that isn't 0)
  if( !_ghostStackedObject->GetID().IsSet() ) {
    _ghostStackedObject->SetID();
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorAcknowledgeObject.GhostObjectCreated",
                   "Created ghost object with id %d for debug viz",
                   _ghostStackedObject->GetID().GetValue());
  }
  
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorAcknowledgeObject::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _shouldStart ) {
    _shouldStart = false;
    // now figure out which object to react to
    BeginIteration(behaviorExternalInterface);
  }

  return super::UpdateInternal_WhileRunning(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  
  JsonTools::GetValueOptional(config,kReactionAnimGroupKey,_params.reactionAnimTrigger);
  
  if(GetAngleOptional(config, kMaxTurnAngleKey, _params.maxTurnAngle_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetMaxTurnAngle",
                      "%.1fdeg", _params.maxTurnAngle_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kPanToleranceKey, _params.panTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetPanTolerance",
                      "%.1fdeg", _params.panTolerance_rad.getDegrees());
  }
  
  if(GetAngleOptional(config, kTiltToleranceKey, _params.tiltTolerance_rad, true)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetTiltTolerance",
                      "%.1fdeg", _params.tiltTolerance_rad.getDegrees());
  }
  
  if(GetValueOptional(config, kNumImagesToWaitForKey, _params.numImagesToWaitFor)) {
    PRINT_NAMED_DEBUG("IBehaviorPoseBasedAcknowledgement.LoadConfig.SetNumImagesToWaitFor",
                      "%d", _params.numImagesToWaitFor);
  }
} // LoadConfig()
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::BeginIteration(BehaviorExternalInterface& behaviorExternalInterface)
{
  _currTarget.UnSet();
  if(_targets.size() <= 0) {
    return;
  }
  
  _currTarget = *_targets.begin();
  DEV_ASSERT(_currTarget.IsSet(), "BehaviorAcknowledgeObject.GotUnsetTarget");

  const ObservableObject* targetObj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_currTarget);

  if( nullptr == targetObj ) {
    // could happen if the cube "disappears" somehow between this behavior getting triggered and the time this
    // function is called
    PRINT_NAMED_WARNING("BehaviorAcknowledgeObject.BeginIteration.NullObject",
                        "Object id %d is a target, but can't get it from blockworld",
                        _currTarget.GetValue());
    _targets.erase(_currTarget.GetValue());
    _currTarget.UnSet();
    
    // let it try again with another target, if there is one
    if( !_targets.empty() ) {
      BeginIteration(behaviorExternalInterface);
    }
    
    return;
  }
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();

  Pose3d poseWrtRobot;
  targetObj->GetPose().GetWithRespectTo(robot.GetPose(), poseWrtRobot);
  
  // Only bother checking below the robot if the object is significantly above the robot
  _shouldCheckBelowTarget = poseWrtRobot.GetTranslation().z() > ROBOT_BOUNDING_Z * 0.5f;
  
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, _currTarget,
                                                                    _params.maxTurnAngle_rad);
  turnAction->SetTiltTolerance(_params.tiltTolerance_rad);
  turnAction->SetPanTolerance(_params.panTolerance_rad);

  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(turnAction);

  if( _params.numImagesToWaitFor > 0 ) {
    // If visual verification fails (e.g. because object has moved since we saw it),
    // then the compound action fails, and we don't play animation, since it's silly
    // to react to something that's no longer there.
    VisuallyVerifyObjectAction* verifyAction = new VisuallyVerifyObjectAction(robot, _currTarget);
    verifyAction->SetNumImagesToWaitFor(_params.numImagesToWaitFor);
    action->AddAction(verifyAction);
  }

  if(!ShouldStreamline()){
    action->AddAction(new TriggerLiftSafeAnimationAction(robot, _params.reactionAnimTrigger));
  }

  StartActing(action,
              [this, &behaviorExternalInterface](ActionResult result)
              {
                if(result == ActionResult::SUCCESS)
                {
                  // The first try, don't need to force a backup: computing a look up may be sufficient
                  _forceBackupWhenCheckingForStack = false;
                  LookForStackedCubes(behaviorExternalInterface);
                }
                else
                {
                  // If we don't successfully turn towards the cube, then just finish
                  FinishIteration(behaviorExternalInterface);
                }
              });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::LookForStackedCubes(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* obj = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_currTarget);
  if( nullptr == obj ) {
    PRINT_NAMED_WARNING("BehaviorAcknowledgeObject.StackedCube.NullTargetObject",
                        "Target object %d returned null from blockworld",
                        _currTarget.GetValue());
    FinishIteration(behaviorExternalInterface);
    return;
  }

  const float zSize = obj->GetDimInParentFrame<'Z'>();
  
  const float offsetAboveTarget = zSize;
  const float offsetBelowTarget = -zSize;
  
  // if we can already see below block, don't look down after looking up
  if(_shouldCheckBelowTarget){
    _shouldCheckBelowTarget = !CheckIfGhostBlockVisible(behaviorExternalInterface, obj, offsetBelowTarget);
  }
  
  // if we can already see above block, don't bother looking up
  bool outsideFOV = false;
  const bool ghostBlockAboveVisibleFromHere = CheckIfGhostBlockVisible(behaviorExternalInterface, obj, offsetAboveTarget, outsideFOV);
  if(!ghostBlockAboveVisibleFromHere && outsideFOV)
  {
    // Can't see ghost cube from current position, but it's because it's outside our
    // current FOV.
    SetGhostBlockPoseRelObject(behaviorExternalInterface, obj, offsetAboveTarget);
    
    bool backupFirst = false;
    
    if(_forceBackupWhenCheckingForStack)
    {
      backupFirst = true;
    }
    else
    {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      const Robot& robot = behaviorExternalInterface.GetRobot();
      // First see if we need to back up first, by checking if we can even reach a head
      // angle that would put the center of the closest marker (in XY) in our FOV. Note that
      // we don't just use the ghost object's pose b/c at close range, seeing the closest
      // marker could require a significantly larger angle than the center of the cube.
      Radians headAngle(0.f);
      Pose3d checkPose;
      Result result = _ghostStackedObject->GetClosestMarkerPose(robot.GetPose(), true, checkPose);
      
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("BehaviorAcknowledgeObject.LookForStackedCubes.ClosestMarkerPoseFailed", "");
        checkPose = _ghostStackedObject->GetPose();
      }
      
      result = robot.ComputeHeadAngleToSeePose(checkPose, headAngle, .1f);
      
      const f32 kMaxHeadAngleBeforeBackingUp_rad = MAX_HEAD_ANGLE - DEG_TO_RAD(10);
      backupFirst = (RESULT_OK != result || FLT_GT(headAngle.ToFloat(), kMaxHeadAngleBeforeBackingUp_rad));
      
      // In case this fails and we come back here (e.g. because we may be able to compute a head angle for the
      // marker's pose but then actually looking there still doesn't actually make the ghost block visible because
      // we are too close), then make sure we back up the next time so we don't get stuck in a loop here.
      _forceBackupWhenCheckingForStack = true;
    }
    
    // Try looking up at the cube and then trying this whole thing again
    PRINT_CH_DEBUG(kLogChannelName,
                   "BehaviorAcknowledgeObject.LookForStackedCubes.LookingUpToSeeCubeAbove",
                   "BackingUpFirst=%c", backupFirst ? 'Y' : 'N');
    
    LookAtGhostBlock(behaviorExternalInterface, backupFirst, &BehaviorAcknowledgeObject::LookForStackedCubes);
    return;
  }
  
  // check for blocks below
  if(_shouldCheckBelowTarget){
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorAcknowledgeObject.LookForStackedCubes.LookingDownToSeeCubeBelow", "");
    SetGhostBlockPoseRelObject(behaviorExternalInterface, obj, offsetBelowTarget);
    LookAtGhostBlock(behaviorExternalInterface, false, &BehaviorAcknowledgeObject::FinishIteration);
    return;
  }
  
  FinishIteration(behaviorExternalInterface);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::SetGhostBlockPoseRelObject(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset)
{
  Pose3d ghostPose = obj->GetPose().GetWithRespectToRoot();
  ghostPose.SetTranslation({
    ghostPose.GetTranslation().x(),
    ghostPose.GetTranslation().y(),
    ghostPose.GetTranslation().z() + zOffset});
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  // Using the PoseConfimer for ghost objects is weird, we need to fake PoseConfirmations here
  robot.GetObjectPoseConfirmer().SetGhostObjectPose(_ghostStackedObject.get(), ghostPose, PoseState::Dirty);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool BehaviorAcknowledgeObject::CheckIfGhostBlockVisible(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset)
{
  bool temp = false;
  return CheckIfGhostBlockVisible(behaviorExternalInterface, obj, zOffset, temp);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeObject::CheckIfGhostBlockVisible(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset, bool& shouldRetry)
{
  // store the current ghost pose so that it can be restored after the check
  const Pose3d currentGhostPose = _ghostStackedObject->GetPose();
  
  SetGhostBlockPoseRelObject(behaviorExternalInterface, obj, zOffset);
  
  if( kVizPossibleStackCube ) {
    _ghostStackedObject->Visualize(NamedColors::WHITE);
  }
  
  // check if this fake object is theoretically visible from our current position
  static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see // ANDREW: is this true?
  static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it, size doesn't matter
  
  // it's ok if markers have nothing behind them, or even if they are occluded. What we want to know is if
  // we'd gain any information by trying to look at the marker pose
  static const std::set<Vision::KnownMarker::NotVisibleReason> okReasons{{
    Vision::KnownMarker::NotVisibleReason::IS_VISIBLE,
    Vision::KnownMarker::NotVisibleReason::OCCLUDED,
    Vision::KnownMarker::NotVisibleReason::NOTHING_BEHIND }};
  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  Vision::KnownMarker::NotVisibleReason reason = _ghostStackedObject->IsVisibleFromWithReason(robot.GetVisionComponent().GetCamera(),
                                                                                              kMaxNormalAngle,
                                                                                              kMinImageSizePix,
                                                                                              false);
  
  //restore ghost pose
  robot.GetObjectPoseConfirmer().SetGhostObjectPose(_ghostStackedObject.get(), currentGhostPose, PoseState::Dirty);
  
  // If we couldn't see the ghost cube b/c it was outside FOV, we should retry
  shouldRetry = (reason == Vision::KnownMarker::NotVisibleReason::OUTSIDE_FOV);
  
  if( okReasons.find(reason) != okReasons.end() ) {
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorAcknowledgeObject.StackedCube.AlreadyVisible",
                   "Any stacked cube that exists should already be visible (reason %s), nothing to do",
                   NotVisibleReasonToString(reason));
    return true;
  }
  else {
    PRINT_CH_DEBUG(kLogChannelName, "BehaviorAcknowledgeObject.StackedCube.NotVisible",
                   "looking up in case there is another cube on top of this one (reason %s)",
                   NotVisibleReasonToString(reason));
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void BehaviorAcknowledgeObject::LookAtGhostBlock(BehaviorExternalInterface& behaviorExternalInterface, bool backupFirst, void(T::*callback)(BehaviorExternalInterface&))
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  CompoundActionSequential* compoundAction = new CompoundActionSequential(robot);
  
  if(backupFirst)
  {
    compoundAction->AddAction(new DriveStraightAction(robot, -kBackupDistance_mm, kBackupSpeed_mmps, false));
  }
  
  // use 0 for max turn angle so we only look with the head
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, ObjectID{}, 0.f);
  turnAction->UseCustomObject(_ghostStackedObject.get());
  
  // turn and then wait for images to give us a chance to see the new marker
  compoundAction->AddAction(turnAction);
  compoundAction->AddAction(new WaitForImagesAction(robot, _params.numImagesToWaitFor,
                                                    VisionMode::DetectingMarkers));
  
  StartActing(compoundAction, std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::FinishIteration(BehaviorExternalInterface& behaviorExternalInterface)
{
  // notify about the id being done and decide what to do next
  // inform parent class that we completed a reaction
  for(auto listener: _objectListeners){
    listener->ReactedToID(behaviorExternalInterface, _currTarget.GetValue());
  }
  
  _targets.erase(_currTarget.GetValue());
  
  auto callback = [this,&behaviorExternalInterface]()
  {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedObject);
    // move on to the next target, if there is one
    BeginIteration(behaviorExternalInterface);
  };
  
  // NOTE: this is not really sufficient logic, because we could fail to turn towards
  // a remaining target object and then leave the head not facing this object. COZMO-7108
  if(!_targets.empty())
  {
    // Have other targets to react to, don't turn towards this target. Just run the callback.
    callback();
  }
  else
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // There's nothing else to react to, so turn back towards the target so we're
    // left facing it (as long as it wasn't too far to turn towards to begin with)
    StartActing(new TurnTowardsObjectAction(robot, _currTarget, _params.maxTurnAngle_rad), callback);
  }
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // if we get interrupted for any reason, kill the queue. We don't want to back up a bunch of stuff in here,
  // this is meant to handle seeing new objects while we are running
  for(auto listener: _objectListeners){
    listener->ClearDesiredTargets(behaviorExternalInterface);
  }
  
  _currTarget.UnSet();
  _targets.clear();
  _shouldStart = false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeObject::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return !_targets.empty();
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeObject::AddListener(IReactToObjectListener* listener)
{
  _objectListeners.insert(listener);
}

  

} // namespace Cozmo
} // namespace Anki

