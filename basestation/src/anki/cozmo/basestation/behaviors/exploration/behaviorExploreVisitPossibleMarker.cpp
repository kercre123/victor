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

#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

#include <array>
#include "anki/cozmo/basestation/actions/basicActions.h"

// if true, plan to a pose which is adjacent to one of the markers. Otherwise, just drive straight towards the
// cube
#define APPROACH_NORMAL_TO_MARKERS 0

namespace Anki {
namespace Cozmo {

// distance from the possible cube to go visit
CONSOLE_VAR(float, kEvpm_DistanceFromPossibleCubeMin_mm, "BehaviorExploreLookAroundInPlace", 100.0f);
CONSOLE_VAR(float, kEvpm_DistanceFromPossibleCubeMax_mm, "BehaviorExploreLookAroundInPlace", 150.0f);
CONSOLE_VAR(float, kBEVPM_backupSpeed_mmps, "BehaviorExploreLookAroundInPlace", 90.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreVisitPossibleMarker::BehaviorExploreVisitPossibleMarker(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorExploreVisitPossibleMarker");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreVisitPossibleMarker::~BehaviorExploreVisitPossibleMarker()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreVisitPossibleMarker::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  // check whiteboard for known markers
  const AIWhiteboard& whiteboard = preReqData.GetRobot().GetAIComponent().GetWhiteboard();
  whiteboard.GetPossibleObjectsWRTOrigin(_possibleObjects);

  const bool canRun = !_possibleObjects.empty(); // TODO: consider distance limit
  return canRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreVisitPossibleMarker::InitInternal(Robot& robot)
{
  // 1) pick possible marker
  const AIWhiteboard::PossibleObject* closestPossibleObject = nullptr;
  float distToClosestSQ = 0.0f;

  // get all markers from whiteboard
  for( const auto& possibleObject : _possibleObjects )
  {
    // all possible objects have to be in robot's origin, otherwise whiteboard lied to us
    DEV_ASSERT((&possibleObject.pose.FindOrigin()) == (&robot.GetPose().FindOrigin()),
               "BehaviorExploreVisitPossibleMarker.InitInternal.InvalidOrigin" );
  
    // pick closest marker to us
    const Vec3f& dirToPossibleObject = possibleObject.pose.GetTranslation() - robot.GetPose().GetTranslation();
    const float distToPosObjSQ = dirToPossibleObject.LengthSq();
    // don't think we need this anymore since we remove them once we look at them
    // if( distToMarkerSQ < std::pow( kEvpm_DistanceFromPossibleCubeMin_mm, 2) ) {      
    //   // ignore cubes which we are already close to and facing
    //   continue;
    // }
    if ( (nullptr==closestPossibleObject) || (distToPosObjSQ<distToClosestSQ) )
    {
      closestPossibleObject = &possibleObject;
      distToClosestSQ = distToPosObjSQ;
    }
  }

  // 2) create action to approach possible marker
  // we should have a closest marker
  if ( nullptr != closestPossibleObject )
  {
    PRINT_NAMED_DEBUG("BehaviorExploreVisitPossibleMarker.Init",
                      "Approaching possible marker which is sqrt(%f)mm away",
                      distToClosestSQ);
    
    // calculate best approach position
    ApproachPossibleCube(robot, closestPossibleObject->type, closestPossibleObject->pose);
  
    return Result::RESULT_OK;
  }
  else
  {
    // this should not happen, otherwise we should have been not runnable
    PRINT_NAMED_ERROR("BehaviorExploreVisitPossibleMarker.InitInternal", "Could not pick closest marker on init");
    return Result::RESULT_FAIL;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreVisitPossibleMarker::ApproachPossibleCube(Robot& robot,
                                                              ObjectType objectType,
                                                              const Pose3d& possibleCubePose)
{
  // trust that the whiteboard will never return information that is not valid in the current origin
  DEV_ASSERT(&robot.GetPose().FindOrigin() == &possibleCubePose.FindOrigin(),
             "BehaviorExploreVisitPossibleMarker.WhiteboardPossibleMarkersDirty");

  // TODO if we are closer than max, limit max to that. I dont want to simply face the cube in that case because
  // we may have seen the marker from afar from a different position, and something is blocking it now. Ideally
  // I would like to store in the whiteboard which vantage positions I have already visited, so that I try a different
  // angle.

  // randomize distance now
  const float distanceRand = static_cast<float>( GetRNG().RandDblInRange(
    kEvpm_DistanceFromPossibleCubeMin_mm, kEvpm_DistanceFromPossibleCubeMax_mm));

  Pose3d relPose;
  if( possibleCubePose.GetWithRespectTo( robot.GetPose(), relPose ) ) {

    const float distSq = relPose.GetTranslation().LengthSq();
    
    if( std::pow(kEvpm_DistanceFromPossibleCubeMin_mm, 2) <= distSq &&
        distSq <= std::pow( kEvpm_DistanceFromPossibleCubeMax_mm, 2) ) {
      // the robot is already within range, so just look at the cube and verify that it's there

      // NOTE: there may be an obstacle here, this isn't a great way to do this (should use planning distance,
      // etc)

      static const int kNumImagesForVerification = 5;
      
      CompoundActionSequential* action = new CompoundActionSequential(robot, {
          new TurnTowardsPoseAction(robot, relPose),
          new WaitForImagesAction(robot, kNumImagesForVerification, VisionMode::DetectingMarkers) });

      PRINT_NAMED_INFO("BehaviorExploreVisitPossibleMarker.WithinRange.Verify",
                       "robot is already within range of the cube, check if we can see it");
      
      StartActing(action, [this, &robot, objectType, possibleCubePose](ActionResult res) {
          if( res == ActionResult::SUCCESS ) {
            MarkPossiblePoseAsEmpty(robot, objectType, possibleCubePose);
          }
        });
      return;
    }
  }
  else {
    PRINT_NAMED_WARNING("BehaviorExploreVisitPossibleMarker.NoTransform",
                        "Could not get pose of possible object W.R.T robot");
    return;
  }
  
  CompoundActionSequential* approachAction = new CompoundActionSequential(robot);
  
  if( APPROACH_NORMAL_TO_MARKERS ) {
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
    approachAction->AddAction( new DriveToPoseAction(robot, goalPose, false) );
  }
  else {

    // if we're too close, back up
    if( std::pow(kEvpm_DistanceFromPossibleCubeMin_mm, 2) > relPose.GetTranslation().LengthSq() ) {
      approachAction->AddAction( new TurnTowardsPoseAction(robot, possibleCubePose, M_PI_F) );
      const float backupDist = distanceRand - relPose.GetTranslation().Length();
      approachAction->AddAction( new DriveStraightAction(robot, -backupDist, kBEVPM_backupSpeed_mmps) );
    }
    else {    
      // drive directly to the cube
      const Pose2d relPose2d(relPose);
      Vec3f newTranslation = relPose.GetTranslation();
      float oldLength = newTranslation.MakeUnitLength();
      
      Pose3d newTargetPose(RotationVector3d{},
                           newTranslation * (oldLength - distanceRand),
                           &robot.GetPose());

      // turn first to signal intent
      approachAction->AddAction( new TurnTowardsPoseAction(robot, possibleCubePose, M_PI_F) );
      approachAction->AddAction( new DriveToPoseAction(robot, newTargetPose, false) );

      // This requires us to bail out of this behavior if we see one, but that's not currently happening
      // // add a search action after driving / facing, in case we don't see the object
      // approachAction->AddAction(new SearchSideToSideAction(robot));
    }
  }

  // TODO Either set head up at end, or at start and as soon as we see the cube, react
  
  StartActing(approachAction);
}

void BehaviorExploreVisitPossibleMarker::MarkPossiblePoseAsEmpty(Robot& robot, ObjectType objectType, const Pose3d& pose)
{
  PRINT_NAMED_INFO("BehaviorExploreVisitPossibleMarker.ClearPose",
                   "robot looked at pose, so clear it");

  robot.GetAIComponent().GetWhiteboard().FinishedSearchForPossibleCubeAtPose(objectType, pose);
}


} // namespace Cozmo
} // namespace Anki
