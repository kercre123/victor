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
        
    BehaviorManager::BehaviorManager() :
    robotMgr_(nullptr), world_(nullptr)
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
      robot_ = NULL;
    }
    
    
    // TODO: Make this a blockWorld function?
    void BehaviorManager::SelectNextBlockOfInterest()
    {
      bool currBlockOfInterestFound = false;
      bool newBlockOfInterestSet = false;
      u32 numTotalObjects = 0;
      
      // Iterate through the Object
      BlockWorld::ObjectsMap_t blockMap = world_->GetAllExistingBlocks();
      for (auto const & blockType : blockMap) {
        numTotalObjects += blockType.second.size();
        
        PRINT_INFO("currType: %d\n", blockType.first);
        BlockWorld::ObjectsMapByID_t blockMapByID = blockType.second;
        for (auto const & block : blockMapByID) {

          PRINT_INFO("currID: %d\n", block.first);
          if (currBlockOfInterestFound) {
            // Current block of interest has been found.
            // Set the new block of interest to the next block in the list.
            blockOfInterest_ = block.first;
            newBlockOfInterestSet = true;
            PRINT_INFO("new block found: id %d  type %d\n", block.first, blockType.first);
            break;
          } else if (block.first == blockOfInterest_) {
            currBlockOfInterestFound = true;
            PRINT_INFO("curr block found: id %d  type %d\n", block.first, blockType.first);
          }
        }
        if (newBlockOfInterestSet)
          break;
      }
      
      // If the current block of interest was found, but a new one was not set
      // it must have been the last block in the map. Set the new block of interest
      // to the first block in the map as long as it's not the same block.
      if (!currBlockOfInterestFound || !newBlockOfInterestSet) {
        
        // Find first block
        ObjectID_t firstBlock = u16_MAX;
        for (auto const & blockType : blockMap) {
          for (auto const & block : blockType.second) {
            firstBlock = block.first;
            break;
          }
          if (firstBlock != u16_MAX) {
            break;
          }
        }

        
        if (firstBlock == blockOfInterest_){
          PRINT_INFO("Only one block in existence.");
        } else {
          PRINT_INFO("Setting block of interest to first block\n");
          blockOfInterest_ = firstBlock;
        }
      }
      
      PRINT_INFO("Block of interest: id %d  (total objects %d)\n", blockOfInterest_, numTotalObjects);
      
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
          if (robot_->ExecutePathToPose(p) == RESULT_OK)
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
