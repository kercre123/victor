/**
 * File: behaviorVisitInterestingEdge
 *
 * Author: Raul
 * Created: 07/19/16
 *
 * Description: Behavior to visit interesting edges from the memory map. Some decision on whether we want to
 * visit any edges found there can be done by the behavior.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorVisitInterestingEdge.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTrigger.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
// kVie_MoveActionRetries: should probably not be in json, since it's not subject to gameplay tweaks
CONSOLE_VAR(uint8_t, kVie_MoveActionRetries, "BehaviorVisitInterestingEdge", 3);
// kVieDrawDebugInfo: Debug. If set to true the behavior renders debug privimitives
CONSOLE_VAR(bool, kVieDrawDebugInfo, "BehaviorVisitInterestingEdge", true);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// This is the configuration of memory map types that this behavior checks against. If a type
// is set to true, it means that if they are a border to an interesting edge, we will explore that interesting edge
constexpr NavMemoryMapTypes::FullContentArray typesToExploreFrom =
{
  {NavMemoryMapTypes::EContentType::Unknown               , true },
  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , true },
  {NavMemoryMapTypes::EContentType::ClearOfCliff          , true },
  {NavMemoryMapTypes::EContentType::ObstacleCube          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {NavMemoryMapTypes::EContentType::ObstacleCharger       , false},
  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
  {NavMemoryMapTypes::EContentType::Cliff                 , false},
  {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
};
static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesToExploreFrom),
  "This array does not define all types once and only once.");

// This is the configuration of memory map types that would invalidate goals because we would
// need to cross an obstacle or edge to get there
constexpr NavMemoryMapTypes::FullContentArray typesThatInvalidateGoals =
{
  {NavMemoryMapTypes::EContentType::Unknown               , false},
  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCube          , false}, // this could be ok, since we will walk around them
  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {NavMemoryMapTypes::EContentType::ObstacleCharger       , false}, // this could be ok, since we will walk around the charger
  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
  {NavMemoryMapTypes::EContentType::Cliff                 , true},
  {NavMemoryMapTypes::EContentType::InterestingEdge       , false}, // the goal itself is the closest one, so we can afford not to do this (which simplifies goal point)
  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , true}
};
static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesThatInvalidateGoals),
  "This array does not define all types once and only once.");
  
  
// This is the configuration of memory map types that would invalidate vantage points because an obstacle would
// block the point or another edge would present a problem
constexpr NavMemoryMapTypes::FullContentArray typesThatInvalidateVantagePoints =
{
  {NavMemoryMapTypes::EContentType::Unknown               , false},
  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCube          , true},
  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {NavMemoryMapTypes::EContentType::ObstacleCharger       , true},
  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
  {NavMemoryMapTypes::EContentType::Cliff                 , true},
  {NavMemoryMapTypes::EContentType::InterestingEdge       , true},
  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , true}
};
static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesThatInvalidateVantagePoints),
  "This array does not define all types once and only once.");
  
};

static const char* kConfigParamsKey = "params";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::BehaviorVisitInterestingEdge(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _configParams{}
, _operatingState(EOperatingState::Invalid)
{
  SetDefaultName("BehaviorVisitInterestingEdge");
  
  // load parameters from json
  LoadConfig(config[kConfigParamsKey]);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::~BehaviorVisitInterestingEdge()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVisitInterestingEdge::IsRunnableInternal(const Robot& robot) const
{
  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  if ( nullptr == memoryMap ) {
    return false;
  }
  
  const bool hasBorders = memoryMap->HasBorders(NavMemoryMapTypes::EContentType::InterestingEdge, typesToExploreFrom);
  return hasBorders;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorVisitInterestingEdge::InitInternal(Robot& robot)
{
  // select borders we want to visit
  BorderScore selectedGoal;
  PickBestGoal(robot, selectedGoal);
  
  if ( !selectedGoal.IsValid() ) {
    PRINT_NAMED_ERROR("BehaviorVisitInterestingEdge.InitInternal", "'%s' is runnable, but no goals were found",
      GetName().c_str());
    return RESULT_FAIL;
  }
  
  // validate whether the goal is reachable
  bool isGoalValid = true;
  // this is in place of the planner not getting us there (TODO)
  const bool allowGoalsBehindOthers = _configParams.allowGoalsBehindOtherEdges;
  if ( !allowGoalsBehindOthers )
  {
    const Vec3f& goalPoint = selectedGoal.borderInfo.GetCenter();
    isGoalValid = CheckGoalReachable(robot, goalPoint);
  }
  
  // calculate vantage points for the goal, only if it's valid/reachable
  _currentVantagePoints.clear();
  if ( isGoalValid )
  {
    // pick the lookAtPoint and vantage points for it
    const Vec3f& insideGoalDir = (-selectedGoal.borderInfo.normal);
    const Vec3f& lookAtPoint = selectedGoal.borderInfo.GetCenter() + (insideGoalDir * _configParams.distanceInsideGoalToLookAt_mm);
    _lookAtPoint = lookAtPoint;

    // pick a vantage point from where to look at the goal. Those are the points we will feed the planner
    GenerateVantagePoints(robot, selectedGoal, _lookAtPoint, _currentVantagePoints);
  }

  // we couldn't calculate vantage points for the selected goal, maybe the goal was in an unreachable place
  if ( _currentVantagePoints.empty() )
  {
    // flag this goal as not intereseting so that we don't try to visit it again
    const float noVantageGoalHalfQuadSideSize_mm = _configParams.noVantageGoalHalfQuadSideSize_mm;
    // grab goal's point and direction
    const Point3f& goalPoint  = selectedGoal.borderInfo.GetCenter();
    const Point3f& goalNormal = selectedGoal.borderInfo.normal;
    // flag as not good to visit
    FlagQuadAroundGoalAsNotInteresting(robot, goalPoint, goalNormal, noVantageGoalHalfQuadSideSize_mm);
    
    // log
    PRINT_CH_INFO("Behaviors", (GetName() + ".InitInternal").c_str(),
      "Goal at [%.2f,%.2f,%.2f] is not reachable, flagging as not interesting",
      goalPoint.x(), goalPoint.y(), goalPoint.z());

    // simulate that he is thinking and calculating stuff, since that's what he is doing
    const AnimationTrigger trigger = _configParams.goalDiscardedAnimTrigger;
    if ( trigger != AnimationTrigger::Count )
    {
      // play the animation that let's us know he is thinking and discarding goals
      IAction* discardedGoalAnimAction = new TriggerLiftSafeAnimationAction(robot, trigger);
      StartActing( discardedGoalAnimAction );
    }

    // we are going to discard goals
    _operatingState = EOperatingState::DiscardingGoals;
    
    // discarding goals doesn't have other states
    SetDebugStateName("DiscardingGoals");
  }
  else
  {
    // we are going to visit a goal
    _operatingState = EOperatingState::VisitingGoal;
  
    // we have a vantage point, go there now
    TransitionToS1_MoveToVantagePoint(robot, 0);
  }

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::StopInternal(Robot& robot)
{
  robot.GetContext()->GetVizManager()->EraseSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::BaseClass::Status BehaviorVisitInterestingEdge::UpdateInternal(Robot& robot)
{
  // try to discard a new goal per tick while discarding
  if ( _operatingState == EOperatingState::DiscardingGoals ) {
    const bool discardedOne = DiscardNextUnreachableGoal(robot);
    if ( !discardedOne ) {
      _operatingState = EOperatingState::DoneDiscarding;
    }
  }

  // delegate on parent for return value
  const BaseClass::Status baseRet = BaseClass::UpdateInternal(robot);
  return baseRet;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::PickBestGoal(Robot& robot, BorderScore& bestGoal) const
{
  bestGoal.Reset();

  // grab borders
  INavMemoryMap::BorderVector borders;
  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::InterestingEdge, typesToExploreFrom, borders);
  if ( !borders.empty() )
  {
    const Vec3f& robotLoc = robot.GetPose().GetWithRespectToOrigin().GetTranslation();
  
    for ( const auto& border : borders )
    {
      // debugging all borders
      if ( kVieDrawDebugInfo )
      {
        robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        border.from, border.to, Anki::NamedColors::YELLOW, false, 35.0f);
      }
      
      // compare goal to best so far
      const float curDistSQ = (border.GetCenter() - robotLoc).LengthSq();
      const bool isBetter = FLT_LT(curDistSQ, bestGoal.distanceSQ);
      if ( isBetter ) {
        // this is the new best
        bestGoal.Set(border, curDistSQ);
      }
    }
  }
  
  // debugging
  if ( kVieDrawDebugInfo )
  {
    const NavMemoryMapTypes::Border& b = bestGoal.borderInfo;
    robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      b.from, b.to, Anki::NamedColors::DARKGREEN, false, 15.0f);
    Vec3f centerLine = (b.from + b.to)*0.5f;
    robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      centerLine, centerLine+b.normal*15.0f, Anki::NamedColors::YELLOW, false, 45.0f);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVisitInterestingEdge::CheckGoalReachable(const Robot& robot, const Vec3f& goalPosition) const
{
  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  ASSERT_NAMED(nullptr != memoryMap, "BehaviorVisitInterestingEdge.CheckGoalReachable.NeedMemoryMap");
  
  const Vec3f& fromRobot = robot.GetPose().GetWithRespectToOrigin().GetTranslation();
  const Vec3f& toGoal    = goalPosition; // assumed wrt origin
  
  // unforunately the goal (border point) can be inside InterestingEdge; this happens for diagonal edges.
  // Fortunately, we currently only pick the closest goal, which means that if we cross an interesting edge
  // it has to be the one belonging to the goal itself. This allows setting that type to false, and proceed
  // to raycast towards the goal, rather than having to calculate the proper offset from the goal towards the
  // actual corner in the quad
  static_assert( typesThatInvalidateGoals[Util::EnumToUnderlying(NavMemoryMapTypes::EContentType::InterestingEdge)].Value() == false,
    "toGoal is inside an InterestingEdge. This type needs to be false for current implementation");
  const bool hasCollision = memoryMap->HasCollisionRayWithTypes(fromRobot, toGoal, typesThatInvalidateGoals);
  
  // debug render this ray
  if ( kVieDrawDebugInfo )
  {
    const ColorRGBA& color = hasCollision ? Anki::NamedColors::RED : Anki::NamedColors::GREEN;
    robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      fromRobot, toGoal, color, false, 20.0f);
  }
  
  // check result of collision test
  const bool isGoalReachable = !hasCollision;
  return isGoalReachable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::GenerateVantagePoints(Robot& robot, const BorderScore& goal, const Vec3f& lookAtPoint, VantagePointVector& outVantagePoints) const
{
  const Vec3f& kFwdVector = X_AXIS_3D();
  const Vec3f& kRightVector = -Y_AXIS_3D();
  const Vec3f& kUpVector = Z_AXIS_3D();

  const Pose3d* worldOrigin = robot.GetWorldOrigin();

  outVantagePoints.clear();
  {
    uint16_t totalRayTries = _configParams.vantagePointAngleOffsetTries * 2; // *2 because we do +-angle per try
    uint16_t rayIndex = 0;
    while ( rayIndex <= totalRayTries )
    {
      // calculate rotation offset
      const uint16_t offsetIndex = (rayIndex+1) / 2; // rayIndex(0) = offset(0), rayIndex(1,2) = offset(1), rayIndex(3,4) = offset(2), ...
      const float offsetSign = ((rayIndex%2) == 0) ? 1.0f : -1.0f; // change sign every rayIndex
      const float rotationOffset_deg = (offsetIndex * _configParams.vantagePointAngleOffsetPerTry_deg) * offsetSign;
      ASSERT_NAMED((rayIndex==0) || !FLT_NEAR(rotationOffset_deg, 0.0f), "BehaviorVisitInterestingEdge.GenerateVantagePoints.BadRayOffset");
      
      Vec3f normalFromLookAtTowardsVantage = goal.borderInfo.normal;
      // rotate by the offset of this try
      const bool hasRotation = (rayIndex != 0); // we assert that rayIndex!=0 has a valid offset_def
      if ( hasRotation ) {
        Radians rotationOffset_rad = DEG_TO_RAD(rotationOffset_deg);
        Rotation3d rotationToTry(rotationOffset_rad, kUpVector);
        normalFromLookAtTowardsVantage = rotationToTry * normalFromLookAtTowardsVantage;
      }

      // randomize distance for this ray
      const float distanceFromLookAtToVantage = Util::numeric_cast<float>( GetRNG().RandDblInRange(
        _configParams.distanceFromLookAtPointMin_mm, _configParams.distanceFromLookAtPointMax_mm));
      const Point3f vantagePointPos = lookAtPoint + (normalFromLookAtTowardsVantage * distanceFromLookAtToVantage);
      
      // check for collisions in the memory map from the goal, not from the lookAt point, since the lookAt
      // point is inside the border
      bool isValidVantagePoint = true;
      {
        // Implementation Note: it is possible that the border point be inside the InterestingEdge we want to visit (this
        // happens for diagonal borders). Casting the ray from there will indeed collide with that InterestingEdge itself.
        // The most accurate solution would be to ask the memory map for the intersection point of that quad with the ray
        // we are trying to cast, and use that intersection point (with a little margin outwards) as the toPoint for the
        // actual check. However that requires knowledge of how the memory map is divided into quads.
        // Fortunately we have a minimum distance away from the goal, and we would have to account for the front of the
        // robot to not step on edges, so we can cast the point from the front of the robot with a little offset
        // to give it jiggle room in case it needs to turn.
        
        // Alternatively, note that the the Border information provides From and To points, as well as the normal.
        // With that information, it should be trivial to calculate the corner of the quad (since From and To will have
        // different X and Y coordinates for diagonals. This however assumes the underlying structure, and hence I would
        // rather not use that extra knowledge here, unless necessary. (Maybe provide that point in INavMemoryMap API)
      
        // Note on clearance distances: since we generate the vantage point randomly and we check
        // for clearance after the random distance, there's a chance that we could have picked a closer or further distance
        // and not fail the check, but that the random distance fails. Not readjusting smartly instead of randomly
        // shouldn't be a big deal in the general case, and it simplifies logic here.
      
        // vars to calculate the collision ray
        const float robotFront = ROBOT_BOUNDING_X_FRONT + ROBOT_BOUNDING_X_LIFT; // consider lift too
        const float clearDistanceInFront = _configParams.additionalClearanceInFront_mm + robotFront;
        const float robotBack  = ROBOT_BOUNDING_X - ROBOT_BOUNDING_X_FRONT;
        const float clearDistanceBehind  = _configParams.additionalClearanceBehind_mm  + robotBack;
        const Vec3f& toPoint   = vantagePointPos - (normalFromLookAtTowardsVantage * clearDistanceInFront);
        const Vec3f& fromPoint = vantagePointPos + (normalFromLookAtTowardsVantage * clearDistanceBehind );
        const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
        assert(memoryMap); // otherwise we are not even runnable
        
        // the vantage point is valid if there's no collision with the invalid types (would block the view or the pose)
        const bool hasCollision = memoryMap->HasCollisionRayWithTypes(fromPoint, toPoint, typesThatInvalidateVantagePoints);
        isValidVantagePoint = !hasCollision;
        
        // debug render this ray
        if ( kVieDrawDebugInfo )
        {
          const float kUpLine_mm = 10.0f;
          const ColorRGBA& color = isValidVantagePoint ? Anki::NamedColors::GREEN : Anki::NamedColors::RED;
          robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            fromPoint, toPoint, color, false, 20.0f);
          robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            vantagePointPos-Vec3f{0,0,kUpLine_mm}, vantagePointPos+Vec3f{0,0,kUpLine_mm}, color, false, 20.0f);
        }
      }
      
      // if this vantage point is valid, add to the vector
      if ( isValidVantagePoint )
      {
        // generate a pose that looks at the LookAt point inside the border
        const Vec3f& vantagePointDir = -normalFromLookAtTowardsVantage;
        const float fwdAngleCos = DotProduct(vantagePointDir, kFwdVector);
        const bool isPositiveAngle = (DotProduct(vantagePointDir, kRightVector) >= 0.0f);
        float rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);

        // add pose to vector
        outVantagePoints.emplace_back(rotRads, kUpVector, vantagePointPos, worldOrigin);
        
        // we only need one vantage point, do not check more (optimization, because we could give the planner several)
        break;
      }
      ++rayIndex;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS1_MoveToVantagePoint(Robot& robot, uint8_t retries)
{
  SetDebugStateName("TransitionToS1_MoveToVantagePoint");
  ASSERT_NAMED( _operatingState == EOperatingState::VisitingGoal,
    "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.InvalidState" );
  
  PRINT_CH_INFO("Behaviors", (GetName() + ".S1").c_str(), "Moving to vantage point");
  
  // there have to be vantage points. If it's impossible to generate vantage points from the memory map,
  // we should change those borders/quads to "not visitable" to prevent failing multiple times
  ASSERT_NAMED(!_currentVantagePoints.empty(),
    "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.NoVantagePoints");
  
  // create compound action to force lift to be on low dock (just in case) and then move
  CompoundActionSequential* moveAction = new CompoundActionSequential(robot);

  // 1) make sure lift is down so that we can detect more edges as we move
  {
    IAction* moveLiftDownAction = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK);
    moveAction->AddAction( moveLiftDownAction );
  }
  
  // 2) now move to the vantage point
  {
    // todo rsam: what happens if we are already close to the pose? Check that the action will succeed
    // request the action
    DriveToPoseAction* driveToPoseAction = new DriveToPoseAction( robot, _currentVantagePoints );
    moveAction->AddAction( driveToPoseAction );
  }
  
  RobotCompletedActionCallback onActionResult = [this, &robot, retries](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    if ( actionRet.result == ActionResult::SUCCESS ) {
      // we got there, observe the border
      TransitionToS2_ObserveAtVantagePoint(robot);
    }
    else if (actionRet.result == ActionResult::FAILURE_RETRY)
    {
      // retry as long as we haven't run out of tries
      if ( retries < kVie_MoveActionRetries ) {
        PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailedRetry",
          "Trying again (%d)", retries+1);
        TransitionToS1_MoveToVantagePoint(robot, retries+1);
      } else {
        PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailedRetry",
          "Attempted to retry (%d) times. Bailing", retries);
      }
    } else {
      PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailed", "Unhandled result");
    }
  };
  
  // start moving, and react to action results
  StartActing(moveAction, onActionResult);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS2_ObserveAtVantagePoint(Robot& robot)
{
  SetDebugStateName("TransitionToS2_ObserveAtVantagePoint");
  ASSERT_NAMED( _operatingState == EOperatingState::VisitingGoal,
    "BehaviorVisitInterestingEdge.TransitionToS2_ObserveAtVantagePoint.InvalidState" );
  
  PRINT_CH_INFO("Behaviors", (GetName() + ".S2").c_str(), "Observing edges from vantage point");

  // ask blockworld to flag the interesting edges in front of Cozmo as noninteresting anymore
  const float halfWidthAtRobot_mm = _configParams.vantageConeHalfWidthAtRobot_mm;
  const float farPlaneDistFromLookAt_mm = _configParams.vantageConeFarPlaneDistFromLookAt_mm;
  const float halfWidthAtFarPlane_mm = _configParams.vantageConeHalfWidthAtFarPlane_mm;
  FlagVisitedQuadAsNotInteresting(robot, halfWidthAtRobot_mm, _lookAtPoint, farPlaneDistFromLookAt_mm, halfWidthAtFarPlane_mm);
  
  // play "i'm observing stuff" animation
  IAction* pauseAction = nullptr;
  
  const AnimationTrigger trigger = _configParams.observeEdgeAnimTrigger;
  if ( trigger != AnimationTrigger::Count )
  {
    pauseAction = new TriggerLiftSafeAnimationAction(robot,trigger);
  }
  else
  {
    // create wait action as fallback (should not happen since a proper anim should have been specified)
    const double waitTime_sec = 1.0f;
    pauseAction = new WaitAction( robot, waitTime_sec );
  }
  
  // request action with transition to proper state
  ASSERT_NAMED( nullptr!=pauseAction, "BehaviorExploreLookAroundInPlace::TransitionToS2_Pause.NullAction");
  StartActing( pauseAction );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVisitInterestingEdge::DiscardNextUnreachableGoal(Robot& robot) const
{
  // we only discard during discarding state
  ASSERT_NAMED( _operatingState == EOperatingState::DiscardingGoals,
    "BehaviorVisitInterestingEdge.DiscardNextUnreachableGoal.InvalidState" );

  // select the next potential goal (note that we would have already flagged the first one as not interesting,
  // so searching here again will return a different one)
  BorderScore potentialGoal;
  PickBestGoal(robot, potentialGoal);
  
  if ( !potentialGoal.IsValid() )
  {
    // log - no more goals available
    PRINT_CH_INFO("Behaviors", (GetName() + ".DiscardNextUnreachableGoal").c_str(),
      "No more goals available, done discarding" );
    return false;
  }
  
  // check if the goal is reachable
  bool isGoalReachable = true;
  // this is in place of the planner not getting us there (TODO)
  const bool allowGoalsBehindOthers = _configParams.allowGoalsBehindOtherEdges;
  if ( !allowGoalsBehindOthers )
  {
    const Vec3f& goalPoint = potentialGoal.borderInfo.GetCenter();
    isGoalReachable = CheckGoalReachable(robot, goalPoint);
  }
  
  // calculate vantage points for the goal, only if it's valid/reachable
  VantagePointVector potentialVantagePoints;
  if ( isGoalReachable )
  {
    // pick the lookAtPoint and vantage points for it
    const Vec3f& insideGoalDir = (-potentialGoal.borderInfo.normal);
    const Vec3f& potentialLookAtPoint = potentialGoal.borderInfo.GetCenter() + (insideGoalDir * _configParams.distanceInsideGoalToLookAt_mm);

    // pick a vantage point from where to look at the goal. Those are the points we will feed the planner
    GenerateVantagePoints(robot, potentialGoal, potentialLookAtPoint, potentialVantagePoints);
  }

  // we couldn't calculate vantage points for the selected goal, maybe the goal was in an unreachable place
  if ( potentialVantagePoints.empty() )
  {
    // also flag this goal as not interesting
    const float noVantageGoalHalfQuadSideSize_mm = _configParams.noVantageGoalHalfQuadSideSize_mm;
    // grab goal's point and direction
    const Point3f& goalPoint  = potentialGoal.borderInfo.GetCenter();
    const Point3f& goalNormal = potentialGoal.borderInfo.normal;
    // flag as not good to visit
    FlagQuadAroundGoalAsNotInteresting(robot, goalPoint, goalNormal, noVantageGoalHalfQuadSideSize_mm);
    
    // log
    PRINT_CH_INFO("Behaviors", (GetName() + ".DiscardNextUnreachableGoal.Discarded").c_str(),
      "Goal at [%.2f,%.2f,%.2f] is not reachable, flagging as not interesting",
      goalPoint.x(), goalPoint.y(), goalPoint.z());

    // we discarded one, keep discarding later
    return true;
  }
  
  // the goal and some vantage points are actually reachable, so we should stop discarding now
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::FlagVisitedQuadAsNotInteresting(Robot& robot, float halfWidthAtRobot_mm,
  const Vec3f& lookAtPoint, float farPlaneDistFromGoal_mm, float halfWidthAtFarPlane_mm)
{
  const Pose3d& robotPoseWrtOrigin = robot.GetPose().GetWithRespectToOrigin();
  
  // bottom corners of the quad are based on the robot pose
  const Point3f& cornerBL = robotPoseWrtOrigin * Vec3f{ 0.f, +halfWidthAtRobot_mm, 0.f};
  const Point3f& cornerBR = robotPoseWrtOrigin * Vec3f{ 0.f, -halfWidthAtRobot_mm, 0.f};
  
  // top corners of the quad are based on the far plane
  Vec3f dirRobotToLookAt = (lookAtPoint - robotPoseWrtOrigin.GetTranslation());
  dirRobotToLookAt.MakeUnitLength();
  const Vec3f& farPlanePos = lookAtPoint + dirRobotToLookAt * farPlaneDistFromGoal_mm;
  const Point3f& cornerTL = farPlanePos + robotPoseWrtOrigin.GetRotation() * Vec3f{ 0.f, +halfWidthAtFarPlane_mm, 0.f};
  const Point3f& cornerTR = farPlanePos + robotPoseWrtOrigin.GetRotation() * Vec3f{ 0.f, -halfWidthAtFarPlane_mm, 0.f};

  const Quad2f robotToFarPlaneQuad{ cornerTL, cornerBL, cornerTR, cornerBR };
  robot.GetBlockWorld().FlagQuadAsNotInterestingEdges(robotToFarPlaneQuad);

  // render the quad we are flagging as not interesting anymore
  if ( kVieDrawDebugInfo ) {
    robot.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      robotToFarPlaneQuad, 32.0f, Anki::NamedColors::BLUE, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::FlagQuadAroundGoalAsNotInteresting(Robot& robot, const Vec3f& goalPoint, const Vec3f& goalNormal, float halfQuadSideSize_mm)
{
  ASSERT_NAMED( FLT_NEAR(goalNormal.z(), 0.0f), "BehaviorVisitInterestingEdge.FlagQuadAroundGoalAsNotInteresting.MemoryMapIs2DAtTheMoment");
  const Vec3f rightNormal{goalNormal.y(), -goalNormal.x(), goalNormal.z()}; // 2d perpendicular to the right
  const Vec3f forwardDir = goalNormal  * halfQuadSideSize_mm;
  const Vec3f rightDir   = rightNormal * halfQuadSideSize_mm;
  
  // corners of the quad are centered around goalPoint
  const Point3f& cornerBL = goalPoint - forwardDir - rightDir;
  const Point3f& cornerBR = goalPoint - forwardDir + rightDir;
  const Point3f& cornerTL = goalPoint + forwardDir - rightDir;
  const Point3f& cornerTR = goalPoint + forwardDir + rightDir;

  // creat
  const Quad2f quadAroundGoal{ cornerTL, cornerBL, cornerTR, cornerBR };
  robot.GetBlockWorld().FlagQuadAsNotInterestingEdges(quadAroundGoal);
  
  // render the quad we are flagging as not interesting beacuse
  if ( kVieDrawDebugInfo ) {
    robot.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      quadAroundGoal, 32.0f, Anki::NamedColors::BLUE, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetName() + ".BehaviorVisitInterestingEdge.LoadConfig";

  // observeEdge animation
  const std::string& obsAnimTriggerName = ParseString(config, "observeEdgeAnimTrigger", debugName);
  _configParams.observeEdgeAnimTrigger = obsAnimTriggerName.empty() ?
    AnimationTrigger::Count :
    AnimationTriggerFromString(obsAnimTriggerName.c_str());
  // goalDiscarded animation
  const std::string& gdAnimTriggerName = ParseString(config, "goalDiscardedAnimTrigger", debugName);
  _configParams.goalDiscardedAnimTrigger = gdAnimTriggerName.empty() ?
    AnimationTrigger::Count :
    AnimationTriggerFromString(gdAnimTriggerName.c_str());
  // gameplay params
  _configParams.allowGoalsBehindOtherEdges = ParseBool(config, "allowGoalsBehindOtherEdges", debugName);
  _configParams.distanceFromLookAtPointMin_mm = ParseFloat(config, "distanceFromLookAtPointMin_mm", debugName);
  _configParams.distanceFromLookAtPointMax_mm = ParseFloat(config, "distanceFromLookAtPointMax_mm", debugName);
  _configParams.distanceInsideGoalToLookAt_mm = ParseFloat(config, "distanceInsideGoalToLookAt_mm", debugName);
  _configParams.additionalClearanceInFront_mm = ParseFloat(config, "additionalClearanceInFront_mm", debugName);
  _configParams.additionalClearanceBehind_mm  = ParseFloat(config, "additionalClearanceBehind_mm", debugName);
  _configParams.vantagePointAngleOffsetPerTry_deg = ParseFloat(config, "vantagePointAngleOffsetPerTry_deg", debugName);
  _configParams.vantagePointAngleOffsetTries = ParseUint8(config, "vantagePointAngleOffsetTries", debugName);
  _configParams.vantageConeHalfWidthAtRobot_mm = ParseFloat(config, "vantageConeHalfWidthAtRobot_mm", debugName);
  _configParams.vantageConeFarPlaneDistFromLookAt_mm = ParseFloat(config, "vantageConeFarPlaneDistFromLookAt_mm", debugName);
  _configParams.vantageConeHalfWidthAtFarPlane_mm = ParseFloat(config, "vantageConeHalfWidthAtFarPlane_mm", debugName);
  
  // optional
  GetValueOptional(config, "noVantageGoalHalfQuadSideSize_mm", _configParams.noVantageGoalHalfQuadSideSize_mm);
  
  // validate
  ASSERT_NAMED( FLT_GT(_configParams.noVantageGoalHalfQuadSideSize_mm, 0.0f),
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidNoVantageGoalHalfQuadSideSize_mm");
  ASSERT_NAMED( _configParams.observeEdgeAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidObserveEdgeAnimTrigger");
  ASSERT_NAMED( _configParams.goalDiscardedAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidGGoalDiscardedAnimTrigger");
  ASSERT_NAMED( FLT_LE(_configParams.distanceFromLookAtPointMin_mm,_configParams.distanceFromLookAtPointMax_mm),
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidDistanceFromGoalRange");
  ASSERT_NAMED( (_configParams.vantagePointAngleOffsetTries == 0) || FLT_GT(_configParams.vantagePointAngleOffsetPerTry_deg,0.0f),
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidOffsetConfiguration");
}

} // namespace Cozmo
} // namespace Anki
