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


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorLookAround.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/shared/radians.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "util/helpers/boundedWhile.h"
#include <cmath>

#define SAFE_ZONE_VIZ 0 // (ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS)

#if SAFE_ZONE_VIZ
#include "engine/viz/vizManager.h"
#include "coretech/common/engine/math/polygon_impl.h"
#endif

#define SET_STATE(s) SetState_internal(s, #s)

namespace Anki {
namespace Cozmo {

namespace{
static const char* const kShouldHandleConfirmedKey = "shouldHandleConfirmedObject";
static const char* const kShouldHandlePossibleKey = "shouldHandlePossibleObject";
// The default radius (in mm) we assume exists for us to move around in
constexpr static f32 kDefaultSafeRadius = 150;
// How far back (at most) to move the center when we encounter a cliff
constexpr static f32 kMaxCliffShiftDist = 100.0f;
// Number of destinations we want to reach before resting for a bit (needs to be at least 2)
constexpr static u32 kDestinationsToReach = 6;
// How far back from a possible object to observe it (at most)
constexpr static f32 kMaxObservationDistanceSq_mm = SQUARE(200.0f);
// If the possible block is too far, this is the distance to view it from
constexpr static f32 kPossibleObjectViewingDist_mm = 100.0f;
}


using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAround::InstanceConfig::InstanceConfig()
{
  shouldHandleConfirmedObjectOverved = true;
  shouldHandlePossibleObjectOverved  = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAround::DynamicVariables::DynamicVariables()
{
  currentState             = State::Inactive;
  currentDestination       = Destination::North;
  safeRadius               = kDefaultSafeRadius;
  numDestinationsLeft      = kDestinationsToReach;
  lookAroundHeadAngle_rads = DEG_TO_RAD(-5);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAround::BehaviorLookAround(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.shouldHandleConfirmedObjectOverved = config.get(kShouldHandleConfirmedKey, true).asBool();
  _iConfig.shouldHandlePossibleObjectOverved = config.get(kShouldHandlePossibleKey, true).asBool();
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotObservedPossibleObject,
    EngineToGameTag::RobotOffTreadsStateChanged,
    EngineToGameTag::CliffEvent
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAround::~BehaviorLookAround()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kShouldHandleConfirmedKey,
    kShouldHandlePossibleKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookAround::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(event.GetData().Get_RobotObservedObject(),
                           true);
      break;

    case EngineToGameTag::RobotObservedPossibleObject:
      HandleObjectObserved(event.GetData().Get_RobotObservedPossibleObject().possibleObject,
                           false);
      break;
            
    case EngineToGameTag::RobotOffTreadsStateChanged:
      HandleRobotOfftreadsStateChanged(event);
      break;

    case EngineToGameTag::CliffEvent:
      // handled below
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorLookAround.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
    case EngineToGameTag::RobotObservedPossibleObject:
    case EngineToGameTag::RobotOffTreadsStateChanged:
      // Handled above
      break;

    case EngineToGameTag::CliffEvent:
      // always handle cliff events. Most of the time we'll reset the safe region anyway, but if we get
      // resumed we won't
      HandleCliffEvent(event);
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorLookAround.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  // Update explorable area center to current robot pose
  ResetSafeRegion();
  
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::TransitionToWaitForOtherActions()
{
  // Special state so that we can wait for other actions (from other behaviors) to complete before we do anything
  SET_STATE(State::WaitForOtherActions);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::TransitionToInactive()
{
  SET_STATE(State::Inactive);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::TransitionToRoaming()
{
  // Check for a collision-free pose
  Pose3d destPose;
  const int MAX_NUM_CONSIDERED_DEST_POSES = 30;
  for (int i = MAX_NUM_CONSIDERED_DEST_POSES; i > 0; --i) {
    destPose = GetDestinationPose(_dVars.currentDestination);
    
    auto& robotInfo = GetBEI().GetRobotInfo();
    // Get robot bounding box at destPose
    Quad2f robotQuad = robotInfo.GetBoundingQuadXY(destPose);
    
    std::vector<ObservableObject*> existingObjects;
    GetBEI().GetBlockWorld().FindLocatedIntersectingObjects(robotQuad, existingObjects, 10);
    
    if (existingObjects.empty()) {
      break;
    }
    
    if (i == 1) {
      PRINT_NAMED_WARNING("BehaviorLookAround.StartMoving.NoDestPoseFound",
                          "attempts %d",
                          MAX_NUM_CONSIDERED_DEST_POSES);
      
      // Try another destination
      _dVars.currentDestination = GetNextDestination(_dVars.currentDestination);
      if (_dVars.numDestinationsLeft == 0) {
        TransitionToInactive();
        return;
      }
    }
  }

  SET_STATE(State::Roaming);

  IActionRunner* goToPoseAction = new DriveToPoseAction(destPose,
                                                        false);

  // move head and lift to reasonable place before we start Roaming
  IActionRunner* setHeadAndLiftAction = new CompoundActionParallel({
      new MoveHeadToAngleAction(_dVars.lookAroundHeadAngle_rads),
      new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK) });

  DelegateIfInControl(new CompoundActionSequential({setHeadAndLiftAction, goToPoseAction}),
              [this](ActionResult result) {
                const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(result);
                if( resCat == ActionResultCategory::SUCCESS || resCat == ActionResultCategory::RETRY ) {
                  _dVars.currentDestination = GetNextDestination(_dVars.currentDestination);
                }
                if (_dVars.numDestinationsLeft == 0) {
                  TransitionToInactive();
                }
                else {
                  TransitionToRoaming();
                }
              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::TransitionToLookingAtPossibleObject()
{
  SET_STATE(State::LookingAtPossibleObject);

  auto& robotInfo = GetBEI().GetRobotInfo();
  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction(new TurnTowardsPoseAction(_dVars.lastPossibleObjectPose));

  // if the pose is too far away, drive towards it 
  Pose3d relPose;
  if( _dVars.lastPossibleObjectPose.GetWithRespectTo( robotInfo.GetPose(), relPose ) ) {
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
                           robotInfo.GetPose());

      action->AddAction(new DriveToPoseAction(newTargetPose, false));
    }
  }
  else {
    PRINT_NAMED_WARNING("BehaviorLookAround.PossibleObject.NoTransform",
                        "Could not get pose of possible object W.R.T robot");
    if(ANKI_DEVELOPER_CODE)
    {
      _dVars.lastPossibleObjectPose.Print();
      _dVars.lastPossibleObjectPose.PrintNamedPathToRoot(false);
    }
  }

  // add a search action after driving / facing, in case we don't see the object
  action->AddAction(new SearchForNearbyObjectAction());
  
  // Note that in the positive case, this drive to action is likely to get canceled
  // because we discover it is a real object
  DelegateIfInControl(action,
              [this](ActionResult result) {
                if( IActionRunner::GetActionResultCategory(result) != ActionResultCategory::CANCELLED ) {
                  // we finished without observing an object, so go back to roaming
                  TransitionToRoaming();
                }
              });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::TransitionToExaminingFoundObject()
{
  if( _dVars.recentObjects.empty() ) {
    TransitionToRoaming();
    return;
  }

  SET_STATE(State::ExaminingFoundObject);
  
  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("FoundObservedObject",
                                    MoodManager::GetCurrentTimeInSeconds());
  }
  
  ObjectID recentObjectID = *_dVars.recentObjects.begin();

  PRINT_NAMED_DEBUG("BehaviorLookAround.TransitionToExaminingFoundObject", "examining new object %d",
                    recentObjectID.GetValue());
  
  DelegateIfInControl(new CompoundActionSequential({
                  new TurnTowardsObjectAction(recentObjectID),
                  new TriggerLiftSafeAnimationAction(AnimationTrigger::DEPRECATED_BlockReact) }),
               [this, recentObjectID](ActionResult result) {
                 if( result == ActionResult::SUCCESS ) {
                   PRINT_NAMED_DEBUG("BehaviorLookAround.Objects",
                                     "Done examining object %d, adding to boring list",
                                     recentObjectID.GetValue());
                   _dVars.recentObjects.erase(recentObjectID);
                   _dVars.oldBoringObjects.insert(recentObjectID);
                 }

                 if( IActionRunner::GetActionResultCategory(result) != ActionResultCategory::CANCELLED ) {
                   TransitionToRoaming();
                 }
             });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
#if SAFE_ZONE_VIZ
  const auto& robotInfo = GetBEI().GetRobotInfo();
  Point2f center = { _dVars.moveAreaCenter.GetTranslation().x(), _dVars.moveAreaCenter.GetTranslation().y() };
  robotInfo.GetContext()->GetVizManager()->DrawXYCircle(robotInfo.GetID(), ::Anki::NamedColors::GREEN, center, _dVars.safeRadius);
#endif

  if( IsControlDelegated() ) {
    return;
  }
  
  if( _dVars.currentState == State::WaitForOtherActions ) {
    if( !IsControlDelegated() ) {
      TransitionToRoaming();
    }
    return;
  }
  
#if SAFE_ZONE_VIZ
  robotInfo.GetContext()->GetVizManager()->EraseCircle(robotInfo.GetID());
#endif
  CancelSelf();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Pose3d BehaviorLookAround::GetDestinationPose(BehaviorLookAround::Destination destination)
{
  Pose3d destPose(_dVars.moveAreaCenter);
  
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
    f32 distMod = GetRNG().RandDbl() * radiusVariation * _dVars.safeRadius;
    destPose.SetTranslation(destPose.GetTranslation() + destPose.GetRotation() * Point3f(_dVars.safeRadius + distMod, 0, 0));
  }
  
  return destPose;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::HandleObjectObserved(const RobotObservedObject& msg, bool confirmed)
{
  assert(IsActivated());

  if( ! _iConfig.shouldHandleConfirmedObjectOverved && confirmed ) {
    return;
  }

  if( !_iConfig.shouldHandlePossibleObjectOverved && !confirmed ) {
    return;
  }
  
  static const std::set<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };
  
  if (familyList.count(msg.objectFamily) > 0) {
    if( ! confirmed ) {
      if( _dVars.currentState != State::LookingAtPossibleObject && _dVars.currentState != State::ExaminingFoundObject ) {
        const auto& robotInfo = GetBEI().GetRobotInfo();
        _dVars.lastPossibleObjectPose = Pose3d{0, Z_AXIS_3D(),
                                         {msg.pose.x, msg.pose.y, msg.pose.z},
                                         robotInfo.GetWorldOrigin()};
        PRINT_NAMED_DEBUG("BehaviorLookAround.HandleObjectObserved.LookingAtPossibleObject",
                          "stopping to look at possible object");
        CancelDelegates(false);
        TransitionToLookingAtPossibleObject();
      }
    }
    else if( 0 == _dVars.oldBoringObjects.count(msg.objectID) && _dVars.currentState != State::ExaminingFoundObject) {

      PRINT_NAMED_DEBUG("BehaviorLookAround.HandleObjectObserved.ExaminingFoundObject",
                        "stopping to look at found object id %d",
                        msg.objectID);

      _dVars.recentObjects.insert(msg.objectID);

      CancelDelegates(false);
      TransitionToExaminingFoundObject();
    }

    const ObservableObject* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(msg.objectID);
    if (nullptr != object)
    {
      UpdateSafeRegionForCube(object->GetPose().GetTranslation());
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::UpdateSafeRegionForCube(const Vec3f& objectPosition)
{
  Vec3f translationDiff = objectPosition - _dVars.moveAreaCenter.GetTranslation();
  // We're only going to care about the XY plane distance
  translationDiff.z() = 0;
  const f32 distanceSqr = translationDiff.LengthSq();
  
  // If the distance between our safe area center and the object we're seeing exceeds our current safe radius,
  // update our center point and safe radius to include the object's location
  if (_dVars.safeRadius * _dVars.safeRadius < distanceSqr)
  {
    const f32 distance = std::sqrt(distanceSqr);
    
    // Ratio is ratio of distance to new center point to distance of observed object
    const f32 newCenterRatio = 0.5f - _dVars.safeRadius / (2.0f * distance);
    // The new center is calculated as: C1 = C0 + (ObjectPosition - C0) * Ratio
    _dVars.moveAreaCenter.SetTranslation(_dVars.moveAreaCenter.GetTranslation() + translationDiff * newCenterRatio);

    // The new radius is simply half the distance between the far side of the previus circle and the observed object
    _dVars.safeRadius = 0.5f * (distance + _dVars.safeRadius);

    PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cube", "New safe radius is %fmm", _dVars.safeRadius);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::UpdateSafeRegionForCliff(const Pose3d& cliffPose)
{
  Vec3f translationDiff = cliffPose.GetTranslation() - _dVars.moveAreaCenter.GetTranslation();
  // We're only going to care about the XY plane distance
  translationDiff.z() = 0;
  const f32 distanceSqr = translationDiff.LengthSq();

  // if the cliff is inside our safe radius, we need to update the safe region
  if (_dVars.safeRadius * _dVars.safeRadius > distanceSqr)
  {
    const f32 distance = std::sqrt(distanceSqr);


    // see if we can just shrink it, but never shrink smaller than the original size
    if( distance > kDefaultSafeRadius ) {
      PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cliff.ShrinkR",
                        "new safe radius = %fmm",
                        kDefaultSafeRadius);
      _dVars.safeRadius = kDefaultSafeRadius;
    }
    else {
      // otherwise we need to move the safe radius. Use the angle in the pose, since this is the angle at
      // which the robot approached the cliff, so moving in the opposite of that direction seems like a good
      // idea. Move the safe region so that the radius is kDefaultSafeRadius and the center is moved backwards
      // enough so that the radius just touches the the cliff pose

      // NOTE: assume everything is in the xy plane

      // You can see a crappy little python script to plot this stuff in python/behaviors/lookAroundSafeRegion.py

      // imagine centering the world at the cliff (and rotating as well). Take the current safe region center
      // point and move backwards along the direction of the cliff for distance "d" to find a new center such
      // that the distance between C1 and the cliff is kDefaultSafeRadius. This means that, in cliff coordinates:
      // x^2 + (y + d)^2 = kDefaultSafeRadius^2
      // solve the above for d (quadratic equation) and you get the code below
      float dx = _dVars.moveAreaCenter.GetTranslation().x() - cliffPose.GetTranslation().x();
      float dy = _dVars.moveAreaCenter.GetTranslation().y() - cliffPose.GetTranslation().y();
      float cliffTheta = cliffPose.GetRotationAngle<'Z'>().ToFloat();
      float x = dx * std::cos(cliffTheta) - dy * std::sin(cliffTheta);
      float y = dx * std::sin(cliffTheta) + dy * std::cos(cliffTheta);

      float op = std::pow(y,2) - (std::pow(x,2) + std::pow(y,2) - std::pow(kDefaultSafeRadius,2));
      if( op < 0.0f ) {
        PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cliff.Failure.NegativeSqrt",
                          "Operand is negative (%f), not updating safe region",
                          op);
        return;
      }
      float sqrtOp = std::sqrt(op);
      float ans1 = -y + sqrtOp;
      float ans2 = -y - sqrtOp;

      float shiftDist;

      if( ans1 > 0.0f && ans2 > 0.0f ) {
        shiftDist = std::min(ans1, ans2);
      }
      else if( ans1 > 0.0f ) {
        shiftDist = ans1;
      }
      else if( ans2 > 0.0f ) {
        shiftDist = ans2;
      }
      else {
        PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cliff.Failure.NoPositiveAnswer",
                          "answers %f and %f both negative, not updating safe region",
                          ans1,
                          ans2);
        return;
      }

      if( shiftDist > kMaxCliffShiftDist ) {
        PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cliff.ShiftTooFar",
                          "Computed that we should shift %fmm, which is > %f, so clipping",
                          shiftDist,
                          kMaxCliffShiftDist);
        shiftDist = kMaxCliffShiftDist;
      }


      // PRINT_NAMED_DEBUG("DEBUG.SafeRegion",
      //                   "c0 = [%f, %f], r=%f, cliff = [%f,%f], theta = %f",
      //                   _dVars.moveAreaCenter.GetTranslation().x(), _dVars.moveAreaCenter.GetTranslation().y(),
      //                   kDefaultSafeRadius,
      //                   cliffPose.GetTranslation().x(), cliffPose.GetTranslation().y(),
      //                   cliffTheta);

      PRINT_NAMED_DEBUG("BehaviorLookAround.UpdateSafeRegion.Cliff",
                        "moving center by %fmm and resetting radius",
                        shiftDist);
      
      Vec3f newTranslation( _dVars.moveAreaCenter.GetTranslation() );
      newTranslation.x() -= shiftDist * std::cos( cliffTheta );
      newTranslation.y() -= shiftDist * std::sin( cliffTheta );
      newTranslation.z() = 0.0f;
      
      _dVars.moveAreaCenter.SetTranslation(newTranslation);
      _dVars.safeRadius = kDefaultSafeRadius;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookAround::Destination BehaviorLookAround::GetNextDestination(BehaviorLookAround::Destination current)
{
  static BehaviorLookAround::Destination previous = BehaviorLookAround::Destination::Center;
  
  // If we've visited enough destinations, go back to center
  if (_dVars.numDestinationsLeft <= 1)
  {
    _dVars.numDestinationsLeft = 0;
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

  _dVars.numDestinationsLeft--;

  PRINT_NAMED_DEBUG("BehaviorLookAround.GetNextDestination", "%s (%d left)",
                    DestinationToString(*newDestIter),
                    _dVars.numDestinationsLeft);
  
  return *newDestIter;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::HandleRobotOfftreadsStateChanged(const EngineToGameEvent& event)
{
  ResetSafeRegion();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::HandleCliffEvent(const EngineToGameEvent& event)
{
  if( event.GetData().Get_CliffEvent().detectedFlags != 0 ) {
    // consider this location an obstacle
    UpdateSafeRegionForCliff(GetBEI().GetRobotInfo().GetPose());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::ResetSafeRegion()
{
  _dVars.moveAreaCenter = GetBEI().GetRobotInfo().GetPose();
  _dVars.safeRadius = kDefaultSafeRadius;
  PRINT_NAMED_DEBUG("BehaviorLookAround.ResetSafeRegion", "safe region reset");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookAround::SetState_internal(State state, const std::string& stateName)
{
  _dVars.currentState = state;
  SetDebugStateName(stateName);
}


} // namespace Cozmo
} // namespace Anki
