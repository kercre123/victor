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
    
    typedef enum {
      WAITING_FOR_ROBOT,
      
      // PickAndPlaceBlock
      WAIT_FOR_TWO_SINGLE_BLOCKS,
      EXECUTE_PATH_TO_DOCK_POSE,
      EXECUTE_DOCK
      
      
    } BehaviorState;
    
    
    class BehaviorManager
    {
    public:
      BehaviorManager();
      
      void Init(RobotManager* robotMgr, BlockWorld* world);
      
      void Update();
      
    protected:
      RobotManager* robotMgr_;
      BlockWorld* world_;
      
      BehaviorState state_;
      Robot* robot_;
      
      // Behavior state machines
      void Update_PickAndPlaceBlock();
      
      
    }; // class BehaviorManager
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // BEHAVIOR_MANAGER_H