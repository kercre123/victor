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
  
// TODO RSAM json config
CONSOLE_VAR(uint8_t, kVieMaxBorderGoals, "BehaviorVisitInterestingEdge", 1); // max number of goals to ask the planner
CONSOLE_VAR(float, kVie_MoveActionRetries, "BehaviorVisitInterestingEdge", 3);
CONSOLE_VAR(float, kVie_DistanceFromEdgeMin_mm, "BehaviorVisitInterestingEdge",  80.0f);
CONSOLE_VAR(float, kVie_DistanceFromEdgeMax_mm, "BehaviorVisitInterestingEdge", 120.0f);
CONSOLE_VAR(bool, kVieDrawDebugInfo, "BehaviorVisitInterestingEdge", true); // if set to true the behavior renders debug privimitives

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
  
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::BehaviorVisitInterestingEdge(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorVisitInterestingEdge");
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
  BorderScoreVector borderGoals;
  PickGoals(robot, borderGoals);

  // cache the goal pose so that we know what we are looking at from the vantage point.
  // Note: currently we support only one goal. If we want to support multiple, we need to know which vantage point
  // the planner took us to, and which goal was associated to that vantage point. It's doable, just don't wanna support
  // that now (rsam 07/20/16)
  if ( !borderGoals.empty() ) {
    ASSERT_NAMED(borderGoals.size() == 1,
      "BehaviorVisitInterestingEdge::GenerateVantagePoints.MultipleGoalsAreNotSupported");
    // store the pose wrt current origin
    const Vec3f& kUpVector = Z_AXIS_3D();
    const Point3f& goalPos = borderGoals[0].borderInfo.GetCenter();
    _goalPose = Pose3d(Rotation3d(0, kUpVector), goalPos, robot.GetWorldOrigin(), "InterestingEdgeGoalPose");
  }

  // for every goal, pick a target point to look at the cube from there. Those are the goals we will feed the planner
  GenerateVantagePoints(robot, borderGoals, _currentVantagePoints);

  // debugging
  if ( kVieDrawDebugInfo )
  {
    // border goals
    for ( const auto& bG : borderGoals )
    {
      const NavMemoryMapTypes::Border& b = bG.borderInfo;
      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        b.from, b.to, Anki::NamedColors::DARKGREEN, false, 15.0f);
      Vec3f centerLine = (b.from + b.to)*0.5f;
      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        centerLine, centerLine+b.normal*15.0f, Anki::NamedColors::GREEN, false, 60.0f);
    }

    // vantage points
    for ( const auto& p : _currentVantagePoints )
    {
      Point3f testOrigin{};
      Point3f testEnd = X_AXIS_3D() * 15.0f;
      testOrigin = p * testOrigin;
      testEnd = p * testEnd;
      robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        testOrigin, testEnd, Anki::NamedColors::CYAN, false, 15.0f);
    }
  }

  // we shouldn't be running if we don't have borders/vantage points
  if ( _currentVantagePoints.empty() ) {
    PRINT_NAMED_ERROR("BehaviorVisitInterestingEdge::InitInternal", "Could not calculate vantage points from borders!");
    return RESULT_FAIL;
  }
  
  // move to the vantage point
  TransitionToS1_MoveToVantagePoint(robot, 0);

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::StopInternal(Robot& robot)
{
  robot.GetContext()->GetVizManager()->EraseSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::PickGoals(Robot& robot, BorderScoreVector& outGoals) const
{
  outGoals.clear();

  // grab borders
  INavMemoryMap::BorderVector borders;
  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::InterestingEdge, typesToExploreFrom, borders);
  if ( !borders.empty() )
  {
    outGoals.reserve(kVieMaxBorderGoals);
    for ( const auto& border : borders )
    {
      // debugging all borders
      if ( kVieDrawDebugInfo )
      {
        robot.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        border.from, border.to, Anki::NamedColors::YELLOW, false, 15.0f);
      }
      
      // find where this goal would go with respect to the rest
      // note we compare the center of the border, not the destination point we will choose to go to
      const float curDistSQ = (border.GetCenter() - robot.GetPose().GetWithRespectToOrigin().GetTranslation()).LengthSq();
      BorderScoreVector::iterator goalIt = outGoals.begin();
      while( goalIt != outGoals.end() )
      {
        if ( curDistSQ < goalIt->distanceSQ ) {
          break;
        }
        ++goalIt;
      }
      
      // this border should be in the goals, if it's better or if we don't have the max
      if ( (goalIt != outGoals.end()) || (outGoals.size() < kVieMaxBorderGoals) )
      {
        // if we have the max already, we need to kick out the last one
        if ( outGoals.size() >= kVieMaxBorderGoals )
        {
          // kicking out the last one invalidates iterators pointing at it, check if we were pointing at last
          bool newIsLast = (goalIt == outGoals.end()-1);
          // kick out last one
          outGoals.pop_back();
          // if we were pointing at the last one, grab end() now, since we destroyed the last one
          if ( newIsLast ) {
            goalIt = outGoals.end();
          }
        }

        // add this goal where it should go within the vector (sorted by distance)
        outGoals.emplace(goalIt, border, curDistSQ);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::GenerateVantagePoints(Robot& robot, const BorderScoreVector& goals, VantagePointVector& outVantagePoints) const
{
  const Pose3d* worldOrigin = robot.GetWorldOrigin();

  outVantagePoints.clear();
  for ( const auto& goal : goals )
  {
    // randomize distance for this goal
    const float distanceFromGoal = Util::numeric_cast<float>( GetRNG().RandDblInRange(
      kVie_DistanceFromEdgeMin_mm, kVie_DistanceFromEdgeMax_mm));
    
    const Point3f vantagePointPos = goal.borderInfo.GetCenter() + (goal.borderInfo.normal * distanceFromGoal);
    
    const Vec3f& kFwdVector = X_AXIS_3D();
    const Vec3f& kRightVector = -Y_AXIS_3D();
    const Vec3f& kUpVector = Z_AXIS_3D();
    
    // use directionality from the extra info to point in the same direction
    const Vec3f& vantagePointDir = (-goal.borderInfo.normal);
    const float fwdAngleCos = DotProduct(vantagePointDir, kFwdVector);
    const bool isPositiveAngle = (DotProduct(vantagePointDir, kRightVector) >= 0.0f);
    float rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);
    
    // add pose to vector
    outVantagePoints.emplace_back(rotRads, kUpVector, vantagePointPos, worldOrigin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS1_MoveToVantagePoint(Robot& robot, uint8_t retries)
{
  SetStateName("TransitionToS1_MoveToVantagePoint");
  PRINT_CH_INFO("Behaviors", (GetName() + ".S1").c_str(), "Moving to vantage point");
  
  // there have to be vantage points. If it's impossible to generate vantage points from the memory map,
  // we should change those borders/quads to "not visitable" to prevent failing multiple times
  ASSERT_NAMED(!_currentVantagePoints.empty(),
    "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.NoVantagePoints");
  
  // todo rsam: what happens if we are already close to the pose? Check that the action will succeed
  // request the action
  DriveToPoseAction* driveToPoseAction = new DriveToPoseAction( robot, _currentVantagePoints );
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
  StartActing(driveToPoseAction, onActionResult);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS2_ObserveAtVantagePoint(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".S2").c_str(), "Observing edges from vantage point");

  // ask blockworld to flag the interesting edges in front of Cozmo as noninteresting anymore
  FlagQuadAsNotInteresting(robot, 80, _goalPose, 50, 120); // todo config
  
  // play "i'm observing stuff" animation
  IAction* pauseAction = nullptr;
  
  const std::string& animTriggerName = ""; // todo rsam json this
  const AnimationTrigger trigger = animTriggerName.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerName.c_str());
  if ( trigger != AnimationTrigger::Count )
  {
    pauseAction = new TriggerAnimationAction(robot,trigger);
  }
  else
  {
    // create wait action
    const double waitTime_sec = 2.0f; // this is until we have the anim. It should not simply pause here
    pauseAction = new WaitAction( robot, waitTime_sec );
  }
  
  // request action with transition to proper state
  ASSERT_NAMED( nullptr!=pauseAction, "BehaviorExploreLookAroundInPlace::TransitionToS2_Pause.NullAction");
  StartActing( pauseAction );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::FlagQuadAsNotInteresting(Robot& robot, float halfWitdhAtRobot_mm,
  const Pose3d& goalPosition, float farPlaneDistFromGoal_mm, float halfAtFarPlaneWidth_mm)
{
  const Pose3d& robotPoseWRTOrigin = robot.GetPose().GetWithRespectToOrigin();
  
  Pose3d goalWRTOrigin;
  if ( goalPosition.GetWithRespectTo(*robot.GetWorldOrigin(), goalWRTOrigin) )
  {
    // bottom corners of the quad are based on the robot pose
    const Point3f& cornerBL = robotPoseWRTOrigin * Vec3f{ 0.f, +halfWitdhAtRobot_mm, 0.f};
    const Point3f& cornerBR = robotPoseWRTOrigin * Vec3f{ 0.f, -halfWitdhAtRobot_mm, 0.f};
    
    // top corners of the quad are based on the far plane
    Vec3f dirRobotToGoal = (goalWRTOrigin.GetTranslation() - robotPoseWRTOrigin.GetTranslation());
    dirRobotToGoal.MakeUnitLength();
    const Vec3f& farPlanePos = goalWRTOrigin.GetTranslation() + dirRobotToGoal * farPlaneDistFromGoal_mm;
    const Point3f& cornerTL = farPlanePos + robotPoseWRTOrigin.GetRotation() * Vec3f{ 0.f, +halfAtFarPlaneWidth_mm, 0.f};
    const Point3f& cornerTR = farPlanePos + robotPoseWRTOrigin.GetRotation() * Vec3f{ 0.f, -halfAtFarPlaneWidth_mm, 0.f};

    const Quad2f robotToFarPlaneQuad{ cornerTL, cornerBL, cornerTR, cornerBR };
    robot.GetBlockWorld().FlagQuadAsNotInterestingEdges(robotToFarPlaneQuad);

    // render the quad we are flagging as not interesting anymore
    if ( kVieDrawDebugInfo ) {
      robot.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        robotToFarPlaneQuad, 32.0f, Anki::NamedColors::BLUE, true);
    }
  } else {
    PRINT_NAMED_ERROR("BehaviorVisitInterestingEdge.FlagQuadAsNotInteresting.InvalidGoalPose",
      "Can't get goal pose with respect to origin");
  }
}

} // namespace Cozmo
} // namespace Anki
