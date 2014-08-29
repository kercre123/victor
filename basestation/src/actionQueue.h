/**
 * File: actionQueue.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Defines IAction interface for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ACTIONQUEUE_H
#define ANKI_COZMO_ACTIONQUEUE_H

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

namespace Anki {
  
  // TODO: Is this Cozmo-specific or can it be moved to coretech?
  // (Note it does require a Robot, which is currently only present in Cozmo)
  
  namespace Cozmo {

    // Forward Declarations:
    class Robot;
    
    class IAction
    {
    public:
      typedef enum  {
        RUNNING,
        SUCCESS,
        FAILURE_TIMEOUT,
        FAILURE_PROCEED,
        FAILURE_RETRY,
        FAILURE_ABORT
      } ActionResult;
      
      IAction(Robot& robot);
      
      // This is what gets called from the outside:
      virtual ActionResult Update() final; // can't override this in derived classes
    
      virtual const std::string& GetName() const;
    
    protected:
      
      Robot&        _robot;
      
      // Derived Actions should implement these:
      virtual ActionResult  CheckPreconditions() const { return SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone() = 0;
      
      virtual f32 GetStartDelayInSeconds() const { return 0.f;  } // Optional: default is no delay
      virtual f32 GetTimeoutInSeconds()    const { return 60.f; } // Optional: default is one minute
      
    private:
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
    }; // class IAction
    
    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(Robot& robot, const Pose3d& pose, const Radians& headAngle);
      DriveToPoseAction(Robot& robot, const Pose3d& pose); // with current head angle
      
      virtual const std::string& GetName() const override;
      
    protected:
      virtual ActionResult CheckIfDone() override;
      
      Pose3d  _goalPose;
      Radians _goalHeadAngle;
      f32     _goalDistanceThreshold;
      Radians _goalAngleThreshold;
    };
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTIONQUEUE_H
