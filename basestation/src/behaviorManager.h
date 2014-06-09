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

namespace Anki {
  namespace Cozmo {
    
    class BlockWorld;
    class RobotManager;
    class Robot;
    class Block;
    
    typedef enum {
      BM_None,
      BM_PickAndPlace,
      BM_June2014DiceDemo
    } BehaviorMode;
    
    class BehaviorManager
    {
    public:
      BehaviorManager();
      
      void Init(RobotManager* robotMgr, BlockWorld* world);
      
      void StartMode(BehaviorMode mode);
      
      void Update();

      Robot* GetRobot() const {return robot_;}
      
      const ObjectID_t GetBlockOfInterest() const {return blockOfInterest_;}
      
      // Select the next object in blockWorld as the block of interest
      void SelectNextBlockOfInterest();
      
    protected:
      
      typedef enum {
        WAITING_FOR_ROBOT,
        ACKNOWLEDGEMENT_NOD,
        
        // PickAndPlaceBlock
        WAITING_FOR_DOCK_BLOCK,
        EXECUTING_PATH_TO_DOCK_POSE,
        BEGIN_DOCKING,
        EXECUTING_DOCK,
        
        // June2014DiceDemo
        WAITING_TO_SEE_DICE,
        WAITING_FOR_DICE_TO_DISAPPEAR,
        BEGIN_EXPLORING,
        EXPLORING
        
      } BehaviorState;
      
      RobotManager* robotMgr_;
      BlockWorld* world_;
      
      BehaviorState state_, nextState_, problemState_;
      void (BehaviorManager::*updateFcn_)();
      
      Robot* robot_;

      // Block that the robot is currently travelling to, docking to,
      ObjectID_t blockOfInterest_;
      
      // Thresholds for knowing we're done with a path traversal
      f32    distThresh_mm_;
      f32    angThresh_;
      
      void Reset();
      
      //bool GetPathToPreDockPose(const Block& b, Path& p);
      
      // Behavior state machines
      void Update_PickAndPlaceBlock();
      void Update_June2014DiceDemo();
      
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
      const Block* dockBlock_;
      const Vision::KnownMarker *dockMarker_;
      
      // Target pose for predock
      Pose3d nearestPreDockPose_;
      
      // A general time value for gating state transitions
      double waitUntilTime_;

      DockAction_t dockAction_;
      
      
      /////// June2014DiceDemo vars ///////
      const TimeStamp_t TimeBetweenDice_ms = 2000; // 2 sec
      ObjectType_t blockToPickUp_;
      ObjectType_t blockToPlaceOn_;
      TimeStamp_t  diceDeletionTime_;
      
      
    }; // class BehaviorManager
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H