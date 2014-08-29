/**
 * File: actionQueue.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements IAction interface for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "actionQueue.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/robot.h"

namespace Anki {
  
  namespace Cozmo {
    
    IAction::IAction(Robot& robot)
    : _robot(robot)
    , _preconditionsMet(false)
    , _waitUntilTime(-1.f)
    , _timeoutTime(-1.f)
    {
    
    }
    
    IAction::ActionResult  IAction::Update()
    {
      ActionResult result = RUNNING;
      
      // On first call to Update(), figure out the waitUntilTime
      const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitUntilTime < 0.f) {
        _waitUntilTime = currentTimeInSeconds + GetStartDelayInSeconds();
      }
      if(_timeoutTime < 0.f) {
        _timeoutTime = currentTimeInSeconds + GetTimeoutInSeconds();
      }

      // Fail if we have exceeded timeout time
      if(currentTimeInSeconds > _timeoutTime) {
        PRINT_NAMED_INFO("IAction.Update.TimedOut",
                         "%s timed out after %.1f seconds.\n",
                         GetName().c_str(), GetStartDelayInSeconds());
        
        result = FAILURE_TIMEOUT;
      }
      // Don't do anything until we have reached the waitUntilTime
      else if(currentTimeInSeconds > _waitUntilTime)
      {
        
        if(!_preconditionsMet) {
          // Note that derived classes will define what to do when pre-conditions
          // are not met: if they return RUNNING, then the action will effectively
          // just wait for the preconditions to be met. Otherwise, a failure
          // will get propagated out as the return value of the Update method.
          result = this->CheckPreconditions();
          if(result == SUCCESS) {
            PRINT_NAMED_INFO("IAction.Update.PrecondtionsMet",
                             "Preconditions for %s successfully met.\n", GetName().c_str());
            _preconditionsMet = true;
          }
          
        } else {
          // Pre-conditions already met, just run until done
          result = this->CheckIfDone();
          
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      return result;
    } // Update()
    
    const std::string& IAction::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
  
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const Pose3d& pose)
    : DriveToPoseAction(robot, pose, robot.GetHeadAngle())
    {
      
    }
    
    DriveToPoseAction::DriveToPoseAction(Robot& robot, const Pose3d& pose, const Radians& headAngle)
    : IAction(robot)
    , _goalPose(pose)
    , _goalHeadAngle(headAngle)
    {
      
    }
    
    const std::string& DriveToPoseAction::GetName() const
    {
      static const std::string name("DriveToPoseAction");
      return name;
    }
    
    IAction::ActionResult DriveToPoseAction::CheckIfDone()
    {
      ActionResult result = RUNNING;
      
      if(!_robot.IsTraversingPath())
      {
        if(_robot.GetPose().IsSameAs(_goalPose, _goalDistanceThreshold, _goalAngleThreshold))
        {
          PRINT_INFO("Robot %d finished following path.\n", _robot.GetID());
          _robot.ClearPath(); // clear path and indicate that we are not replanning
          result = SUCCESS;
        }
        // The last path sent was definitely received by the robot
        // and it is no longer executing it.
        else if (_robot.GetLastSentPathID() == _robot.GetLastRecvdPathID()) {
          PRINT_NAMED_INFO("Robot.Update.FollowToNextState", "lastPathID %d\n", _robot.GetLastRecvdPathID());
          result = FAILURE_PROCEED;
        }
        else {
          PRINT_NAMED_INFO("Robot.Update.FollowPathStateButNotTraversingPath",
                           "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false. currPathSegment = %d\n",
                           _robot.GetCurrPathSegment());
          result = FAILURE_ABORT;
        }
      }
      
      return result;
      
    } // CheckIfDone()
    
    
  } // namespace Cozmo
} // namespace Anki
