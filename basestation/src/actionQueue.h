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
#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/shared/cozmoTypes.h"

#include "actionableObject.h"

namespace Anki {
  
  // TODO: Is this Cozmo-specific or can it be moved to coretech?
  // (Note it does require a Robot, which is currently only present in Cozmo)
  
  
  namespace Vision {
    // Forward Declarations:
    class KnownMarker;
  }
  
  namespace Cozmo {

    // Forward Declarations:
    class Robot;
    
    // Action Interface
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
      
      // Provide a retry function that will be called by Update() if
      // FAILURE_RETRY is returned by the derived CheckIfDone() method.
      void SetRetryFunction(std::function<Result(Robot&)> retryFcn);
      
      // This is what gets called from the outside:
      virtual ActionResult Update() final; // can't override this in derived classes
    
      virtual const std::string& GetName() const; // Defaults to "UnnamedAction"
    
    protected:
      
      Robot&        _robot;
      
      // Derived Actions should implement these:
      virtual ActionResult  CheckPreconditions() { return SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone() = 0;
      
      virtual f32 GetStartDelayInSeconds() const { return 0.5f;  } // Optional: default is 0.5s delay
      virtual f32 GetTimeoutInSeconds()    const { return 60.f;  } // Optional: default is one minute
      
    private:
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
      std::function<Result(Robot&)> _retryFcn;
      
    }; // class IAction
    
    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(Robot& robot, const Pose3d& pose, const Radians& headAngle);
      DriveToPoseAction(Robot& robot, const Pose3d& pose); // with current head angle
      
      // Let robot's planner select "best" possible pose as the goal
      DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses, const Radians& headAngle);
      DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses); // with current head angle
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      virtual const std::string& GetName() const override;
      
    protected:
      virtual ActionResult CheckIfDone() override;
      virtual ActionResult CheckPreconditions() override;
      
      Pose3d   _goalPose;
      std::vector<Pose3d> _possibleGoalPoses;
      
      Radians  _goalHeadAngle;
      f32      _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      
    }; // class DriveToPoseAction
    
    
    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public IAction
    {
    public:
      TurnInPlaceAction(Robot& robot, const Radians& angle);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone() override;
      virtual ActionResult CheckPreconditions() override;
      
      Radians _turnAngle;
      Pose3d  _goalPose;
      
    }; // class TurnInPlaceAction
    
    
    class IDockAction : public IAction
    {
    public:
      IDockAction(Robot& robot, ObjectID objectID);
      
    protected:
      
      virtual ActionResult CheckPreconditions() override;
      virtual ActionResult CheckIfDone() override;
      
      virtual Result DockWithObjectHelper(const std::vector<ActionableObject::PoseMarkerPair_t>& preActionPoseMarkerPairs,
                                  const size_t closestIndex);
      
      virtual Result SelectDockAction(ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      
      ObjectID                    _dockObjectID;
      const Vision::KnownMarker*  _dockMarker;
      const Vision::KnownMarker*  _dockMarker2; // for bridges
      DockAction_t                _dockAction;
      Pose3d                      _dockObjectOrigPose;
    };

    
    class PickUpObjectAction : public IDockAction
    {
    public:
      PickUpObjectAction(Robot& robot, ObjectID objectID);
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
    }; // class DockWithObjectAction

    
    class CrossBridgeAction : public IDockAction
    {
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual Result DockWithObjectHelper(const std::vector<ActionableObject::PoseMarkerPair_t>& preActionPoseMarkerPairs,
                                          const size_t closestIndex) override;
      
    }; // class TraverseObjectAction
    
    
    class AscendOrDescendRampAction : public IDockAction
    {
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
    }; // class AscendOrDesceneRampAction
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTIONQUEUE_H
