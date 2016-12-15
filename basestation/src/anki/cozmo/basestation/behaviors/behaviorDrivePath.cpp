/**
 * File: BehaviorDrivePath.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-06-16
 *
 * Description: Behavior to all cozmo to drive around in a circle
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorDrivePath.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/actions/drivePathAction.h"
#include "anki/planning/shared/path.h"
#include "anki/planning/basestation/pathHelper.h"

namespace Anki {
namespace Cozmo {
  

BehaviorDrivePath::BehaviorDrivePath(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("DrivePath");
}

bool BehaviorDrivePath::IsRunnableInternal(const Robot& robot) const
{
  //Possibly add other limits later
  return robot.GetEnabledAnimationTracks() & (u8)AnimTrackFlag::BODY_TRACK;
}

float BehaviorDrivePath::EvaluateScoreInternal(const Robot& robot) const
{
  return 0.f;
}

Result BehaviorDrivePath::InitInternal(Robot& robot)
{
  TransitionToFollowingPath(robot);
  return Result::RESULT_OK;
}

void BehaviorDrivePath::TransitionToFollowingPath(Robot& robot)
{
  DEBUG_SET_STATE(FollowingPath);
  SelectPath(robot.GetPose(), _path);
  
  IActionRunner* drivePath = new DrivePathAction(robot, _path);
  //Perform action and then callback
  StartActing(drivePath, [this](ActionResult res) {
    switch(IActionRunner::GetActionResultCategory(res))
    {
      case ActionResultCategory::SUCCESS:
        BehaviorObjectiveAchieved(BehaviorObjective::DroveAsIntended);
        break;
        
      case ActionResultCategory::RETRY:
        break;
        
      case ActionResultCategory::ABORT:
        PRINT_NAMED_INFO("BehaviorDrivePath.FailedAbort",
                         "Failed to drive path, aborting");
        break;
        
      default:
        // other failure, just end
        PRINT_NAMED_INFO("BehaviorDrivePath.FailedDriveAction",
                         "action failed with %s, behavior ending",
                         EnumToString(res));
    } // switch(msg.result)
  });
  
}

void BehaviorDrivePath::SelectPath(const Pose3d& startingPose, Planning::Path& path)
{
  _path.Clear();
  
  int numberPathTypes = 3;
  int pathSelect =  GetRNG().RandInt(numberPathTypes) % numberPathTypes;
  switch(pathSelect){
    case 0:
      BuildSquare(startingPose, path);
      break;
    case 1:
      BuildFigureEight(startingPose, path);
      break;
    case 2:
      BuildZ(startingPose, path);
      break;
  }
}

void BehaviorDrivePath::BuildSquare(const Pose3d& startingPose, Planning::Path& path)
{
  Planning::PathRelativeCozmo relPath;
  auto straight = std::make_shared<Planning::RelativePathLine>(100.0, 100.0, 100.0, 100.0);
  auto turn = std::make_shared<Planning::RelativePathTurn>(M_PI_2, 100.0, 100.0, 100.0, M_PI_4, true);
  
  for(int i=0; i < 4; i++){
    relPath.AppendSegment(straight);
    relPath.AppendSegment(turn);
  }
  relPath.GetPlanningPath(startingPose, path);
}

void BehaviorDrivePath::BuildFigureEight(const Pose3d& startingPose, Planning::Path& path)
{
  Planning::PathRelativeCozmo relPath;
  bool clockwise = true;
  
  for(int i = 0; i < 4; i++){
    auto arcSeg = std::make_shared<Planning::RelativePathArc>(100, M_PI, 100, 100, 100, clockwise);
    relPath.AppendSegment(arcSeg);
    relPath.AppendSegment(arcSeg);
    clockwise = !clockwise;
  }
  
  relPath.GetPlanningPath(startingPose, path);
  
}

void BehaviorDrivePath::BuildZ(const Pose3d& startingPose, Planning::Path& path)
{
  Planning::PathRelativeCozmo relPath;
  auto top_bottom = std::make_shared<Planning::RelativePathLine>(100.0, 100.0, 100.0, 100.0);
  auto turnDown = std::make_shared<Planning::RelativePathTurn>(M_PI_2 + M_PI_4, 100.0, 100.0, 100.0, M_PI_4, true);
  auto turnBack = std::make_shared<Planning::RelativePathTurn>(M_PI_2 + M_PI_4, 100.0, 100.0, 100.0, M_PI_4, false);
  auto diagonal = std::make_shared<Planning::RelativePathLine>(100.0, 100.0, 100.0, 100.0);
  
  relPath.AppendSegment(top_bottom);
  relPath.AppendSegment(turnDown);
  relPath.AppendSegment(diagonal);
  relPath.AppendSegment(turnBack);
  relPath.AppendSegment(top_bottom);
  
  relPath.GetPlanningPath(startingPose, path);
}
  
} //namespace Cozmo
} //namespace Anki
