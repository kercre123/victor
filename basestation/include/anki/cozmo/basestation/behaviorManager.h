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

#include <unordered_map>
#include <string>

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
    
    
    ///////////////////////////////////////////////////////////////////////////
    //
    //                NEW BEHAVIOR ENGINE API EXPLORATION
    //
    
    // Forward declaration
    class IBehavior;
    class Reward;
    
    class BehaviorEngine
    {
    public:
      
      // Calls the currently-selected behavior's Update() method until it
      // returns COMPLETE or FAILURE. Once current behavior completes
      // switches to next behavior (including an "idle" behavior).
      Result Update(Robot& robot);
      
      // Picks next behavior based on robot's current state. This does not
      // transition immediately to running that behavior, but will let the
      // current beheavior know it needs wind down with a call to its
      // Interrupt() method.
      Result SelectNextBehavior(Robot& robot);
      
      // Forcefully select the next behavior by name (versus by letting the
      // selection mechanism choose based on current state)
      Result SelectNextBehavior(const std::string& name);
      
      Result AddBehavior(const std::string& name, IBehavior* newBehavior);
      
    private:
      // Map from behavior name to available behaviors
      std::unordered_map<std::string, IBehavior*> _behaviors;
      
      IBehavior* _currentBehavior;
      IBehavior* _nextBehavior;
      
    }; // class BehaviorEngine
    
    
    // Base Behavior Interface specification
    class IBehavior
    {
    public:
      enum class Status {
        FAILURE,
        RUNNING,
        COMPLETE
      };
      
      virtual Status Init(Robot& robot) = 0;
      
      // Step through the behavior and deliver rewards to the robot along the way
      virtual Status Update(Robot& robot) = 0;
      
      virtual Status Interrupt(Robot& robot) = 0;
      
      // Figure out the reward this behavior will offer, given the robot's current
      // state. Returns true if the Behavior is runnable, false if not. (In the
      // latter case, the returned reward is not populated.)
      virtual bool GetRewardBid(Robot& robot, Reward& reward) = 0;
      
      
    }; // class IBehavior
    
    
    // A behavior that tries to neaten up 
    class OCD_Behavior : public IBehavior
    {
    public:
      
      virtual Status Init(Robot& robot) override;
      
      virtual Status Update(Robot& robot) override;
      
      virtual bool GetRewardBid(Robot& robot, Reward& reward) override;
      
    private:
      
      // Internally, this behavior is just a little state machine going back and
      // forth between picking up and placing blocks
      
      enum class State {
        PICKING_UP_BLOCK,
        PLACING_BLOCK
      };
      
      State _currentState;
      
      // Enumerate possible arrangements Cozmo "likes".
      enum class Arrangement {
        STACKS,
        LINE
      };
      
      Arrangement _currentArrangement;
      
    }; // class OCD_Behavior
    
    
    class TrackFaceBehavior : public IBehavior
    {
    public:
      
      virtual Status Update(Robot& robot) override;
      
    private:
      
      enum class State {
        LOOKING_AROUND,
        TRACKING_FACE
      };
      
      State _currentState;
      
    }; // LookForFacesBehavior
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H
