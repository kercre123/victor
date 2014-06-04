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
#include "vizManager.h"


#include "anki/common/basestation/general.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/shared/utilities_shared.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/robot/cozmoConfig.h"

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
        case BM_None:
          state_ = WAITING_FOR_ROBOT;
          nextState_ = state_;
          updateFcn_ = nullptr;
          break;
        case BM_PickAndPlace:
          dockBlock_ = nullptr;
          nextState_ = WAITING_FOR_DOCK_BLOCK;
          updateFcn_ = &BehaviorManager::Update_PickAndPlaceBlock;
          break;
        case BM_June2014DiceDemo:
          state_     = WAITING_FOR_ROBOT;
          nextState_ = WAITING_TO_SEE_DICE;
          updateFcn_ = &BehaviorManager::Update_June2014DiceDemo;
          break;
        default:
          PRINT_NAMED_ERROR("BehaviorManager.InvalidMode", "Invalid behavior mode");
          return;
      }
      
      //assert(updateFcn_ != nullptr);
    }
    
    void BehaviorManager::Reset()
    {
      state_ = WAITING_FOR_ROBOT;
      nextState_ = state_;
      updateFcn_ = nullptr;
      robot_ = nullptr;
      
      dockBlock_ = nullptr;
    }
    
    
    // TODO: Make this a blockWorld function?
    void BehaviorManager::SelectNextBlockOfInterest()
    {
      bool currBlockOfInterestFound = false;
      bool newBlockOfInterestSet = false;
      u32 numTotalObjects = 0;
      
      // Iterate through the Object
      auto const & blockMap = world_->GetAllExistingBlocks();
      for (auto const & blockType : blockMap) {
        numTotalObjects += blockType.second.size();
        
        //PRINT_INFO("currType: %d\n", blockType.first);
        auto const & blockMapByID = blockType.second;
        for (auto const & block : blockMapByID) {

          //PRINT_INFO("currID: %d\n", block.first);
          if (currBlockOfInterestFound) {
            // Current block of interest has been found.
            // Set the new block of interest to the next block in the list.
            blockOfInterest_ = block.first;
            newBlockOfInterestSet = true;
            //PRINT_INFO("new block found: id %d  type %d\n", block.first, blockType.first);
            break;
          } else if (block.first == blockOfInterest_) {
            currBlockOfInterestFound = true;
            //PRINT_INFO("curr block found: id %d  type %d\n", block.first, blockType.first);
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
          //PRINT_INFO("Only one block in existence.");
        } else {
          //PRINT_INFO("Setting block of interest to first block\n");
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
          if (updateFcn_) {
            (this->*updateFcn_)();
          } else {
            state_ = nextState_ = WAITING_FOR_ROBOT;
          }
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
      // Params for determining whether the predock pose has been reached
      const f32 distThresh_mm = 20;
      const Radians angThresh(0.15);

      
      switch(state_) {
        case WAITING_FOR_DOCK_BLOCK:
        {
          // TODO: BlockWorld needs function for retrieving collision-free docking poses for a given block
          //
          // 1) Get all blocks in the world
          // 2) Find one that is on the bottom level and no block is on top
          // 3) Get collision-free docking poses
          // 4) Find a collision-free path to one of the docking poses
          // 5) Command path to that docking pose
          
          
          // Get block object
          const Vision::ObservableObject* oObject = world_->GetObservableObjectByID(blockOfInterest_);
          if (oObject == nullptr) {
            break;
          }
          dockBlock_ = (Block*)oObject;
        
          
          // Check that we're not already carrying a block if the block of interest is a high block.
          if (dockBlock_->GetPose().get_translation().z() > 44.f && robot_->IsCarryingBlock()) {
            PRINT_INFO("Already carrying block. Can't dock to high block. Aborting (0).\n");
            StartMode(BM_None);
            return;
          }
          

          // Get predock poses
          std::vector<Block::PoseMarkerPair_t> preDockPoseMarkerPairs;
          dockBlock_->GetPreDockPoses(PREDOCK_DISTANCE_MM, preDockPoseMarkerPairs);
         
          
          // Select (closest) predock pose
          const Pose3d& robotPose = robot_->get_pose();

          if (!preDockPoseMarkerPairs.empty()) {
            f32 shortestDist2Pose = -1;
            for (auto const & poseMarkerPair : preDockPoseMarkerPairs) {
              
              PRINT_INFO("Candidate pose: (%f %f %f), (%f %f %f %f)\n",
                         poseMarkerPair.first.get_translation().x(),
                         poseMarkerPair.first.get_translation().y(),
                         poseMarkerPair.first.get_translation().z(),
                         poseMarkerPair.first.get_rotationAxis().x(),
                         poseMarkerPair.first.get_rotationAxis().y(),
                         poseMarkerPair.first.get_rotationAxis().z(),
                         poseMarkerPair.first.get_rotationAngle().ToFloat() );
              
              f32 dist2Pose = computeDistanceBetween(poseMarkerPair.first, robotPose);
              if (dist2Pose < shortestDist2Pose || shortestDist2Pose < 0) {
                shortestDist2Pose = dist2Pose;
                nearestPreDockPose_ = poseMarkerPair.first;
                dockMarker_ = &(poseMarkerPair.second);
              }
            }
          } else {
            PRINT_INFO("No docking pose found\n");
            StartMode(BM_None);
            return;
          }

          
          // Generate path to predock pose
          // TODO: Generate collision-free path!!!
          if (robot_->get_pose().IsSameAs(nearestPreDockPose_, distThresh_mm, angThresh) ||
              robot_->ExecutePathToPose(nearestPreDockPose_) == RESULT_OK) {
            // Make sure head is tilted down so that it can localize well
            robot_->MoveHeadToAngle(-0.26, 5, 10);
            PRINT_INFO("Executing path to pose %f %f %f\n",
                       nearestPreDockPose_.get_translation().x(),
                       nearestPreDockPose_.get_translation().y(),
                       nearestPreDockPose_.get_rotationAngle().ToFloat());
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
            state_ = EXECUTING_PATH_TO_DOCK_POSE;
          }
          
          break;
        }
        case EXECUTING_PATH_TO_DOCK_POSE:
        {
          // Once robot is confirmed at the docking pose, execute docking.
          if(!robot_->IsTraversingPath() && waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            
            if (robot_->get_pose().IsSameAs(nearestPreDockPose_, distThresh_mm, angThresh)) {
              PRINT_INFO("Dock pose reached\n");

              // Verify that the block of interest still exists in case it somehow disappeared while
              // the robot was travelling to the predock pose.
              const Vision::ObservableObject* oObject = world_->GetObservableObjectByID(blockOfInterest_);
              if (oObject == nullptr) {
                PRINT_INFO("Block of interest no longer present.\n");
                state_ = WAITING_FOR_DOCK_BLOCK;
                break;
              }

              
              // Need to confirm that expected marker is within view.
              // Move head if necessary based on block height.
              f32 dockBlockHeight = dockBlock_->GetPose().get_translation().z();
              if (dockBlockHeight > 44.f) {
                robot_->MoveHeadToAngle(0.25, 1, 1);
              } else {
                robot_->MoveHeadToAngle(-0.25, 1, 1);
              }
              
              // Wait long enough for head to move and a message with the expected marker to be received.
              waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;

              state_ = CONFIRM_BLOCK_IS_VISIBLE;
              
            } else {
              PRINT_INFO("Not at expected position at the end of the path. Looking for docking block again (Robot: %f %f %f %f, Dock %f %f %f %f)\n.",
                         robot_->get_pose().get_translation().x(),
                         robot_->get_pose().get_translation().y(),
                         robot_->get_pose().get_translation().z(),
                         robot_->get_pose().get_rotationAngle().ToFloat(),
                         nearestPreDockPose_.get_translation().x(),
                         nearestPreDockPose_.get_translation().y(),
                         nearestPreDockPose_.get_translation().z(),
                         nearestPreDockPose_.get_rotationAngle().ToFloat()
                         );
              StartMode(BM_None);
              return;
            }
          }
          break;
        }
        case CONFIRM_BLOCK_IS_VISIBLE:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > waitUntilTime_) {
            // TODO: Check that the marker was recently seen at roughly the expected image location
            // ...
            //const Point2f& imgCorners = dockMarker_->GetImageCorners().computeCentroid();
            // For now, just docking to the marker no matter where it is in the image.

            
            // Get dock action
            f32 dockBlockHeight = dockBlock_->GetPose().get_translation().z();
            dockAction_ = DA_PICKUP_LOW;
            if (dockBlockHeight > dockBlock_->GetSize().z()) {
              if (robot_->IsCarryingBlock()) {
                PRINT_INFO("Already carrying block. Can't dock to high block. Aborting.\n");
                StartMode(BM_None);
                return;
              } else {
                dockAction_ = DA_PICKUP_HIGH;
              }
            } else if (robot_->IsCarryingBlock()) {
              dockAction_ = DA_PLACE_HIGH;
            }
            
            // Start dock
            PRINT_INFO("Docking with marker %d (action = %d)\n", dockMarker_->GetCode(), dockAction_);
            robot_->DockWithBlock(dockMarker_->GetCode(),  dockMarker_->GetSize(), dockAction_);
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5;
            state_ = EXECUTING_DOCK;
          }
          break;
        }
        case EXECUTING_DOCK:
        {
          if (!robot_->IsPickingOrPlacing() && waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            // Stopped executing docking path. Did it successfully dock?
            if (((dockAction_ == DA_PICKUP_LOW || dockAction_ == DA_PICKUP_HIGH) && robot_->IsCarryingBlock()) ||
                ((dockAction_ == DA_PLACE_HIGH) && !robot_->IsCarryingBlock()) ) {
              PRINT_INFO("Docking successful!\n");
            } else {
              PRINT_INFO("Dock failed! Aborting\n");
            }
            
            StartMode(BM_None);
            return;

          }
          break;
        }
        default:
        {
          PRINT_NAMED_ERROR("BehaviorManager.UnknownBehaviorState", "Transitioned to unknown state %d!", state_);
          StartMode(BM_None);
          return;
        }
      }
      
    } // Update_PickAndPlaceBlock()
    
    
    /********************************************************
     * June2014DiceDemo
     *
     * Look for two dice rolls. Look for the block with the 
     * number corresponding to the first roll and pick it up.
     * Place it on the block with the number corresponding to
     * the second roll.
     *
     ********************************************************/
    void BehaviorManager::Update_June2014DiceDemo()
    {
      switch(state_) {
          
        case WAITING_FOR_DICE_TO_DISAPPEAR:
        {
          const BlockWorld::ObjectsMapByID_t& diceBlocks = world_->GetExistingBlocks(Block::DICE_BLOCK_TYPE);
          if(diceBlocks.empty()) {
            state_ = WAITING_TO_SEE_DICE;
          } else {
            
            if(diceBlocks.size() > 1) {
              // Multiple dice blocks in the world, just use first for now.
              // TODO: Issue warning?
              CoreTechPrint("More than one dice block found, using first!\n");
            }
            
            // Check to see if the dice the world still knows about is
            // sufficiently stale (i.e. we haven't seen it in awhile)
            // If so, tell the world to delete it and then go back to looking
            // for dice.
            const TimeStamp_t DiceObservationTimeout_ms = 2000; // 2 sec
            const TimeStamp_t timeSinceSeenDice_ms = (diceBlocks.begin()->second->GetLastObservedTime() -
                                                      BaseStationTimer::getInstance()->GetCurrentTimeStamp());
            if(timeSinceSeenDice_ms > DiceObservationTimeout_ms) {
              CoreTechPrint("Removing dice we haven't seen for awhile.\n");
              const BlockID_t blockID = diceBlocks.begin()->first;
              world_->ClearBlock(blockID);
              state_ = WAITING_TO_SEE_DICE;
            }
            
          }
          break;
        }
          
        case WAITING_TO_SEE_DICE:
        {
          const BlockWorld::ObjectsMapByID_t& diceBlocks = world_->GetExistingBlocks(Block::DICE_BLOCK_TYPE);
          if(!diceBlocks.empty()) {
            
            if(diceBlocks.size() > 1) {
              // Multiple dice blocks in the world, just use first for now.
              // TODO: Issue warning?
              CoreTechPrint("More than one dice block found, using first!\n");
            }
            
            Vision::ObservableObject* diceBlock = diceBlocks.begin()->second;
            
            // Get all the observed markers on the dice and look for the one
            // facing up (i.e. the one that is nearly aligned with the z axis)
            const f32 dotprodThresh = 1.f - cos(DEG_TO_RAD(20));
            std::vector<const Vision::KnownMarker*> diceMarkers;
            diceBlock->GetObservedMarkers(diceMarkers);
            
            const Vision::KnownMarker* topMarker = nullptr;
            for(auto marker : diceMarkers) {
              //const f32 dotprod = DotProduct(marker->ComputeNormal(), Z_AXIS_3D);
              const f32 dotprod = marker->ComputeNormal().z();
              if(NEAR(dotprod, 1.f, dotprodThresh)) {
                topMarker = marker;
              }
            }
            
            if(topMarker != nullptr) {
              // We found and observed the top marker on the dice. Use it to
              // set which block we are looking for.
              Vision::Marker::Code blockToLookFor = Vision::MARKER_UNKNOWN;
              switch(static_cast<Vision::MarkerType>(topMarker->GetCode()))
              {
                case Vision::MARKER_DICE1:
                {
                  blockToLookFor = Vision::MARKER_1;
                  break;
                }
                case Vision::MARKER_DICE2:
                {
                  blockToLookFor = Vision::MARKER_2;
                  break;
                }
                case Vision::MARKER_DICE3:
                {
                  blockToLookFor = Vision::MARKER_3;
                  break;
                }
                case Vision::MARKER_DICE4:
                {
                  blockToLookFor = Vision::MARKER_4;
                  break;
                }
                case Vision::MARKER_DICE5:
                {
                  blockToLookFor = Vision::MARKER_5;
                  break;
                }
                case Vision::MARKER_DICE6:
                {
                  blockToLookFor = Vision::MARKER_6;
                  break;
                }
                  
                default:
                  PRINT_NAMED_ERROR("BehaviorManager.UnknownDiceMarker",
                                    "Found unexpected marker on dice: %s!",
                                    Vision::MarkerTypeStrings[topMarker->GetCode()]);
                  StartMode(BM_None);
                  return;
              } // switch(topMarker->GetCode())
              
              CoreTechPrint("Found top marker on dice: %s!\n",
                            Vision::MarkerTypeStrings[blockToLookFor]);
              
              if(blockToPickUp_ == Vision::MARKER_UNKNOWN) {
                blockToPickUp_ = blockToLookFor;
                blockToPlaceOn_ = Vision::MARKER_UNKNOWN;
                // Wait for first dice to disappear
                state_ = WAITING_FOR_DICE_TO_DISAPPEAR;
              } else {
                blockToPlaceOn_ = blockToLookFor;
                
                // TODO: create a path to blockToPickUp_
                
                state_ = EXECUTING_PATH_TO_DOCK_POSE;
              }
              
            } else {
              
              // TODO: if we don't have one facing up, try driving closer to dice
              CoreTechPrint("Found dice, but not its top marker.\n");
            }
            
          } // IF any diceBlocks available
          
          break;
        } // case WAITING_FOR_FIRST_DICE

        default:
        {
          PRINT_NAMED_ERROR("BehaviorManager.UnknownBehaviorState",
                            "Transitioned to unknown state %d!", state_);
          StartMode(BM_None);
          return;
        }
      } // switch(state_)
      
    } // Update_June2014DiceDemo()

    
  } // namespace Cozmo
} // namespace Anki
