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
#include "anki/cozmo/robot/cozmoConfig.h"

// amount of radians to be off from the desired angle in order to
// introduce a turn in place action
#define FACE_AND_APPROACH_THETA_THRESHOLD 0.0872664625997

// distance (in mm) away at which to introduce a straight action
#define FACE_AND_APPROACH_LENGTH_THRESHOLD DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM

#define FACE_AND_APPROACH_LENGTH_SQUARED_THRESHOLD FACE_AND_APPROACH_LENGTH_THRESHOLD * FACE_AND_APPROACH_LENGTH_THRESHOLD

#define FACE_AND_APPROACH_PLANNER_ACCEL 200.0f
#define FACE_AND_APPROACH_PLANNER_DECEL 200.0f
#define FACE_AND_APPROACH_TARGET_SPEED 30.0f

#define FACE_AND_APPROACH_PLANNER_ROT_ACCEL 100.0f
#define FACE_AND_APPROACH_PLANNER_ROT_DECEL 100.0f
#define FACE_AND_APPROACH_TARGET_ROT_SPEED 0.5f

#define FACE_AND_APPRACH_DELTA_THETA_FOR_BACKUP 1.0471975512


namespace Anki {
namespace Cozmo {

IPathPlanner::EPlanStatus FaceAndApproachPlanner::GetPlan(Planning::Path &path,
                                                          const Pose3d& startPose,
                                                          const Pose3d& targetPose)
{
  _targetVec = targetPose.get_translation();
  _finalTargetAngle = targetPose.get_rotationAngle<'Z'>().ToFloat();

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

  bool doTurn0 = false;
  bool doTurn1 = false;
  bool doStraight = false;

  Vec3f startVec(startPose.get_translation());

  // check if we need to do each segment
  Radians targetAngle = atan2(_targetVec.y() - startVec.y(), _targetVec.x() - startVec.x());
  Radians currAngle = startPose.get_rotationAngle<'Z'>();

  float deltaTheta = currAngle.minAngularDistance(targetAngle);

  if(std::abs(deltaTheta) > FACE_AND_APPROACH_THETA_THRESHOLD) {
    printf("FaceAndApproachPlanner: doing initial turn because delta theta of %f > %f\n",
           deltaTheta,
           FACE_AND_APPROACH_THETA_THRESHOLD);
    doTurn0 = true;
  }

  if(std::abs(targetAngle.ToFloat() - _finalTargetAngle) > FACE_AND_APPROACH_THETA_THRESHOLD) {
    printf("FaceAndApproachPlanner: doing final turn because delta theta of %f > %f\n",
           deltaTheta,
           FACE_AND_APPROACH_THETA_THRESHOLD);
    doTurn1 = true;
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

  if(!doTurn0 && !doStraight && !doTurn1) {
    return PLAN_NOT_NEEDED;
  }

  path.Clear();

  bool backup = false;
  if(doTurn0) {
    if(std::abs(deltaTheta) > FACE_AND_APPRACH_DELTA_THETA_FOR_BACKUP) {
      printf("FaceAndApproachPlanner: deltaTheta of %f above threshold, doing backup!\n", deltaTheta);
      deltaTheta = (Radians(deltaTheta) + M_PI).ToFloat();
      targetAngle = targetAngle + M_PI;
      backup = true;
    }

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
                    backup ? -FACE_AND_APPROACH_TARGET_SPEED : FACE_AND_APPROACH_TARGET_SPEED,
                    FACE_AND_APPROACH_PLANNER_ACCEL,
                    FACE_AND_APPROACH_PLANNER_DECEL);
  }

  if(doTurn1) {
    float deltaTheta1 = _finalTargetAngle - targetAngle.ToFloat();
    path.AppendPointTurn(0,
                         _targetVec.x(), _targetVec.y(), _finalTargetAngle,
                         deltaTheta1 < 0 ? -FACE_AND_APPROACH_TARGET_ROT_SPEED : FACE_AND_APPROACH_TARGET_ROT_SPEED,
                         FACE_AND_APPROACH_PLANNER_ROT_ACCEL,
                         FACE_AND_APPROACH_PLANNER_ROT_DECEL);
  }

  return DID_PLAN;  
}

    
 
} // namespace Cozmo
} // namespace Anki
