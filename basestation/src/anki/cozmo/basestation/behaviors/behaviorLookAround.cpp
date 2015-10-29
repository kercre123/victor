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

#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/common/shared/radians.h"
#include "anki/common/robot/config.h"
#include "clad/externalInterface/messageEngineToGame.h"


#define SAFE_ZONE_VIZ (ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS)

#if SAFE_ZONE_VIZ
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/common/basestation/math/polygon_impl.h"
#endif

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

BehaviorLookAround::BehaviorLookAround(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _moveAreaCenter(robot.GetPose())
{
  _name = "LookAround";
  
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotPutDown
  });

   // Boredom and loneliness -> LookAround
  AddEmotionScorer(EmotionScorer(EmotionType::Excited,    Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.5f}}), false));
  AddEmotionScorer(EmotionScorer(EmotionType::Socialized, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.5f}}), false));
}
  
BehaviorLookAround::~BehaviorLookAround()
{
  /* Necessary? (We have no robot available in the destructor!)
  if (_currentState != State::Inactive)
  {
    ResetBehavior(0);
  }
   */
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
    case State::StartLooking:
    case State::LookingForObject:
    case State::ExamineFoundObject:
    case State::WaitToFinishExamining:
    {
      return true;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.IsRunnable.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  return false;
}

void BehaviorLookAround::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject:
      HandleObjectObserved(event, robot);
      break;
      
    case EngineToGameTag::RobotCompletedAction:
      HandleCompletedAction(event);
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
  // Update explorable area center to current robot pose
  ResetSafeRegion(robot);
  
  _currentState = State::StartLooking;
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorLookAround::UpdateInternal(Robot& robot, double currentTime_sec)
{
#if SAFE_ZONE_VIZ
  Point2f center = { _moveAreaCenter.GetTranslation().x(), _moveAreaCenter.GetTranslation().y() };
  VizManager::getInstance()->DrawXYCircle(robot.GetID(), ::Anki::NamedColors::GREEN, center, _safeRadius);
#endif
  switch (_currentState)
  {
    case State::Inactive:
    {
      // This is the last update before we stop running, so store off time
      _lastLookAroundTime = currentTime_sec;
      
      // Reset our number of destinations for next time we run this behavior
      _numDestinationsLeft = kDestinationsToReach;
      
      break; // Jump down and return Status::Complete
    }
    case State::StartLooking:
    {
      IActionRunner* moveHeadAction = new MoveHeadToAngleAction(0);
      robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveHeadAction);
      IActionRunner* moveLiftAction = new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK);
      robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveLiftAction);
      if (StartMoving(robot) == RESULT_OK) {
        _currentState = State::LookingForObject;
      }
    }
    // NOTE INTENTIONAL FALLTHROUGH
    case State::LookingForObject:
    {
      if (0 < _recentObjects.size())
      {
        _currentState = State::ExamineFoundObject;
      }
      return Status::Running;
    }
    case State::ExamineFoundObject:
    {
      robot.GetActionList().Cancel();
      bool queuedFaceObjectAction = false;
      while (!_recentObjects.empty())
      {
        auto iter = _recentObjects.begin();
        ObjectID objID = *iter;
        IActionRunner* faceObjectAction = new FaceObjectAction(objID, Vision::Marker::ANY_CODE, DEG_TO_RAD(2), DEG_TO_RAD(1440), false, true);
        
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, faceObjectAction);
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, new MoveLiftToHeightAction(LIFT_HEIGHT_LOWDOCK));
        queuedFaceObjectAction = true;
        
        ++_numObjectsToLookAt;
        _oldBoringObjects.insert(objID);
        _recentObjects.erase(iter);
      }
      
      // If we queued up some face object actions, add a move head action at the end to go back to normal
      if (queuedFaceObjectAction)
      {
        IActionRunner* moveHeadAction = new MoveHeadToAngleAction(0);
        robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, moveHeadAction);
      }
      _currentState = State::WaitToFinishExamining;
      
      return Status::Running;
    }
    case State::WaitToFinishExamining:
    {
      if (0 == _numObjectsToLookAt)
      {
        _currentState = State::StartLooking;
      }
      return Status::Running;
    }
    default:
    {
      PRINT_NAMED_ERROR("LookAround_Behavior.Update.UnknownState",
                        "Reached unknown state %d.", _currentState);
    }
  }
  
#if SAFE_ZONE_VIZ
  VizManager::getInstance()->EraseCircle(robot.GetID());
#endif
  
  return Status::Complete;
}
  
Result BehaviorLookAround::StartMoving(Robot& robot)
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
      PRINT_NAMED_WARNING("BehaviorLookAround.StartMoving.NoDestPoseFound", "attempts %d", MAX_NUM_CONSIDERED_DEST_POSES);
      
      // Try another destination
      _currentDestination = GetNextDestination(_currentDestination);
      
      // Kinda hacky, but if we're on the last destination to go to, just quit because we're
      // it always returns Center on the last destination and if we couldn't get there in
      // in this loop it probably won't happen in the next.
      if (_numDestinationsLeft == 1) {
        _currentState = State::Inactive;
      }
      
      return RESULT_FAIL;
    }
  }
  
  
  IActionRunner* goToPoseAction = new DriveToPoseAction(destPose, false, false);
  _currentDriveActionID = goToPoseAction->GetTag();
  robot.GetActionList().QueueActionAtEnd(IBehavior::sActionSlot, goToPoseAction, 3);
  return RESULT_OK;
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
  
void BehaviorLookAround::ResetBehavior(Robot& robot, float currentTime_sec)
{
  _lastLookAroundTime = currentTime_sec;
  _currentState = State::Inactive;
  robot.GetMoveComponent().StopAllMotors();
  robot.GetActionList().Cancel();
  _recentObjects.clear();
  _oldBoringObjects.clear();
  _numObjectsToLookAt = 0;
}
  
void BehaviorLookAround::HandleObjectObserved(const EngineToGameEvent& event, Robot& robot)
{
  assert(IsRunning());
  
  const RobotObservedObject& msg = event.GetData().Get_RobotObservedObject();
  
  static const std::set<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };
  
  // We'll get continuous updates about objects in view, so only care about new ones whose markers we can see
  if (familyList.count(msg.objectFamily) > 0 && msg.markersVisible && 0 == _oldBoringObjects.count(msg.objectID))
  {
    _recentObjects.insert(msg.objectID);
    
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
  
void BehaviorLookAround::HandleCompletedAction(const EngineToGameEvent& event)
{
  assert(IsRunning());
  
  const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
  if (RobotActionType::FACE_OBJECT == msg.actionType)
  {
    if (0 == _numObjectsToLookAt)
    {
      PRINT_NAMED_WARNING("BehaviorLookAround.HandleCompletedAction",
                          "Getting unexpected FaceObjectAction completion messages");
    }
    else
    {
      --_numObjectsToLookAt;
    }
  }
  else if (msg.idTag == _currentDriveActionID)
  {
    // If we're back to center, time to go inactive
    if (Destination::Center == _currentDestination)
    {
      _currentState = State::Inactive;
    }
    else
    {
      _currentState = State::StartLooking;
    }
    
    if (_numDestinationsLeft > 0)
    {
      --_numDestinationsLeft;
    }
    else
    {
      PRINT_NAMED_WARNING("BehaviorLookAround.HandleCompletedAction", "Getting unexpected DriveAction completion messages");
    }
    
    // If this was a successful drive action, move on to the next destination
    _currentDestination = GetNextDestination(_currentDestination);
  }
}
  
BehaviorLookAround::Destination BehaviorLookAround::GetNextDestination(BehaviorLookAround::Destination current)
{
  static BehaviorLookAround::Destination previous = BehaviorLookAround::Destination::Center;
  
  // If we've visited enough destinations, go back to center
  if (1 == _numDestinationsLeft)
  {
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

} // namespace Cozmo
} // namespace Anki
