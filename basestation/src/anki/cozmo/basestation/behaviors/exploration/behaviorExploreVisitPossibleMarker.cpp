/**
 * File: behaviorExploreVisitPossibleMarker
 *
 * Author: Raul
 * Created: 03/11/16
 *
 * Description: Behavior for looking around the environment for stuff to interact with. This behavior starts
 * as a copy from behaviorLookAround, which we expect to eventually replace. The difference is this new
 * behavior will simply gather information and put it in a place accessible to other behaviors, rather than
 * actually handling the observed information itself.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorExploreVisitPossibleMarker.h"

//#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorWhiteboard.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

#include <array>

namespace Anki {
namespace Cozmo {

// distance from the possible cube to go visit
CONSOLE_VAR(float, kEvpm_DistanceFromPossibleCubeMin_mm, "BehaviorExploreLookAroundInPlace", 100.0f);
CONSOLE_VAR(float, kEvpm_DistanceFromPossibleCubeMax_mm, "BehaviorExploreLookAroundInPlace", 200.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreVisitPossibleMarker::BehaviorExploreVisitPossibleMarker(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorExploreVisitPossibleMarker");

  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreVisitPossibleMarker::~BehaviorExploreVisitPossibleMarker()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreVisitPossibleMarker::IsRunnable(const Robot& robot) const
{
  // check whiteboard for known markers
  const BehaviorWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
  const BehaviorWhiteboard::PossibleMarkerList& markerList = whiteboard.GetPossibleMarkers();

  const bool canRun = !markerList.empty(); // consider distance limit
  return canRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreVisitPossibleMarker::InitInternal(Robot& robot)
{
  // 1) pick possible marker
  const BehaviorWhiteboard::PossibleMarker* closestMarker = nullptr;
  float distToClosestSQ = 0.0f;

  // get all markers from whiteboard
  const BehaviorWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
  const BehaviorWhiteboard::PossibleMarkerList& markerList = whiteboard.GetPossibleMarkers();
  for( const auto& marker : markerList )
  {
    // pick closes marker to us
    const Vec3f& dirToMarker = marker.pose.GetTranslation() - robot.GetPose().GetTranslation();
    const float distToMarkerSQ = dirToMarker.LengthSq();
    if ( (nullptr==closestMarker) || (distToMarkerSQ<distToClosestSQ) )
    {
      closestMarker = &marker;
      distToClosestSQ = distToMarkerSQ;
    }
  }

  // 2) create action to approach possible marker
  // we should have a closest marker
  if ( nullptr != closestMarker )
  {
    // calculate best approach position
    ApproachPossibleCube(robot, closestMarker->pose);
  
    return Result::RESULT_OK;
  }
  else
  {
    //this could happen if markers can be removed between IsRunnable and InitInternal
    PRINT_NAMED_ERROR("BehaviorExploreVisitPossibleMarker.InitInternal", "Could not pick closest marker on init");
    return Result::RESULT_FAIL;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreVisitPossibleMarker::ApproachPossibleCube(Robot& robot, const Pose3d& possibleCubePose)
{
  // trust that the whiteboard will never return information that is not valid in the current origin
  ASSERT_NAMED(&robot.GetPose().FindOrigin() == &possibleCubePose.FindOrigin(),
    "BehaviorExploreVisitPossibleMarker.WhiteboardPossibleMarkersDirty");

  // TODO if we are closer than max, limit max to that. I dont want to simply face the cube in that case because
  // we may have seen the marker from afar from a different position, and something is blocking it now. Ideally
  // I would like to store in the whiteboard which vantage positions I have already visited, so that I try a different
  // angle.

  // randomize distance now
  const float distanceRand = static_cast<float>( GetRNG().RandDblInRange(
    kEvpm_DistanceFromPossibleCubeMax_mm, kEvpm_DistanceFromPossibleCubeMin_mm));
  
  // generate all possible points to pick one later on
  Vec3f fwd (possibleCubePose.GetRotation() * X_AXIS_3D());
  fwd.z() = 0.0f;
  Vec3f side(fwd.y(), -fwd.x(), 0.0f);
  std::array<Vec3f, 4> possiblePoints;
  possiblePoints[0] = possibleCubePose.GetTranslation() + (fwd  * distanceRand);
  possiblePoints[1] = possibleCubePose.GetTranslation() - (fwd  * distanceRand);
  possiblePoints[2] = possibleCubePose.GetTranslation() + (side * distanceRand);
  possiblePoints[3] = possibleCubePose.GetTranslation() - (side * distanceRand);

  // pick closest
  const Vec3f& robotLoc = robot.GetPose().GetTranslation();
  size_t bestIndex = 0;
  float bestDistSq = (possiblePoints[bestIndex] - robotLoc).LengthSq();
  for( size_t i=1; i<possiblePoints.size(); ++i)
  {
    const float distSQ = (possiblePoints[i] - robotLoc).LengthSq();
    if ( distSQ < bestDistSq ) {
      bestDistSq = distSQ;
      bestIndex = i;
    }
  }
  
  // calculate pose for the final goal location (is there an easier way to calculate rotation?)
  const Vec3f& goalLocation = possiblePoints[bestIndex];
  float goalRotation_rad = 0.0f;
  {
    const Vec3f& kFwdVector = X_AXIS_3D();
    const Vec3f& kRightVector = -Y_AXIS_3D();
    
    // use directionality from the extra info to point in the same direction
    Vec3f facing = possibleCubePose.GetTranslation() - goalLocation;
    facing.MakeUnitLength();
    float facingAngleCos = DotProduct(facing, kFwdVector);
    const bool isPositiveAngle = (DotProduct(facing, kRightVector) >= 0.0f);
    goalRotation_rad = isPositiveAngle ? -std::acos(facingAngleCos) : std::acos(facingAngleCos);
  }
  
  const Vec3f& kUpVector = Z_AXIS_3D();
  const Pose3d goalPose(goalRotation_rad, kUpVector, goalLocation, &robot.GetPose().FindOrigin());
  
  // create action to go to that point
  DriveToPoseAction* driveAction = new DriveToPoseAction(robot, goalPose, DEFAULT_PATH_MOTION_PROFILE, false);
  
  // TODO Either set head up at end, or at start and as soon as we see the cube, react
  
  StartActing(driveAction);
}

} // namespace Cozmo
} // namespace Anki
