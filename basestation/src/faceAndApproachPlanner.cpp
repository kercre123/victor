/**
 * File: faceAndApproachPlanner.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-18
 *
 * Description: Simple planner that does a point turn and straight to
 * get to a goal. Supports replanning
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#include "pathPlanner.h"
#include "anki/common/basestation/general.h"

// amount of radians to be off from the desired angle in order to
// introduce a turn in place action
#define FACE_AND_APPROACH_THETA_THRESHOLD 0.0872664625997

// distance (in mm) away at which to introduce a straight action
#define FACE_AND_APPROACH_LENGTH_THRESHOLD 0.5

#define FACE_AND_APPROACH_LENGTH_SQUARED_THRESHOLD FACE_AND_APPROACH_LENGTH_THRESHOLD * FACE_AND_APPROACH_LENGTH_THRESHOLD

#define FACE_AND_APPROACH_PLANNER_ACCEL 200.0f
#define FACE_AND_APPROACH_PLANNER_DECEL 200.0f
#define FACE_AND_APPROACH_TARGET_SPEED 30.0f

#define FACE_AND_APPROACH_PLANNER_ROT_ACCEL 100.0f
#define FACE_AND_APPROACH_PLANNER_ROT_DECEL 100.0f
#define FACE_AND_APPROACH_TARGET_ROT_SPEED 0.5f


namespace Anki {
namespace Cozmo {

IPathPlanner::EPlanStatus FaceAndApproachPlanner::GetPlan(Planning::Path &path,
                                                          const Pose3d& startPose,
                                                          const Pose3d& targetPose)
{
  _targetVec = targetPose.get_translation();

  return GetPlan(path, startPose, true);
}


IPathPlanner::EPlanStatus FaceAndApproachPlanner::GetPlan(Planning::Path &path,
                                                          const Pose3d& startPose,
                                                          bool forceReplanFromScratch)
{
  // for now, don't try to replan
  if(!forceReplanFromScratch)
    return PLAN_NOT_NEEDED;

  // TODO:(bn) this logic is incorrect for planning because it will
  // just constantly send a new plan. Instead if needs to detect if it
  // has veered off the plan somehow

  bool doTurn = false;
  bool doStraight = false;

  Vec3f startVec(startPose.get_translation());

  // check if we need to do each segment
  Radians targetAngle = atan2(_targetVec.y() - startVec.y(), _targetVec.x() - startVec.x());
  Radians currAngle = startPose.get_rotationAngle<'Z'>();

  float deltaTheta = currAngle.minAngularDistance(targetAngle);

  if(std::abs(deltaTheta) > FACE_AND_APPROACH_THETA_THRESHOLD) {
    printf("FaceAndApproachPlanner: doing turn because delta theta of %f > %f\n",
           deltaTheta,
           FACE_AND_APPROACH_THETA_THRESHOLD);
    doTurn = true;
  }

  Point2f start2d(startVec.x(), startVec.y());
  Point2f target2d(_targetVec.x(), _targetVec.y());
  float distSquared = pow(target2d.x() - start2d.x(), 2) + pow(target2d.y() - start2d.y(), 2);
  if(distSquared > FACE_AND_APPROACH_LENGTH_SQUARED_THRESHOLD) {
    printf("FaceAndApproachPlanner: doing straight because distance^2 of %f > %f\n",
           distSquared,
           FACE_AND_APPROACH_LENGTH_SQUARED_THRESHOLD);
    doStraight = true;
  }

  if(!doTurn && !doStraight) {
    return PLAN_NOT_NEEDED;
  }

  path.Clear();

  if(doTurn) { // TEMP: sometimes this is backwards!!!
    path.AppendPointTurn(0,
                         startVec.x(), startVec.y(), targetAngle.ToFloat(),
                         deltaTheta < 0 ? -FACE_AND_APPROACH_TARGET_ROT_SPEED : FACE_AND_APPROACH_TARGET_ROT_SPEED,
                         FACE_AND_APPROACH_PLANNER_ROT_ACCEL,
                         FACE_AND_APPROACH_PLANNER_ROT_DECEL);
  }

  if(doStraight) {
    path.AppendLine(0,
                    startVec.x(), startVec.y(),
                    _targetVec.x(), _targetVec.y(),
                    FACE_AND_APPROACH_TARGET_SPEED,
                    FACE_AND_APPROACH_PLANNER_ACCEL,
                    FACE_AND_APPROACH_PLANNER_DECEL);
  }

  return DID_PLAN;  
}

    
 
} // namespace Cozmo
} // namespace Anki
