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
#include "soundManager.h"

#include "anki/common/basestation/general.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/shared/utilities_shared.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/robot/cozmoConfig.h"

namespace Anki {
  namespace Cozmo {
        
    BehaviorManager::BehaviorManager()
    : robotMgr_(nullptr)
    , world_(nullptr)
    , distThresh_mm_(20.f)
    , angThresh_(DEG_TO_RAD(10))
    , blockToPickUp_(Vision::MARKER_UNKNOWN)
    , blockToPlaceOn_(Vision::MARKER_UNKNOWN)
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
      Reset();
      
      switch(mode) {
        case BM_None:
          break;
        case BM_PickAndPlace:
          printf("Starting PickAndPlace behvior\n");
          nextState_ = WAITING_FOR_DOCK_BLOCK;
          updateFcn_ = &BehaviorManager::Update_PickAndPlaceBlock;
          break;
        case BM_June2014DiceDemo:
          printf("Starting June demo behvior\n");
          state_     = WAITING_FOR_ROBOT;
          nextState_ = WAITING_TO_SEE_DICE;
          updateFcn_ = &BehaviorManager::Update_June2014DiceDemo;
          
          if (rand() % 2) {
            SoundManager::getInstance()->Play(SOUND_INPUT);
          } else {
            SoundManager::getInstance()->Play(SOUND_SWEAR);
          }
          
          break;
        default:
          PRINT_NAMED_ERROR("BehaviorManager.InvalidMode", "Invalid behavior mode");
          return;
      }
      
      //assert(updateFcn_ != nullptr);
      
    } // StartMode()
    
    void BehaviorManager::Reset()
    {
      state_ = WAITING_FOR_ROBOT;
      nextState_ = state_;
      updateFcn_ = nullptr;
      robot_ = nullptr;
      
      // Pick and Place
      
      // June2014DiceDemo
      blockToPickUp_  = Vision::MARKER_UNKNOWN;
      blockToPlaceOn_ = Vision::MARKER_UNKNOWN;
      
    } // Reset()
    
    
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
        for (auto const & object : blockMapByID) {

          const Block* block = dynamic_cast<Block*>(object.second);
          if(block != nullptr && !block->GetIsBeingCarried())
          {
            //PRINT_INFO("currID: %d\n", block.first);
            if (currBlockOfInterestFound) {
              // Current block of interest has been found.
              // Set the new block of interest to the next block in the list.
              blockOfInterest_ = block->GetID();
              newBlockOfInterestSet = true;
              //PRINT_INFO("new block found: id %d  type %d\n", block.first, blockType.first);
              break;
            } else if (block->GetID() == blockOfInterest_) {
              currBlockOfInterestFound = true;
              //PRINT_INFO("curr block found: id %d  type %d\n", block.first, blockType.first);
            }
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
          for (auto const & object : blockType.second) {
            const Block* block = dynamic_cast<Block*>(object.second);
            if(block != nullptr && !block->GetIsBeingCarried())
            {
              firstBlock = object.first;
              break;
            }
          }
          if (firstBlock != u16_MAX) {
            break;
          }
        }

        
        if (firstBlock == blockOfInterest_ || firstBlock == u16_MAX){
          //PRINT_INFO("Only one block in existence.");
        } else {
          //PRINT_INFO("Setting block of interest to first block\n");
          blockOfInterest_ = firstBlock;
        }
      }
      
      PRINT_INFO("Block of interest: id %d  (total objects %d)\n", blockOfInterest_, numTotalObjects);
      
      /*
      // Draw BOI
      const Block* block = dynamic_cast<Block*>(world_->GetObservableObjectByID(blockOfInterest_));
      if(block == nullptr) {
        PRINT_INFO("Failed to find/draw block of interest!\n");
      } else {

        static ObjectID_t prev_boi = 0;      // Previous block of interest
        static size_t prevNumPreDockPoses = 0;  // Previous number of predock poses

        // Get predock poses
        std::vector<Block::PoseMarkerPair_t> poses;
        block->GetPreDockPoses(PREDOCK_DISTANCE_MM, poses);
        
        // Erase previous predock pose marker for previous block of interest
        if (prev_boi != blockOfInterest_ || poses.size() != prevNumPreDockPoses) {
          PRINT_INFO("BOI %d (prev %d), numPoses %d (prev %zu)\n", blockOfInterest_, prev_boi, (u32)poses.size(), prevNumPreDockPoses);
          VizManager::getInstance()->EraseVizObjectType(VIZ_PREDOCKPOSE);
          prev_boi = blockOfInterest_;
          prevNumPreDockPoses = poses.size();
        }
        
        // Draw predock poses
        u32 poseID = 0;
        for(auto pose : poses) {
          VizManager::getInstance()->DrawPreDockPose(6*block->GetID()+poseID++, pose.first, VIZ_COLOR_PREDOCKPOSE);
          ++poseID;
        }
      
        // Draw cuboid
        VizManager::getInstance()->DrawCuboid(block->GetID(),
                                              block->GetSize(),
                                              block->GetPose().getWithRespectTo(Pose3d::World),
                                              VIZ_COLOR_SELECTED_OBJECT);
      }
       */
    } // SelectNextBlockOfInterest()
    
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

      
    } // Update()
    
    
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
          Vision::ObservableObject* oObject = world_->GetObservableObjectByID(blockOfInterest_);
          if (oObject == nullptr) {
            break;
          }
          
          // Check that we're not already carrying a block if the block of interest is a high block.
          if (oObject->GetPose().get_translation().z() > 44.f && robot_->IsCarryingBlock()) {
            PRINT_INFO("Already carrying block. Can't dock to high block. Aborting (0).\n");
            StartMode(BM_None);
            return;
          }

          if(robot_->ExecuteDockingSequence(dynamic_cast<Block*>(oObject)) != RESULT_OK) {
            PRINT_INFO("Robot::ExecuteDockingSequence() failed. Aborting.\n");
            StartMode(BM_None);
            return;
          }
          
          state_ = EXECUTING_DOCK;
          
          break;
        }
        case EXECUTING_DOCK:
        {
          // Wait until robot finishes (goes back to IDLE)
          if(robot_->GetState() == Robot::IDLE) {
            StartMode(BM_None);
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
            
            // Check to see if the dice block has been gone for long enough
            const TimeStamp_t timeSinceSeenDice_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - diceDeletionTime_;
            if(timeSinceSeenDice_ms > TimeBetweenDice_ms) {
              
              // TODO: Do a nod or some kind of acknowledgement we're ready to see next dice
              CoreTechPrint("First dice is gone: ready for next dice!\n");
              state_ = WAITING_TO_SEE_DICE;
            }
          } else {
            // Keep clearing blocks until we don't see them anymore
            CoreTechPrint("Please move first dice away.\n");
            world_->ClearBlocksByType(Block::DICE_BLOCK_TYPE);
            diceDeletionTime_ = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
          }
          break;
        }
          
        case WAITING_TO_SEE_DICE:
        {
          /*
          // DEBUG!!!
          blockToPickUp_ = Block::NUMBER5_BLOCK_TYPE;
          blockToPlaceOn_ = Block::NUMBER6_BLOCK_TYPE;
          state_ = BEGIN_EXPLORING;
          break;
          */
          
          // Wait for robot to be IDLE
          if(robot_->GetState() == Robot::IDLE)
          {
            const BlockWorld::ObjectsMapByID_t& diceBlocks = world_->GetExistingBlocks(Block::DICE_BLOCK_TYPE);
            if(!diceBlocks.empty()) {
              
              if(diceBlocks.size() > 1) {
                // Multiple dice blocks in the world, keep deleting them all
                // until we only see one
                CoreTechPrint("More than one dice block found!\n");
                world_->ClearBlocksByType(Block::DICE_BLOCK_TYPE);
                
              } else {
                
                Block* diceBlock = dynamic_cast<Block*>(diceBlocks.begin()->second);
                CORETECH_ASSERT(diceBlock != nullptr);
                
                // Get all the observed markers on the dice and look for the one
                // facing up (i.e. the one that is nearly aligned with the z axis)
                // TODO: expose the threshold here?
                const TimeStamp_t timeWindow = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - 500;
                const f32 dotprodThresh = 1.f - cos(DEG_TO_RAD(20));
                std::vector<const Vision::KnownMarker*> diceMarkers;
                diceBlock->GetObservedMarkers(diceMarkers, timeWindow);
                
                const Vision::KnownMarker* topMarker = nullptr;
                for(auto marker : diceMarkers) {
                  //const f32 dotprod = DotProduct(marker->ComputeNormal(), Z_AXIS_3D);
                  Pose3d markerWrtRobotOrigin;
                  if(marker->GetPose().getWithRespectTo(robot_->GetPose().FindOrigin(), markerWrtRobotOrigin) == false) {
                    PRINT_NAMED_ERROR("BehaviorManager.Update_June2014DiceDemo.MarkerOriginNotRobotOrigin",
                                      "Marker should share the same origin as the robot that observed it.\n");
                    Reset();
                  }
                  const f32 dotprod = marker->ComputeNormal(markerWrtRobotOrigin).z();
                  if(NEAR(dotprod, 1.f, dotprodThresh)) {
                    topMarker = marker;
                  }
                }
                
                if(topMarker != nullptr) {
                  // We found and observed the top marker on the dice. Use it to
                  // set which block we are looking for.
                  
                  // Don't forget to remove the dice as an ignore type for
                  // planning, since we _do_ want to avoid it as an obstacle
                  // when driving to pick and place blocks
                  robot_->GetPathPlanner()->RemoveIgnoreType(Block::DICE_BLOCK_TYPE);
                  
                  BlockID_t blockToLookFor = Block::UNKNOWN_BLOCK_TYPE;
                  switch(static_cast<Vision::MarkerType>(topMarker->GetCode()))
                  {
                    case Vision::MARKER_DICE1:
                    {
                      blockToLookFor = Block::NUMBER1_BLOCK_TYPE;
                      break;
                    }
                    case Vision::MARKER_DICE2:
                    {
                      blockToLookFor = Block::NUMBER2_BLOCK_TYPE;
                      break;
                    }
                    case Vision::MARKER_DICE3:
                    {
                      blockToLookFor = Block::NUMBER3_BLOCK_TYPE;
                      break;
                    }
                    case Vision::MARKER_DICE4:
                    {
                      blockToLookFor = Block::NUMBER4_BLOCK_TYPE;
                      break;
                    }
                    case Vision::MARKER_DICE5:
                    {
                      blockToLookFor = Block::NUMBER5_BLOCK_TYPE;
                      break;
                    }
                    case Vision::MARKER_DICE6:
                    {
                      blockToLookFor = Block::NUMBER6_BLOCK_TYPE;
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
                                Vision::MarkerTypeStrings[topMarker->GetCode()]);
                  
                  if(blockToPickUp_ == Vision::MARKER_UNKNOWN) {
                    
                    blockToPickUp_ = blockToLookFor;
                    blockToPlaceOn_ = Vision::MARKER_UNKNOWN;
                    
                    CoreTechPrint("Set blockToPickUp = %s\n",
                                  Block::IDtoStringLUT[blockToPickUp_].c_str());
                    
                    // Wait for first dice to disappear
                    state_ = WAITING_FOR_DICE_TO_DISAPPEAR;
                    
                  } else {
                    blockToPlaceOn_ = blockToLookFor;
                    
                    CoreTechPrint("Set blockToPlaceOn = %s\n",
                                  Block::IDtoStringLUT[blockToPlaceOn_].c_str());
                    
                    // Back away from dice, so we don't bump it when we blindly spin in
                    // exploring mode and so we aren't within the padded bounding
                    // box of the dice if/when we start planning a path towards
                    // the block to pick up.
                    // TODO: This may not be necessary once we use the planner to explore
                    robot_->DriveWheels(-20.f, -20.f);
                    
                    //goalPose_ = dockBlock_->GetPose();
                    desiredBackupDistance_ = (0.5f*diceBlock->GetSize().Length() +
                                              ROBOT_BOUNDING_RADIUS + 20.f);
                    
                    state_ = BACKING_UP;
                    nextState_ = BEGIN_EXPLORING; // when done backing up
                    
                    SoundManager::getInstance()->Play(SOUND_NOPROBLEMO);
                  }
                  
                } else {
                  
                  CoreTechPrint("Found dice, but not its top marker.\n");
                  
                  //dockBlock_ = dynamic_cast<Block*>(diceBlock);
                  //CORETECH_THROW_IF(dockBlock_ == nullptr);
                  
                  // Try driving closer to dice
                  // Since we are purposefully trying to get really close to the
                  // dice, ignore it as an obstacle.  We'll consider an obstacle
                  // again later, when we start driving around to pick and place.
                  robot_->GetPathPlanner()->AddIgnoreType(Block::DICE_BLOCK_TYPE);
                  const f32 diceViewingHeadAngle = DEG_TO_RAD(-15);
                  
                  Vec3f position( robot_->GetPose().get_translation() );
                  position -= diceBlock->GetPose().get_translation();
                  f32 actualDistToDice = position.Length();
                  f32 desiredDistToDice = ROBOT_BOUNDING_X_FRONT + 0.5f*diceBlock->GetSize().Length() + 5.f;

                  if (actualDistToDice > desiredDistToDice + 5) {
                    position.MakeUnitLength();
                    position *= desiredDistToDice;
                  
                    Radians angle = atan2(position.y(), position.x()) + PI_F;
                    position += diceBlock->GetPose().get_translation();
                    
                    goalPose_ = Pose3d(angle, Z_AXIS_3D, {{position.x(), position.y(), 0.f}});
                    
                    robot_->ExecutePathToPose(goalPose_, diceViewingHeadAngle);
                  } else {
                    CoreTechPrint("Move dice closer!\n");
                  }
                  
                } // IF / ELSE top marker seen
                
              } // IF only one dice
              
            } // IF any diceBlocks available
          } // IF robot is IDLE
          
          break;
        } // case WAITING_FOR_FIRST_DICE
          
        case BACKING_UP:
        {
          const f32 currentDistance = (robot_->GetPose().get_translation() -
                                       goalPose_.get_translation()).Length();
          
          if(currentDistance >= desiredBackupDistance_ )
          {
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
            robot_->DriveWheels(0.f, 0.f);
            state_ = nextState_;
          }
          
          break;
        } // case BACKING_UP

        case BEGIN_EXPLORING:
        {
          // For now, "exploration" is just spinning in place to
          // try to locate blocks
          if(!robot_->IsMoving() && waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            PRINT_INFO("Beginning exploring\n");
            robot_->DriveWheels(15.f, -15.f);
            robot_->MoveHeadToAngle(DEG_TO_RAD(-5), 1, 1);
            
            if(robot_->IsCarryingBlock()) {
              blockOfInterest_ = blockToPlaceOn_;
            } else {
              blockOfInterest_ = blockToPickUp_;
            }
            
            state_ = EXPLORING;
          }
          
          break;
        } // case BEGIN_EXPLORING
          
        case EXPLORING:
        {
          // If we've spotted the block we're looking for, stop exploring, and
          // execute a path to that block
          const BlockWorld::ObjectsMapByID_t& blocks = world_->GetExistingBlocks(blockOfInterest_);
          if(!blocks.empty()) {
            // Dock with the first block of the right type that we see
            // TODO: choose the closest?
            Block* dockBlock = dynamic_cast<Block*>(blocks.begin()->second);
            CORETECH_THROW_IF(dockBlock == nullptr);
            
            robot_->DriveWheels(0.f, 0.f);
            
            robot_->ExecuteDockingSequence(dockBlock);
            
            state_ = EXECUTING_DOCK;
          }
          
          break;
        } // EXPLORING
   
        case EXECUTING_DOCK:
        {
          // Wait for the robot to go back to IDLE
          if(robot_->GetState() == Robot::IDLE)
          {
            const bool donePickingUp = robot_->IsCarryingBlock() && robot_->GetCarryingBlock()->GetType() == blockToPickUp_;
            if(donePickingUp) {
              PRINT_INFO("Picked up block '%s' successfully! Going back to exploring for block to place on.\n",
                         robot_->GetCarryingBlock()->GetName().c_str());
              
              state_ = BEGIN_EXPLORING;
              
              SoundManager::getInstance()->Play(SOUND_NOTIMPRESSED);
              
              return;
            } // if donePickingUp
            
            const bool donePlacing = !robot_->IsCarryingBlock() && robot_->GetDockBlock() && robot_->GetDockBlock()->GetType() == blockToPlaceOn_;
            if(donePlacing) {
              PRINT_INFO("Placed block %d on %d successfully! Going back to waiting for dice.\n",
                         blockToPickUp_, blockToPlaceOn_);
              
              if (rand() %2) {
                SoundManager::getInstance()->Play(SOUND_60PERCENT);
              } else {
                SoundManager::getInstance()->Play(SOUND_TADA);
              }
              
              StartMode(BM_June2014DiceDemo);
              
              return;
            } // if donePlacing
            
            
            // Either pickup or placement failed
            const bool pickupFailed = !robot_->IsCarryingBlock();
            if (pickupFailed) {
              PRINT_INFO("Block pickup failed. Retrying...\n");
            } else {
              PRINT_INFO("Block placement failed. Retrying...\n");
            }
            
            // Backup to re-explore the block
            robot_->DriveWheels(-20.f, -20.f);
            state_ = BACKING_UP;
            nextState_ = BEGIN_EXPLORING;
            desiredBackupDistance_ = 30;
            goalPose_ = robot_->GetPose();
            
            SoundManager::getInstance()->Play(SOUND_STARTOVER);
            
          } // if robot IDLE
          
          break;
        } // case EXECUTING_DOCK
          
        default:
        {
          PRINT_NAMED_ERROR("BehaviorManager.UnknownBehaviorState",
                            "Transitioned to unknown state %d!\n", state_);
          StartMode(BM_None);
          return;
        }
      } // switch(state_)
      
    } // Update_June2014DiceDemo()

    
  } // namespace Cozmo
} // namespace Anki
