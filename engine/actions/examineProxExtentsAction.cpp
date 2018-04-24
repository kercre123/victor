/**
 * File: examineProxExtentsAction.cpp
 *
 * Author: ross
 * Date:   May 2 2018
 *
 * Description: Action that rotates the robot when near an obstacle to help sense the object's size with the prox sensor.
 *              If the action is started and there is no obstacle in front, it will end (successfully).
 *              The action then rotates in some config-driven combination of left/right/center, stopping
 *              rotation and moving to the next stage whenever the prox reading is large enough (config)
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/actions/examineProxExtentsAction.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
ExamineProxExtentsAction::ExamineProxExtentsAction( Ordering order, const Radians& minAngle, const Radians& maxAngle, u16 freeDistance_mm )
  : IAction( "ExamineProxExtentsAction",
             RobotActionType::EXAMINE_PROX_EXTENTS,
             (u8)AnimTrackFlag::NO_TRACKS )
  , _order( order )
  , _minAngle( minAngle )
  , _maxAngle( maxAngle )
  , _distance_mm( freeDistance_mm )
{
  
}
  
ExamineProxExtentsAction::~ExamineProxExtentsAction()
{
  if( _subAction != nullptr ) {
    _subAction->PrepForCompletion();
  }
}

ActionResult ExamineProxExtentsAction::Init()
{
  _initialAngle = GetRobot().GetPose().GetRotationAngle<'Z'>();
  
  if( _order == Ordering::RandomRandomCenter ) {
    if( GetRobot().GetRNG().RandDblInRange(0.0,1.0) < 0.5 ) {
      _order = Ordering::LeftRightCenter;
    } else {
      _order = Ordering::RightLeftCenter;
    }
  }
  _subAction.reset( new CompoundActionSequential() );
  _subAction->SetRobot( &GetRobot() );
  
  const bool absolute = true;
  // The first turn is relative, going at least _minAngle, and stopping when either the prox is free
  // or it hits _maxAngle, whichever happens sooner.
  // The second turn is absolute, so that the robot at least returns to the center and continues past it
  // by _minAngle before stopping when either the prox is free or it hits _maxAngle.
  // The third returns to center, ignoring the prox.
  if( _order == Ordering::LeftRightCenter ) {
    _subAction->AddAction( new TurnUntilFreeAction( -_maxAngle, !absolute, _distance_mm, _minAngle ) );
    _subAction->AddAction( new TurnUntilFreeAction( _initialAngle + _maxAngle, absolute, _distance_mm, _initialAngle + _minAngle ) );
  } else if( _order == Ordering::RightLeftCenter ) {
    _subAction->AddAction( new TurnUntilFreeAction( _maxAngle, !absolute, _distance_mm, _minAngle ) );
    _subAction->AddAction( new TurnUntilFreeAction( _initialAngle - _maxAngle, absolute, _distance_mm, _initialAngle - _minAngle ) );
  } else {
    DEV_ASSERT( false, "ExamineProxExtentsAction.Init.InvalidOrder" );
  }
  // return to center
  _subAction->AddAction( new TurnInPlaceAction( _initialAngle.ToFloat(), absolute ) );
  
  return ActionResult::SUCCESS;
}

ActionResult ExamineProxExtentsAction::CheckIfDone()
{
  auto result = ActionResult::NOT_STARTED;
  
  if (_subAction != nullptr) {
    result = _subAction->Update();
  }
  
  return result;
}
  
  
TurnUntilFreeAction::TurnUntilFreeAction( const Radians& angle, bool isAbsoluteAngle, u16 freeDistance_mm, const Radians& minAngle )
  : IAction( "TurnUntilFreeAction",
             RobotActionType::TURN_UNTIL_FREE,
             (u8)AnimTrackFlag::NO_TRACKS )
  , _angle( angle )
  , _isAbsoluteAngle( isAbsoluteAngle )
  , _distance_mm( freeDistance_mm )
  , _minAngleForExit( minAngle.ToFloat() )
{
}
  
TurnUntilFreeAction::~TurnUntilFreeAction()
{
  if( _subAction != nullptr ) {
    _subAction->PrepForCompletion();
  }
}

ActionResult TurnUntilFreeAction::Init()
{
  _initialAngle = GetRobot().GetPose().GetRotationAngle<'Z'>();
  _subAction.reset( new CompoundActionSequential() );
  _subAction->SetRobot( &GetRobot() );
  
  if( IsLiftInFOV() ) {
    _subAction->AddAction( new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK) );
  }
  
  _subAction->AddAction( new TurnInPlaceAction( _angle.ToFloat(), _isAbsoluteAngle ) );
  
  return ActionResult::SUCCESS;
}

  
ActionResult TurnUntilFreeAction::CheckIfDone()
{
  auto result = ActionResult::NOT_STARTED;
  
  const Radians currAngle = GetRobot().GetPose().GetRotationAngle<'Z'>();
  
  bool turnedEnough;
  if( _isAbsoluteAngle ) {
    // If a non-zero argument was supplied, check for crossing the absolute angle _minAngleForExit
    turnedEnough = (_minAngleForExit != 0.0f)
                   && (std::signbit( (_minAngleForExit - currAngle).ToFloat() )
                       != std::signbit( (_minAngleForExit - _initialAngle).ToFloat() ));
  } else {
    // Must have turned by at least _minAngleForExit in abs val
    const f32 angleDiff = (_initialAngle - currAngle).ToFloat();
    turnedEnough = fabs(angleDiff) >= fabs(_minAngleForExit);
  }
  
  if( IsFree() && !IsLiftInFOV() && turnedEnough ) {
    result = ActionResult::SUCCESS;
  } else {
    
    // Tick the action (if needed):
    if (_subAction != nullptr) {
      result = _subAction->Update();
      
      if( result != ActionResult::RUNNING && IsLiftInFOV() ) {
        result = ActionResult::LIFT_BLOCKING_PROX;
      }
    }
  }
  
  return result;
}
  
bool TurnUntilFreeAction::IsLiftInFOV() const
{
  bool isLiftInFOV = false;
  const auto& proxSensor = GetRobot().GetProxSensorComponent();
  u16 distance_mm = 0;
  const bool isSensorReadingValid = proxSensor.GetLatestDistance_mm( distance_mm );
  if( !isSensorReadingValid ) {
    isLiftInFOV = proxSensor.IsLiftInFOV();
  }
  return isLiftInFOV;
}
  
bool TurnUntilFreeAction::IsFree() const
{
  const auto* memoryMap = GetRobot().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorExploringExamineObstacle.RobotPathFreeOfObstacle.NeedMemoryMap" );
  
  const Pose3d currRobotPose = GetRobot().GetPose();
  
  // Line segment (robot position, p2) extending _distance_mm past robot
  Vec3f offset( _distance_mm, 0.0f, 0.0f );
  Rotation3d rot = Rotation3d(0.f, Z_AXIS_3D());
  const Point2f p2 = (currRobotPose.GetTransform() * Transform3d(rot, offset)).GetTranslation();
  
  const bool hasCollision = memoryMap->HasCollisionWithTypes( {{Point2f{currRobotPose.GetTranslation()}, p2}},
                                                              MemoryMapTypes::kTypesThatAreObstacles );
  return !hasCollision;
}
    

  
}
}

