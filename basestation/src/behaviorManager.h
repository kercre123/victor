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

namespace Anki {
  namespace Cozmo {
    
    class BlockWorld;
    class RobotManager;
    class Robot;
    class Block;
    
    typedef enum {
      BM_None,
      BM_PickAndPlace
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
        
        // PickAndPlaceBlock
        WAITING_FOR_DOCK_BLOCK,
        EXECUTING_PATH_TO_DOCK_POSE,
        CONFIRM_BLOCK_IS_VISIBLE,
        EXECUTING_DOCK
        
      } BehaviorState;
      
      RobotManager* robotMgr_;
      BlockWorld* world_;
      
      BehaviorState state_, nextState_;
      void (BehaviorManager::*updateFcn_)();
      
      Robot* robot_;

      // Block that the robot is currently travelling to, docking to,
      ObjectID_t blockOfInterest_;
      
      void Reset();
      
      //bool GetPathToPreDockPose(const Block& b, Path& p);
      
      // Behavior state machines
      void Update_PickAndPlaceBlock();
      
      
      /////// PickAndPlace vars ///////
    
      // Block to dock with
      Block* dockBlock_;
      const Vision::KnownMarker *dockMarker_;
      
      // Target pose for predock
      Pose3d nearestPreDockPose_;
      
      // A general time value for gating state transitions
      double waitUntilTime_;
      
    }; // class BehaviorManager
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H