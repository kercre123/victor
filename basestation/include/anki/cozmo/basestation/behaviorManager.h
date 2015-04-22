/**
 * File: behaviorManager.h
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Description: High-level module that determines what robots should be doing.
 *              Used primarily for test as this could eventually be replaced by
 *              some sort of game-level module.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef BEHAVIOR_MANAGER_H
#define BEHAVIOR_MANAGER_H

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/common/basestation/objectTypesAndIDs.h"

namespace Anki {
  namespace Cozmo {
    
    class BlockWorld;
    class RobotManager;
    class Robot;
    class Block;
    
    
    class BehaviorManager
    {
    public:
      
      typedef enum {
        None,
        June2014DiceDemo,
        ReactToMarkers,
        CREEP, // "Cozmo Robotic Emotional Engagement PlayTests" - Nov 2014
      } Mode;
      
      
      typedef enum {
        WAITING_FOR_ROBOT,
        ACKNOWLEDGEMENT_NOD,
        
        // June2014DiceDemo
        DRIVE_TO_START,
        WAITING_TO_SEE_DICE,
        WAITING_FOR_DICE_TO_DISAPPEAR,
        GOTO_EXPLORATION_POSE,
        START_EXPLORING_TURN,
        BACKING_UP,
        BEGIN_EXPLORING,
        EXPLORING,
        EXECUTING_DOCK,
        CHECK_IT_OUT_UP,
        CHECK_IT_OUT_DOWN,
        FACE_USER,
        HAPPY_NODDING,
        BACK_AND_FORTH_EXCITED,
        
        // CREEP
        SLEEPING,
        EXCITABLE_CHASE,
        SCARED_FLEE,
        DANCE_WITH_BLOCK,
        SCAN,
        HELP_ME_STATE,
        WHAT_NEXT,
        IDLE,
        
        NUM_STATES
      } BehaviorState;
      
      
      BehaviorManager(Robot* robot);
      
      void StartMode(Mode mode);
      Mode GetMode() const;
      
      void SetNextState(BehaviorState nextState);
      
      void Update();

      Robot* GetRobot() const {return _robot;}
      
      const ObjectID GetObjectOfInterest() const;
      
    protected:
      
      BehaviorState _state, _nextState;
      std::function<void(BehaviorManager*)> _updateFcn;

      Mode _mode;
      
      Robot* _robot;

      // mini-state machine for the idle animation for June demo
      typedef enum {
        IDLE_NONE,
        IDLE_LOOKING_UP,
        IDLE_PLAYING_SOUND,

        // used after some number of idle loops
        IDLE_FACING_USER,
        IDLE_LOOKING_DOWN,
        IDLE_TURNING_BACK
      } IdleAnimationState;
      
      IdleAnimationState _idleState;
      unsigned int _timesIdle;
      Pose3d _originalPose;
            
      // Object _type_ the robot is currently "interested in"
      ObjectType _objectTypeOfInterest;
      
      void Reset();
      
      //bool GetPathToPreDockPose(const Block& b, Path& p);
      
      // Behavior state machines
      void Update_PickAndPlaceBlock();
      void Update_June2014DiceDemo();
      void Update_TraverseObject();
      void Update_CREEP();
      
      // Waits until the current path is complete / timeout occurs.
      // If the blockOfInterest_ is no longer present in the
      //   world at the end of the path, we transition to problemState_.
      // Otherwise, we tilt the head and transition to nextState_.
      // If we don't end up at the right position at the end of the path, we
      //   reset, calling StartMode(BM_None)
      void FollowPathHelper();
      
      // Confirms dockingMarker_ is present in image where expected, chooses
      // low or high dock, actually begins docking procedure, and transitions
      // to EXECUTING_DOCK state.
      void BeginDockingHelper();
      
      // Gets the closest predock pose to the robot for the Block currently
      //  pointed to by dockBlock_, at the specified pre-dock distance, with
      //  the specified head angle.
      // Sets nearestPreDockPose_, dockMarker_, and waitUntilTime_ accordingly.
      // Sets state_ to EXECUTING_PATH_DOCK_POSE on success, leaves state_
      //  unchanged otherwise.
      // If no pre-dock pose is found, StartMode(BM_None) is called.
      void GoToNearestPreDockPoseHelper(const f32 preDockDistance,
                                        const f32 desiredHeadAngle);
      
      /////// PickAndPlace vars ///////
    
      // Block to dock with
      //Block* dockBlock_;
      //const Vision::KnownMarker *dockMarker_;
      
      // Goal pose for backing up
      Pose3d _goalPose;
      
      // A general time value for gating state transitions
      double _waitUntilTime;

      f32 _desiredBackupDistance;
      
      /////// June2014DiceDemo vars ///////
      const TimeStamp_t TimeBetweenDice_ms = 1000; // 1 sec
      ObjectType   _objectToPickUp;
      ObjectType   _objectToPlaceOn;
      TimeStamp_t  _diceDeletionTime;
      bool         _wasCarryingBlockAtDockingStart;
      Radians      _explorationStartAngle;
      bool         _isTurning;
      
      /////// CREEP vars ///////
      std::map<BehaviorState, std::string> _stateAnimations;
      std::map<BehaviorState, std::map<BehaviorState, std::string> > _transitionAnimations;
      
      const std::string& GetBehaviorStateName(BehaviorState) const;

      
    }; // class BehaviorManager
    
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H
