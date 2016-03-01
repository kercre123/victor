/**
 * File: behaviorLookAround.cpp
 *
 * Author: Lee
 * Created: 08/13/15
 *
 * Description: Behavior for looking around the environment for stuff to interact with.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/common/basestation/utils/helpers/boundedWhile.h"
#include "anki/common/shared/radians.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

#define DISABLE_IDLE_DURING_LOOK_AROUND 0

#define SAFE_ZONE_VIZ 0 // (ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS)

#if SAFE_ZONE_VIZ
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/common/basestation/math/polygon_impl.h"
#endif

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki {
namespace Cozmo {

// TODO:(bn) ask Mooly, maybe we can use some of the alts here as well?
static const char* kBlockReactAnim = "ID_react2block_02";

using namespace ExternalInterface;

BehaviorLookAround::BehaviorLookAround(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _moveAreaCenter(robot.GetPose())
{
  SetDefaultName("LookAround");
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotObservedPossibleObject,
    EngineToGameTag::RobotPutDown
  }});

  if (GetEmotionScorerCount() == 0)
  {
    // Boredom and loneliness -> LookAround
    AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
    AddEmotionScorer(EmotionScorer(EmotionType::Social,  Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 0.7f}, {1.0f, 0.05f}}), false));
  }
}
  
BehaviorLookAround::~BehaviorLookAround()
{
}
  
bool BehaviorLookAround::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  switch (_currentState)
  {
    case State::Inactive:
    {
      if ((_lastLookAroundTime == 0.f) || (_lastLookAroundTime + kLookAroundCooldownDuration < currentTime_sec))
      {
        return true;
      }
      break;
    }
    default:
    {
      return true;
    }
  }
  return false;
}

void BehaviorLookAround::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(event.GetData().Get_RobotObservedObject(),
                           true,
                           robot);
      break;

    case EngineToGameTag::RobotObservedPossibleObject:
      HandleObjectObserved(event.GetData().Get_RobotObservedPossibleObject().possibleObject,
                           false,
                           robot);
      break;
            
    case EngineToGameTag::RobotPutDown:
      HandleRobotPutDown(event, robot);
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorLookAround.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
}
  
Result BehaviorLookAround::InitInternal(Robot& robot, double currentTime_sec)
{
  if( DISABLE_IDLE_DURING_LOOK_AROUND ) {
    robot.PushIdleAnimation("NONE");
  }
  
  // Update explorable area center to current robot pose
  ResetSafeRegion(robot);
  
  TransitionToWaitForOtherActions(robot);
  return Result::RESULT_OK;
}

void BehaviorLookAround::TransitionToWaitForOtherActions(Robot& robot)
{
  // Special state so that we can wait for other actions (from other behaviors) to complete before we do anything
  SET_STATE(State::WaitForOtherActions);
}

void BehaviorLookAround::TransitionToInactive(Robot& robot)
{
  SET_STATE(State::Inactive);
}

void BehaviorLookAround::TransitionToRoaming(Robot& robot)
{
  // Check for a collision-free pose
  Pose3d destPose;
  const int MAX_NUM_CONSIDERED_DEST_POSES = 30;
  for (int i = MAX_NUM_CONSIDERED_DEST_POSES; i > 0; --i) {
    destPose = GetDestinationPose(_currentDestination);
    
    // Get robot bounding box at destPose
    Quad2f robotQuad = robot.GetBoundingQuadXY(destPose);
    
    std::vector<ObservableObject*> existingObjects;
    robot.GetBlockWorld().FindIntersectingObjects(robotQuad,
                                                   existingObjects,
                                                   10);
    
    if (existingObjects.empty()) {
      break;
    }
    
    if (i == 1) {
      PRINT_NAMED_WARNING("BehaviorLookAround.StartMoving.NoDestPoseFound",
                          "attempts %d",
                          MAX_NUM_CONSIDERED_DEST_POSES);
      
      // Try another destination
      _currentDestination = GetNextDestination(_currentDestination);
      if (_numDestinationsLeft == 0) {
        TransitionToInactive(robot);
      }
    }
  }

  SET_STATE(State::Roaming);
  
  IActionRunner* goToPoseAction = new DriveToPoseAction(robot,
                                                        destPose,
                                                        // TODO:(bn) randomize profile
                                                        DEFAULT_PATH_MOTION_PROFILE,
                                                        false);

  // move head and lift to reasonable place before we start Roaming
  IActionRunner* setHeadAndLiftAction = new CompoundActionParallel(robot, {
      new MoveHeadToAngleAction(robot, _lookAroundHeadAngle_rads),
      new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK) });

  StartActing(new CompoundActionSequential(robot, {setHeadAndLiftAction, goToPoseAction}),
              [this, &robot](ActionResult result) {
                if( result == ActionResult::SUCCESS || result == ActionResult::FAILURE_RETRY ) {
                  _currentDestination = GetNextDestination(_currentDestination);
                }
                if (_numDestinationsLeft == 0) {
                  TransitionToInactive(robot);
                }
                else {
                  TransitionToRoaming(robot);
                }
              });
}

void BehaviorLookAround::TransitionToLookingAtPossibleObject(Robot& robot)
{
  SET_STATE(State::LookingAtPossibleObject);

  CompoundActionSequential* action = new CompoundActionSequential(robot);
  action->AddAction(new FacePoseAction(robot, _lastPossibleObjectPose, PI_F));

  // if the pose is too far away, drive towards it 
  Pose3d relPose;
  if( _lastPossibleObjectPose.GetWithRespectTo( robot.GetPose(), relPose ) ) {
    const Pose2d relPose2d(relPose);
    float dist2 = relPose2d.GetTranslation().LengthSq();
    if( dist2 > kMaxObservationDistanceSq_mm ) {
      PRINT_NAMED_DEBUG("BehaviorLookAround.PossibleObject.TooFar",
                        "Object dist^2 = %f, so moving towards it",
                        dist2);

      Vec3f newTranslation = relPose.GetTranslation();
      float oldLength = newTranslation.MakeUnitLength();
      
      Pose3d newTargetPose(RotationVector3d{},
                           newTranslation * (oldLength - kPossibleObjectViewingDist_mm),
                           &robot.GetPose());
      // TODO:(bn) motion profile here?
      action->AddAction(new DriveToPoseAction(robot, newTargetPose, DEFAULT_PATH_MOTION_PROFILE, false));
    }
  }
  else {
    PRINT_NAMED_WARNING("BehaviorLookAround.PossibleObject.NoTransform",
                        "Could not get pose of possible object W.R.T robot");
    _lastPossibleObjectPose.Print();
    _lastPossibleObjectPose.PrintNamedPathToOrigin(false);
  }

  // add a wait action, to give the vision system a chance to see the object
  action->AddAction(new WaitAction(robot, kPossibleObjectWaitTime_s));
  
  // Note that in the positive case, this drive to action is likely to get canceled
  // because we discover it is a real object
  StartActing(action,
              [this,&robot](ActionResult result) {
                if( result != ActionResult::CANCELLED ) {
                  // we finished without observing an object, so go back to roaming
                  TransitionToRoaming(robot);
                }
              });
}

void BehaviorLookAround::TransitionToExaminingFoundObject(Robot& robot)
{
  if( _recentObjects.empty() ) {
    TransitionToRoaming(robot);
  }

  SET_STATE(State::ExaminingFoundObject);

  robot.GetMoodManager().TriggerEmotionEvent("FoundObservedObject", MoodManager::GetCurrentTimeInSeconds());
  
  ObjectID recentObjectID = *_recentObjects.begin();

  PRINT_NAMED_DEBUG("BehaviorLookAround.TransitionToExaminingFoundObject", "examining new object %d",
                    recentObjectID.GetValue());
  
  StartActing(new CompoundActionSequential(robot, {
                  new FaceObjectAction(robot, recentObjectID, PI_F),
                  // TODO:(bn) vary animation here?
                  new PlayAnimationAction(robot, kBlockReactAnim) }),
               [this, &robot, recentObjectID](ActionResult result) {
                 if( result == ActionResult::SUCCESS ) {
                   PRINT_NAMED_DEBUG("BehaviorLookAround.Objects",
                                     "Done examining object %d, adding to boring list",
                                     recentObjectID.GetValue());
                   _recentObjects.erase(recentObjectID);
                   _oldBoringObjects.insert(recentObjectID);
                 }

                 if( result != ActionResult::CANCELLED ) {
                   TransitionToRoaming(robot);
                 }
             });
}


IBehavior::Status BehaviorLookAround::UpdateInternal(Robot& robot, double currentTime_sec)
{

#if SAFE_ZONE_VIZ
  Point2f center = { _moveAreaCenter.GetTranslation().x(), _moveAreaCenter.GetTranslation().y() };
  robot.GetContext()->GetVizManager()->DrawXYCircle(robot.GetID(), ::Anki::NamedColors::GREEN, center, _safeRadius);
#endif

  if( IsActing() ) {
    return Status::Running;
  }
  
  if( _currentState == State::WaitForOtherActions ) {
    if( robot.GetActionList().IsEmpty() ) {
      TransitionToRoaming(robot);
    }
    return Status::Running;
  }
  
#if SAFE_ZONE_VIZ
  robot.GetContext()->GetVizManager()->EraseCircle(robot.GetID());
#endif
  
  return Status::Complete;
}
  
Pose3d BehaviorLookAround::GetDestinationPose(BehaviorLookAround::Destination destination)
{
  Pose3d destPose(_moveAreaCenter);
  
  s32 baseAngleDegrees = 0;
  bool shouldRotate = true;
  switch (destination)
  {
    case Destination::Center:
    {
      shouldRotate = false;
      break;
    }
    case Destination::East:
    {
      baseAngleDegrees = -45;
      break;
    }
    case Destination::North:
    {
      baseAngleDegrees = 45;
      break;
    }
    case Destination::West:
    {
      baseAngleDegrees = 135;
      break;
    }
    case Destination::South:
    {
      baseAngleDegrees = -135;
      break;
    }
    case Destination::Count:
    default:
    {
      shouldRotate = false;
      PRINT_NAMED_ERROR("LookAround_Behavior.GetDestinationPose.InvalidDestination",
                        "Reached invalid destination %d.", destination);
      break;
    }
  }
  
  if (shouldRotate)
  {
    // Our destination regions are 90 degrees, so we randomly pick up to 90 degrees to vary our destination
    s32 randAngleMod = GetRNG().RandInt(90);
    destPose.SetRotation(destPose.GetRotation() * Rotation3d(DEG_TO_RAD(baseAngleDegrees + randAngleMod), Z_AXIS_3D()));
  }
  
  if (Destination::Center != destination)
  {
    // The multiplier amount of change we want to vary the radius by (-0.25 means from 75% to 100% of radius)
    static const f32 radiusVariation = -0.25f;
    f32 distMod = GetRNG().RandDbl() * radiusVariation * _safeRadius;
    destPose.SetTranslation(destPose.GetTranslation() + destPose.GetRotation() * Point3f(_safeRadius + distMod, 0, 0));
  }
  
  return destPose;
}

Result BehaviorLookAround::InterruptInternal(Robot& robot, double currentTime_sec)
{
  ResetBehavior(robot, currentTime_sec);
  return Result::RESULT_OK;
}
  
void BehaviorLookAround::StopInternal(Robot& robot, double currentTime_sec)
{
  if( DISABLE_IDLE_DURING_LOOK_AROUND ) {
    robot.PopIdleAnimation();
  }

  ResetBehavior(robot, currentTime_sec);
}
  
void BehaviorLookAround::ResetBehavior(Robot& robot, float currentTime_sec)
{
  // This is the last update before we stop running, so store off time
  _lastLookAroundTime = currentTime_sec;
      
  // Reset our number of destinations for next time we run this behavior
  _numDestinationsLeft = kDestinationsToReach;

  SET_STATE(State::Inactive);
  
  _recentObjects.clear();
  _oldBoringObjects.clear();
}
  
void BehaviorLookAround::HandleObjectObserved(const RobotObservedObject& msg, bool confirmed, Robot& robot)
{
  assert(IsRunning());

  static const std::set<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };
  
  if (familyList.count(msg.objectFamily) > 0) {
    if( ! confirmed ) {
      if( _currentState != State::LookingAtPossibleObject ) {
        _lastPossibleObjectPose = Pose3d{0, Z_AXIS_3D(),
                                         {msg.world_x, msg.world_y, msg.world_z},
                                         robot.GetWorldOrigin()};
        PRINT_NAMED_DEBUG("BehaviorLookAround.HandleObjectObserved.LookingAtPossibleObject",
                          "stopping to look at possible object");
        StopActing(false);
        TransitionToLookingAtPossibleObject(robot);
      }
    }
    else if( 0 == _oldBoringObjects.count(msg.objectID) && _currentState != State::ExaminingFoundObject) {

      PRINT_NAMED_DEBUG("BehaviorLookAround.HandleObjectObserved.ExaminingFoundObject",
                        "stopping to look at found object id %d",
                        msg.objectID);

      _recentObjects.insert(msg.objectID);

      StopActing(false);
      TransitionToExaminingFoundObject(robot);
    }

    ObservableObject* object = robot.GetBlockWorld().GetObjectByID(msg.objectID);
    if (nullptr != object)
    {
      UpdateSafeRegion(object->GetPose().GetTranslation());
    }
  }
}

void BehaviorLookAround::UpdateSafeRegion(const Vec3f& objectPosition)
{
  Vec3f translationDiff = objectPosition - _moveAreaCenter.GetTranslation();
  // We're only going to care about the XY plane distance
  translationDiff.z() = 0;
  const f32 distanceSqr = translationDiff.LengthSq();
  
  // If the distance between our safe area center and the object we're seeing exceeds our current safe radius,
  // update our center point and safe radius to include the object's location
  if (_safeRadius * _safeRadius < distanceSqr)
  {
    const f32 distance = std::sqrt(distanceSqr);
    
    // Ratio is ratio of distance to new center point to distance of observed object
    const f32 newCenterRatio = 0.5f - _safeRadius / (2.0f * distance);
    
    // The new center is calculated as: C1 = C0 + (ObjectPosition - C0) * Ratio
    _moveAreaCenter.SetTranslation(_moveAreaCenter.GetTranslation() + translationDiff * newCenterRatio);
    
    // The new radius is simply half the distance between the far side of the previus circle and the observed object
    _safeRadius = 0.5f * (distance + _safeRadius);
  }
}

const char* BehaviorLookAround::DestinationToString(const BehaviorLookAround::Destination& dest)
{
  switch(dest) {
    case Destination::North: return "north";
    case Destination::West: return "west";
    case Destination::South: return "south";
    case Destination::East: return "east";
    case Destination::Center: return "center";
    case Destination::Count: return "count";
  };
}

BehaviorLookAround::Destination BehaviorLookAround::GetNextDestination(BehaviorLookAround::Destination current)
{
  static BehaviorLookAround::Destination previous = BehaviorLookAround::Destination::Center;
  
  // If we've visited enough destinations, go back to center
  if (_numDestinationsLeft <= 1)
  {
    _numDestinationsLeft = 0;
    PRINT_NAMED_DEBUG("BehaviorLookAround.GetNextDestination.ReturnToCenter", "going back to center");
    return Destination::Center;
  }
  
  // Otherwise pick a new place that doesn't include the center
  std::set<Destination> all = {
    Destination::North,
    Destination::West,
    Destination::South,
    Destination::East
  };
  
  all.erase(current);
  all.erase(previous);
  previous = current;
  
  // Pick a random destination from the remaining options
  s32 randIndex = GetRNG().RandInt(static_cast<s32>(all.size()));
  auto newDestIter = all.begin();
  while (randIndex-- > 0) { newDestIter++; }

  _numDestinationsLeft--;

  PRINT_NAMED_DEBUG("BehaviorLookAround.GetNextDestination", "%s (%d left)",
                    DestinationToString(*newDestIter),
                    _numDestinationsLeft);
  
  return *newDestIter;
}
  
void BehaviorLookAround::HandleRobotPutDown(const EngineToGameEvent& event, Robot& robot)
{
  const RobotPutDown& msg = event.GetData().Get_RobotPutDown();
  if (robot.GetID() == msg.robotID)
  {
    ResetSafeRegion(robot);
  }
}
  
void BehaviorLookAround::ResetSafeRegion(Robot& robot)
{
  _moveAreaCenter = robot.GetPose();
  _safeRadius = kDefaultSafeRadius;
}

void BehaviorLookAround::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  PRINT_NAMED_DEBUG("BehaviorLookAround.TransitionTo", "%s", stateName.c_str());
}


} // namespace Cozmo
} // namespace Anki
