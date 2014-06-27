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
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/robot/cozmoConfig.h"

// The angle wrt the mat at which the user is expected to be.
// For happy head-nodding demo purposes.
#define USER_LOC_ANGLE_WRT_MAT -1.57

namespace Anki {
  namespace Cozmo {
        
    BehaviorManager::BehaviorManager()
    : robotMgr_(nullptr)
    , world_(nullptr)
    , mode_(BM_None)
    , distThresh_mm_(20.f)
    , angThresh_(DEG_TO_RAD(10))
    , blockToPickUp_(Block::UNKNOWN_BLOCK_TYPE)
    , blockToPlaceOn_(Block::UNKNOWN_BLOCK_TYPE)
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
      mode_ = mode;
      switch(mode) {
        case BM_None:
          CoreTechPrint("Starting NONE behavior\n");
          break;
        case BM_PickAndPlace:
          CoreTechPrint("Starting PickAndPlace behavior\n");
          nextState_ = WAITING_FOR_DOCK_BLOCK;
          updateFcn_ = &BehaviorManager::Update_PickAndPlaceBlock;
          break;
        case BM_June2014DiceDemo:
          CoreTechPrint("Starting June demo behavior\n");
          state_     = WAITING_FOR_ROBOT;
          nextState_ = WAITING_TO_SEE_DICE;
          updateFcn_ = &BehaviorManager::Update_June2014DiceDemo;
          SoundManager::getInstance()->Play(SOUND_DEMO_START);
          break;
        default:
          PRINT_NAMED_ERROR("BehaviorManager.InvalidMode", "Invalid behavior mode");
          return;
      }
      
      //assert(updateFcn_ != nullptr);
      
    } // StartMode()
    
    BehaviorMode BehaviorManager::GetMode() const
    {
      return mode_;
    }
    
    void BehaviorManager::Reset()
    {
      state_ = WAITING_FOR_ROBOT;
      nextState_ = state_;
      updateFcn_ = nullptr;
      robot_ = nullptr;
      
      // Pick and Place
      
      // June2014DiceDemo
      blockToPickUp_  = Block::UNKNOWN_BLOCK_TYPE;
      blockToPlaceOn_ = Block::UNKNOWN_BLOCK_TYPE;
      
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
          if(block != nullptr && !block->IsBeingCarried())
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
            if(block != nullptr && !block->IsBeingCarried())
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
          Block* block = world_->GetBlockByID(blockOfInterest_);
          if (block == nullptr) {
            break;
          }
          
          // Check that we're not already carrying a block if the block of interest is a high block.
          if (block->GetPose().get_translation().z() > 44.f && robot_->IsCarryingBlock()) {
            PRINT_INFO("Already carrying block. Can't dock to high block. Aborting (0).\n");
            StartMode(BM_None);
            return;
          }

          if(robot_->ExecuteDockingSequence(block->GetID()) != RESULT_OK) {
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
            world_->ClearBlocksByType(Block::DICE_BLOCK_TYPE);
            diceDeletionTime_ = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
            if (waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
              // Keep clearing blocks until we don't see them anymore
              CoreTechPrint("Please move first dice away.\n");
              robot_->SendPlayAnimation(ANIM_HEAD_NOD, 2);
              waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 5;
              SoundManager::getInstance()->Play(SOUND_WAITING4DICE2DISAPPEAR);
            }
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
                const TimeStamp_t timeWindow = robot_->GetLastMsgTimestamp() - 500;
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
                  
                  ObjectID_t blockToLookFor = Block::UNKNOWN_BLOCK_TYPE;
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
                  
                  if(blockToPickUp_ ==  Block::UNKNOWN_BLOCK_TYPE) {
                    
                    blockToPickUp_ = blockToLookFor;
                    blockToPlaceOn_ =  Block::UNKNOWN_BLOCK_TYPE;
                    
                    CoreTechPrint("Set blockToPickUp = %s\n",
                                  Block::IDtoStringLUT[blockToPickUp_].c_str());
                    
                    // Wait for first dice to disappear
                    state_ = WAITING_FOR_DICE_TO_DISAPPEAR;
                    
                  } else {
                    blockToPlaceOn_ = blockToLookFor;
                    
                    CoreTechPrint("Set blockToPlaceOn = %s\n",
                                  Block::IDtoStringLUT[blockToPlaceOn_].c_str());

                    state_ = BEGIN_EXPLORING;
                    
                  }
                  
                  SoundManager::getInstance()->Play(SOUND_OK_GOT_IT);
                  waitUntilTime_ = 0;
                
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
            
            else {
              // Can't see dice
              if (waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
                CoreTechPrint("Show me the dice!\n");
                robot_->SendPlayAnimation(ANIM_HEAD_NOD, 1);
                SoundManager::getInstance()->Play(SOUND_WAITING4DICE);
                waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 5;
              }
            }
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
        case GOTO_EXPLORATION_POSE:
        {
          const BlockWorld::ObjectsMapByID_t& blocks = world_->GetExistingBlocks(blockOfInterest_);
          if (robot_->GetState() == Robot::IDLE || !blocks.empty()) {
            state_ = START_EXPLORING_TURN;
          }
          break;
        } // case GOTO_EXPLORATION_POSE
        case BEGIN_EXPLORING:
        {
          // For now, "exploration" is just spinning in place to
          // try to locate blocks
          if(!robot_->IsMoving() && waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            
            if(robot_->IsCarryingBlock()) {
              blockOfInterest_ = blockToPlaceOn_;
            } else {
              blockOfInterest_ = blockToPickUp_;
            }
            
            
            // If we already know where the blockOfInterest is, then go straight to it
            const BlockWorld::ObjectsMapByID_t& blocks = world_->GetExistingBlocks(blockOfInterest_);
            if(blocks.empty()) {
              // Compute desired pose at mat center
              Pose3d robotPose = robot_->GetPose();
              f32 targetAngle = atan2(robotPose.get_translation().y(), robotPose.get_translation().x()) + PI_F;
              Pose3d targetPose(targetAngle, Z_AXIS_3D, Vec3f(0,0,0));
              
              if (computeDistanceBetween(targetPose, robotPose) > 50.f) {
                PRINT_INFO("Going to mat center for exploration (%f %f %f)\n", targetPose.get_translation().x(), targetPose.get_translation().y(), targetAngle);
                robot_->GetPathPlanner()->AddIgnoreType(Block::DICE_BLOCK_TYPE);
                robot_->ExecutePathToPose(targetPose);
              }

              state_ = GOTO_EXPLORATION_POSE;
            } else {
              state_ = EXPLORING;
            }
          }
          
          break;
        } // case BEGIN_EXPLORING
        case START_EXPLORING_TURN:
        {
          PRINT_INFO("Beginning exploring\n");
          robot_->GetPathPlanner()->RemoveIgnoreType(Block::DICE_BLOCK_TYPE);
          robot_->DriveWheels(8.f, -8.f);
          robot_->MoveHeadToAngle(DEG_TO_RAD(-10), 1, 1);
          explorationStartAngle_ = robot_->GetPose().get_rotationAngle<'Z'>();
          isTurning_ = true;
          state_ = EXPLORING;
          break;
        }
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
            
            robot_->ExecuteDockingSequence(dockBlock->GetID());
            
            state_ = EXECUTING_DOCK;
            
            wasCarryingBlockAtDockingStart_ = robot_->IsCarryingBlock();
            
            PRINT_INFO("STARTING DOCKING\n");
          }
          
          // Repeat turn-stop behavior for more reliable block detection
          Radians currAngle = robot_->GetPose().get_rotationAngle<'Z'>();
          if (isTurning_ && (std::abs((explorationStartAngle_ - currAngle).ToFloat()) > DEG_TO_RAD(40))) {
            PRINT_INFO("Exploration - pause turning. Looking for %s\n", Block::IDtoStringLUT[blockOfInterest_].c_str());
            robot_->DriveWheels(0.f,0.f);
            isTurning_ = false;
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
          } else if (!isTurning_ && waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
            state_ = START_EXPLORING_TURN;
          }
          
          break;
        } // EXPLORING
   
        case EXECUTING_DOCK:
        {
          // Wait for the robot to go back to IDLE
          if(robot_->GetState() == Robot::IDLE)
          {
            const bool donePickingUp = robot_->IsCarryingBlock() &&
                                       world_->GetBlockByID(robot_->GetCarryingBlock())->GetType() == blockToPickUp_;
            if(donePickingUp) {
              PRINT_INFO("Picked up block %d successfully! Going back to exploring for block to place on.\n",
                         robot_->GetCarryingBlock());
              
              state_ = BEGIN_EXPLORING;
              
              SoundManager::getInstance()->Play(SOUND_NOTIMPRESSED);
              
              return;
            } // if donePickingUp
            
            const bool donePlacing = !robot_->IsCarryingBlock() && wasCarryingBlockAtDockingStart_;
            if(donePlacing) {
              PRINT_INFO("Placed block %d on %d successfully! Going back to waiting for dice.\n",
                         blockToPickUp_, blockToPlaceOn_);

              
              // Compute pose that makes robot face user
              Pose3d userFacingPose = robot_->GetPose();
              userFacingPose.set_rotation(USER_LOC_ANGLE_WRT_MAT, Z_AXIS_3D);
              robot_->ExecutePathToPose(userFacingPose);
              state_ = FACE_USER;
              
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
            robot_->MoveHeadToAngle(DEG_TO_RAD(-5), 10, 10);
            robot_->DriveWheels(-20.f, -20.f);
            state_ = BACKING_UP;
            nextState_ = BEGIN_EXPLORING;
            desiredBackupDistance_ = 30;
            goalPose_ = robot_->GetPose();
            
            SoundManager::getInstance()->Play(SOUND_STARTOVER);
            
          } // if robot IDLE
          
          break;
        } // case EXECUTING_DOCK
        case FACE_USER:
        {
          // Wait for the robot to go back to IDLE
          if(robot_->GetState() == Robot::IDLE)
          {
            // Start nodding
            robot_->SendPlayAnimation(ANIM_HEAD_NOD);
            state_ = HAPPY_NODDING;
            PRINT_INFO("NODDING_HEAD\n");
            SoundManager::getInstance()->Play(SOUND_OK_DONE);
            
            // Compute time to stop nodding
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;
          }
          break;
        } // case FACE_USER
        case HAPPY_NODDING:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > waitUntilTime_) {
            robot_->SendPlayAnimation(ANIM_BACK_AND_FORTH_EXCITED);
            robot_->MoveHeadToAngle(DEG_TO_RAD(-10), 1, 1);
            
            // Compute time to stop back and forth
            waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.5;
            state_ = BACK_AND_FORTH_EXCITED;
          }
          break;
        } // case HAPPY_NODDING
        case BACK_AND_FORTH_EXCITED:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > waitUntilTime_) {
            robot_->SendPlayAnimation(ANIM_IDLE);
            world_->ClearAllExistingBlocks();
            StartMode(BM_June2014DiceDemo);
          }
          break;
        }
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
