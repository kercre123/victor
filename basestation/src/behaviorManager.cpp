/**
 * File: behaviorManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "behaviorManager.h"
#include "pathPlanner.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/general.h"

namespace Anki {
  namespace Cozmo {
    
    const BehaviorState START_BEHAVIOR = WAITING_FOR_ROBOT;
    
    
    BehaviorManager::BehaviorManager()
    {
      state_ = START_BEHAVIOR;
    }

    void BehaviorManager::Init(RobotManager* robotMgr, BlockWorld* world)
    {
      robotMgr_ = robotMgr;
      world_ = world;
    }
    
    void BehaviorManager::Update()
    {
      //Update_PickAndPlaceBlock();
    }
    
    void BehaviorManager::Update_PickAndPlaceBlock()
    {
      const Pose2d p_2d(0, 200, 400);
      const Pose3d p(p_2d);
      const float distThresh = 50;
      const Radians angThresh(0.1);
      
      
      switch(state_) {
        case WAITING_FOR_ROBOT:
        {
          const std::vector<RobotID_t> &robotList = robotMgr_->GetRobotIDList();
          if (!robotList.empty()) {
            robot_ = robotMgr_->GetRobotByID(robotList[0]);
            state_ = WAIT_FOR_TWO_SINGLE_BLOCKS;
          }
          break;
        }
        case WAIT_FOR_TWO_SINGLE_BLOCKS:
        {
          // TODO: BlockWorld needs function for retrieving collision-free docking poses for a given block
          //
          // 1) Get all blocks in the world
          // 2) Find one that is on the bottom level and no block is on top
          // 3) Get collision-free docking poses
          // 4) Find a collision-free path to one of the docking poses
          // 5) Command path to that docking pose
          
          // HACK: For now just generate a random path to anywhere.
          if (robot_->ExecutePathToPose(p) == EXIT_SUCCESS)
            state_ = EXECUTE_PATH_TO_DOCK_POSE;
          
          break;
        }
        case EXECUTE_PATH_TO_DOCK_POSE:
        {
          // TODO: Once robot is confirmed at the docking pose, execute docking.
          if (robot_->get_pose().IsSameAs(p, distThresh, angThresh)) {
            printf("Dock pose reached\n");
            
            // Start dock

            state_ = EXECUTE_DOCK;
          }
          break;
        }
        case EXECUTE_DOCK:
        {
          
          break;
        }
        default:
          PRINT_NAMED_ERROR("UnknownBehaviorState", "Transitioned to unknown state!");
          break;
      }
      
    }
    
  } // namespace Cozmo
} // namespace Anki