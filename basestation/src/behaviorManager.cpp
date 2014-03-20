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
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/common/basestation/general.h"

namespace Anki {
  namespace Cozmo {
        
    BehaviorManager::BehaviorManager()
    {
      Reset();
    }

    void BehaviorManager::Init(RobotManager* robotMgr, BlockWorld* world)
    {
      robotMgr_ = robotMgr;
      world_ = world;
    }
    
    void BehaviorManager::StartMode(BehaviorMode mode)
    {
      switch(mode) {
        case BM_PickAndPlace:
          nextState_ = WAITING_FOR_PICKUP_BLOCK;
          updateFcn_ = &BehaviorManager::Update_PickAndPlaceBlock;
          break;
        default:
          PRINT_NAMED_ERROR("BehaviorManager.InvalidMode", "Invalid behavior mode");
          return;
      }
      
      assert(updateFcn_ != NULL);
    }
    
    void BehaviorManager::Reset()
    {
      state_ = WAITING_FOR_ROBOT;
      nextState_ = state_;
      updateFcn_ = NULL;
    }
    
    void BehaviorManager::Update()
    {
      // Shared states
      switch(state_) {
        case WAITING_FOR_ROBOT:
        {
          const std::vector<RobotID_t> &robotList = robotMgr_->GetRobotIDList();
          if (!robotList.empty()) {
            robot_ = robotMgr_->GetRobotByID(robotList[0]);
            state_ = nextState_;
          }
          break;
        }
        default:
          (this->*updateFcn_)();
          break;
      }

    }
    
    
    // Returns true if successfully found a path to pre-dock pose for specified block.
    //bool BehaviorManager::GetPathToPreDockPose(const Block& b, Path& p)
    //{    }
    
    
    /********************************************************
     * PickAndPlaceBlock
     *
     * Looks for a particular block in the world. When it sees 
     * that it is at ground-level it 
     * 1) Plans a path to a docking pose for that block
     * 2) Docks with the block
     * 3) Places it on any other block in the world
     *
     ********************************************************/
    void BehaviorManager::Update_PickAndPlaceBlock()
    {

      const Pose2d p_2d(0, 200, 400);
      const Pose3d p(p_2d);
      const float distThresh = 50;
      const Radians angThresh(0.1);
      
      
      const Block::Type PICKUP_BLOCK_TYPE = Block::Type::BULLSEYE_BLOCK_TYPE;
      const Block::Type PLACEMENT_BLOCK_TYPE = Block::Type::FUEL_BLOCK_TYPE;
      
      switch(state_) {
        case WAITING_FOR_PICKUP_BLOCK:
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
            state_ = EXECUTING_PATH_TO_DOCK_POSE;
          
          break;
        }
        case EXECUTING_PATH_TO_DOCK_POSE:
        {
          // Once robot is confirmed at the docking pose, execute docking.
          if(!robot_->IsTraversingPath()) {
            if (robot_->get_pose().IsSameAs(p, distThresh, angThresh)) {
              PRINT_INFO("Dock pose reached\n");
              
              // TODO: Confirm that expected marker is within view
              // ...
              
              // TODO: Get marker info
              f32 markerWidth = 30;

              
              // Start dock
              PRINT_INFO("Picking up block with marker %d\n", PICKUP_BLOCK_TYPE);
              robot_->SendDockWithBlock(PICKUP_BLOCK_TYPE, markerWidth, DA_PICKUP_LOW);
              state_ = EXECUTING_DOCK;
              
            } else {
              PRINT_INFO("Not at expected position at the end of the path. Looking for docking block again\n.");
              state_ = WAITING_FOR_PICKUP_BLOCK;
            }
          }
          break;
        }
        case EXECUTING_DOCK:
        {
          if (!robot_->IsTraversingPath()) {
            // Stopped executing docking path. Did it successfully dock?
            if (robot_->IsCarryingBlock()) {
              PRINT_INFO("Pickup successful! Looking for placement block\n");
              state_ = WAITING_FOR_PLACEMENT_BLOCK;
            } else {
              
            }
          }
          break;
        }
        case WAITING_FOR_PLACEMENT_BLOCK:
        {
          auto observedBlocks = world_->GetExistingBlocks(PLACEMENT_BLOCK_TYPE);
          for (auto & observedBlock : observedBlocks) {
            // Take first block in the list and get docking poses.
            // ...
            
            // Plan a path to a docking pose
            // ...
            
            state_ = EXECUTING_PATH_TO_PLACEMENT_POSE;
            break;
          }
          break;
        }
        case EXECUTING_PATH_TO_PLACEMENT_POSE:
        {
          break;
        }
        case EXECUTING_PLACEMENT:
        {
          break;
        }
        default:
          PRINT_NAMED_ERROR("BehaviorManager.UnknownBehaviorState", "Transitioned to unknown state %d!", state_);
          break;
      }
      
    }
    
  } // namespace Cozmo
} // namespace Anki
