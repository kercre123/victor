/**
 * File: drivePathAction.cpp
 *
 * Author: Kevin M. Karol
 * Date:   2016-06-16
 *
 * Description: Allows Cozmo to drive an arbitrary specified path
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/drivePathAction.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/planning/shared/path.h"

namespace Anki {
namespace Cozmo {
  

DrivePathAction::DrivePathAction(Robot& robot, const Planning::Path& path)
: IAction(robot
          , "DrivePathAction"
          , RobotActionType::DRIVE_PATH
          , (u8)AnimTrackFlag::BODY_TRACK)
,_robot(robot)
,_path(path)
{
}

ActionResult DrivePathAction::Init()
{
  ActionResult result = ActionResult::SUCCESS;
  ERobotDriveToPoseStatus status = _robot.CheckDriveToPoseStatus();
  
  if(status != ERobotDriveToPoseStatus::Waiting){
    result = ActionResult::PATH_PLANNING_FAILED_ABORT;
    return result;
  }
  
  // Tell robot to execute this simple path
  if(RESULT_OK != _robot.ExecutePath(_path, false)) {
    result = ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
    return result;
  }
  
  return result;
}

ActionResult DrivePathAction::CheckIfDone()
{
  //Check if robot arrived at destination
  //TODO: Currently no way to detect path failure
  ActionResult result = _robot.IsTraversingPath() ? ActionResult::RUNNING: ActionResult::SUCCESS;
  
  return result;
}

  
} //namespace Cozmo
} //namespace Anki

