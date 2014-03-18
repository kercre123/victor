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

namespace Anki {
  namespace Cozmo {
    
    class BlockWorld;
    class RobotManager;
    class Robot;
    class Block;
    
    typedef enum {
      BM_PickAndPlace
    } BehaviorMode;
    
    class BehaviorManager
    {
    public:
      BehaviorManager();
      
      void Init(RobotManager* robotMgr, BlockWorld* world);
      
      void StartMode(BehaviorMode mode);
      
      void Update();
      
    protected:
      
      typedef enum {
        WAITING_FOR_ROBOT,
        
        // PickAndPlaceBlock
        WAITING_FOR_PICKUP_BLOCK,
        EXECUTING_PATH_TO_DOCK_POSE,
        EXECUTING_DOCK,
        WAITING_FOR_PLACEMENT_BLOCK,
        EXECUTING_PATH_TO_PLACEMENT_POSE,
        EXECUTING_PLACEMENT
        
      } BehaviorState;
      
      RobotManager* robotMgr_;
      BlockWorld* world_;
      
      BehaviorState state_, nextState_;
      void (BehaviorManager::*updateFcn_)();
      
      Robot* robot_;

      void Reset();
      
      //bool GetPathToPreDockPose(const Block& b, Path& p);
      
      // Behavior state machines
      void Update_PickAndPlaceBlock();
      
      
    }; // class BehaviorManager
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H