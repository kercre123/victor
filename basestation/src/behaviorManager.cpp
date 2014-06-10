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
        
    BehaviorManager::BehaviorManager()
    : robotMgr_(nullptr)
    , world_(nullptr)
    , distThresh_mm_(20.f)
    , angThresh_(DEG_TO_RAD(10))
    , dockBlock_(nullptr)
    , dockMarker_(nullptr)
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
      
    } // StartMode()
    
    void BehaviorManager::Reset()
    {
      state_ = WAITING_FOR_ROBOT;
      nextState_ = state_;
      updateFcn_ = nullptr;
      robot_ = nullptr;
      
      // Pick and Place
      dockBlock_ = nullptr;
      dockMarker_ = nullptr;
      
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
    
    
    void BehaviorManager::FollowPathHelper()
    {
      
      if(!robot_->IsTraversingPath() && !robot_->IsMoving() &&
         waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) {
        
        if (robot_->get_pose().IsSameAs(goalPose_, distThresh_mm_, angThresh_)) {
          PRINT_INFO("Dock pose reached\n");
          
          if(dockBlock_ != nullptr) {
            // Verify that the block we are docking with still exists in case it somehow disappeared while
            // the robot was travelling to the predock pose.
            const Vision::ObservableObject* oObject = world_->GetObservableObjectByID(dockBlock_->GetID());
            if (oObject == nullptr) {
              PRINT_INFO("Docking block no longer present. Transitioning to %s.\n",
                         QUOTE(problemState_));
              state_ = problemState_;
              return;
            }
          }
          
          if(dockMarker_ != nullptr) {
            // Need to confirm that expected marker is within view.
            // Move head if necessary based on block height.
            Pose3d markerPoseWrtNeck = dockMarker_->GetPose().getWithRespectTo(&robot_->get_neckPose());
            const f32 headAngle = atan2(markerPoseWrtNeck.get_translation().z(),
                                        markerPoseWrtNeck.get_translation().x());
            PRINT_INFO("Moving head angle to %f degrees.\n", RAD_TO_DEG(headAngle));
            robot_->MoveHeadToAngle(headAngle, 1, 1);
          }
          
          // Wait long enough for head to move and a message to the robot to be processed
          waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2;
          
          state_ = nextState_;
          
        } else {
          PRINT_INFO("Not at expected position at the end of the path. "
                     "Resetting BehaviorManager. (Robot: (%.2f %.2f %.2f) @ %.1fdeg VS. Goal: (%.2f %.2f %.2f) @ %.1fdeg)\n.",
                     robot_->get_pose().get_translation().x(),
                     robot_->get_pose().get_translation().y(),
                     robot_->get_pose().get_translation().z(),
                     robot_->get_pose().get_rotationAngle().getDegrees(),
                     goalPose_.get_translation().x(),
                     goalPose_.get_translation().y(),
                     goalPose_.get_translation().z(),
                     goalPose_.get_rotationAngle().getDegrees()
                     );
          StartMode(BM_None);
          return;
        }
      }
      
    } // FollowPathHelper()
    
    
    void BehaviorManager::GoToNearestPreDockPoseHelper(const f32 preDockDistance,
                                                       const f32 desiredHeadAngle)
    {
      CORETECH_ASSERT(dockBlock_ != nullptr);
      
      std::vector<Block::PoseMarkerPair_t> preDockPoseMarkerPairs;
      dockBlock_->GetPreDockPoses(preDockDistance, preDockPoseMarkerPairs);
            
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
            goalPose_ = poseMarkerPair.first;
            dockMarker_ = &(poseMarkerPair.second);
          }
        }
      } else {
        PRINT_INFO("No docking pose found\n");
        StartMode(BM_None);
        return;
      }
      
      const bool alreadyThere = robot_->get_pose().IsSameAs(goalPose_, distThresh_mm_, angThresh_);
      
      if (alreadyThere || robot_->ExecutePathToPose(goalPose_) == RESULT_OK)
      {
        // Make sure head is tilted down so that it can localize well
        robot_->MoveHeadToAngle(desiredHeadAngle, 5, 10);
        PRINT_INFO("Executing path to nearest pre-dock pose %f %f %f\n",
                   goalPose_.get_translation().x(),
                   goalPose_.get_translation().y(),
                   goalPose_.get_rotationAngle().ToFloat());
        waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
        state_ = EXECUTING_PATH_TO_DOCK_POSE;
      }

    } // GoToNearestPreDockPoseHelper()

    void BehaviorManager::BeginDockingHelper()
    {
      if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > waitUntilTime_) {
        // TODO: Check that the marker was recently seen at roughly the expected image location
        // ...
        //const Point2f& imgCorners = dockMarker_->GetImageCorners().computeCentroid();
        // For now, just docking to the marker no matter where it is in the image.
        
        // Get dock action
        const f32 dockBlockHeight = dockBlock_->GetPose().get_translation().z();
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
      
    } // BeginDockingHelper()
    
    
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

          const f32 headAngle = -0.26f;
          nextState_ = BEGIN_DOCKING;
          problemState_ = WAITING_FOR_DOCK_BLOCK;
          GoToNearestPreDockPoseHelper(PREDOCK_DISTANCE_MM, headAngle);
          
          break;
        }
        case EXECUTING_PATH_TO_DOCK_POSE:
        {
          // Once robot is confirmed at the docking pose, execute docking.
          FollowPathHelper();
          break;
        }
        case BEGIN_DOCKING:
        {
          BeginDockingHelper();
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
            
            // Check to see if the dice block has been gone for long enough
            const TimeStamp_t timeSinceSeenDice_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - diceDeletionTime_;
            if(timeSinceSeenDice_ms > TimeBetweenDice_ms) {
              
              // TODO: Do a nod or some kind of acknowledgement we're ready to see next dice
              CoreTechPrint("First dice is gone: ready for next dice!\n");
              state_ = WAITING_TO_SEE_DICE;
            }
          } else {
            
            if(diceBlocks.size() > 1) {
              // Multiple dice blocks in the world, just use first for now.
              // TODO: Issue warning?
              CoreTechPrint("More than one dice block found, using first!\n");
            }
            
            CoreTechPrint("Please move dice away for a moment.\n");
            const BlockID_t blockID = diceBlocks.begin()->first;
            world_->ClearBlock(blockID);
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
          */
          
          const BlockWorld::ObjectsMapByID_t& diceBlocks = world_->GetExistingBlocks(Block::DICE_BLOCK_TYPE);
          if(!diceBlocks.empty()) {
            
            if(diceBlocks.size() > 1) {
              // Multiple dice blocks in the world, keep deleting them all
              // until we only see one
              CoreTechPrint("More than one dice block found!\n");
              
              for(auto diceBlock : diceBlocks) {
                world_->ClearBlock(diceBlock.first);
              }
              
            } else {
              
              Vision::ObservableObject* diceBlock = diceBlocks.begin()->second;
              
              // Get all the observed markers on the dice and look for the one
              // facing up (i.e. the one that is nearly aligned with the z axis)
              // TODO: expose the threshold here?
              const f32 dotprodThresh = 1.f - cos(DEG_TO_RAD(20));
              std::vector<const Vision::KnownMarker*> diceMarkers;
              diceBlock->GetObservedMarkers(diceMarkers);
              
              const Vision::KnownMarker* topMarker = nullptr;
              for(auto marker : diceMarkers) {
                //const f32 dotprod = DotProduct(marker->ComputeNormal(), Z_AXIS_3D);
                const Pose3d markerWRTworld(marker->GetPose().getWithRespectTo(Pose3d::World));
                const f32 dotprod = marker->ComputeNormal(markerWRTworld).z();
                if(NEAR(dotprod, 1.f, dotprodThresh)) {
                  topMarker = marker;
                }
              }
              
              if(topMarker != nullptr) {
                // We found and observed the top marker on the dice. Use it to
                // set which block we are looking for.
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
                  
                  waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
                  state_ = BEGIN_EXPLORING;
                }
                
              } else {
                
                CoreTechPrint("Found dice, but not its top marker.\n");
                
                dockBlock_ = dynamic_cast<Block*>(diceBlock);
                CORETECH_THROW_IF(dockBlock_ == nullptr);
                
                // Try driving closer to dice
                // Compute a pose on the line between robot and dice, half as
                // far away as we are now, pointed towards the dice
                const f32 diceViewingHeadAngle = DEG_TO_RAD(-15);
                
                Vec3f position( dockBlock_->GetPose().get_translation() );
                position -= robot_->get_pose().get_translation();
                const f32 newDistance = 0.5*position.MakeUnitLength();
                if(newDistance < 30.f) {
                  PRINT_INFO("Getting too close to dice and can't see top. Giving up.\n");
                  StartMode(BM_None);
                  return;
                }
                position *= newDistance;
                
                Radians angle = atan2(position.y(), position.x());
                
                goalPose_ = Pose3d(angle, Z_AXIS_3D, {{position.x(), position.y(), 0.f}});
                
                robot_->ExecutePathToPose(goalPose_);

                // Make sure head is tilted down so that it can localize well
                robot_->MoveHeadToAngle(diceViewingHeadAngle, 5, 10);
                PRINT_INFO("Executing path to get closer to dice. Goal = (%.2f, %.2f) @ %.1fdeg\n",
                           goalPose_.get_translation().x(),
                           goalPose_.get_translation().y(),
                           goalPose_.get_rotationAngle().getDegrees());
                
                waitUntilTime_ = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
                dockMarker_    = nullptr; // not needed for dice
                state_         = EXECUTING_PATH_TO_DOCK_POSE;
                nextState_     = WAITING_TO_SEE_DICE;
                problemState_  = WAITING_TO_SEE_DICE;
                                
              }
              
            } // IF only one dice
            
          } // IF any diceBlocks available
          
          break;
        } // case WAITING_FOR_FIRST_DICE
          
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
            dockBlock_ = dynamic_cast<Block*>(blocks.begin()->second);
            CORETECH_THROW_IF(dockBlock_ == nullptr);
            
            robot_->DriveWheels(0.f, 0.f);
            
            nextState_ = BEGIN_DOCKING;
            problemState_ = BEGIN_EXPLORING;
            GoToNearestPreDockPoseHelper(PREDOCK_DISTANCE_MM, DEG_TO_RAD(-15));
          }
          
          break;
        } // EXPLORING

        case EXECUTING_PATH_TO_DOCK_POSE:
        {
          FollowPathHelper();
          break;
        } // EXECUTING_PATH_TO_DOCK_POSE
          
        case BEGIN_DOCKING:
        {
          // This will transition us to EXECUTING_DOCKING
          BeginDockingHelper();
          
          break;
        } // case BEGIN_DOCKING
          
        case EXECUTING_DOCK:
        {
          if (!robot_->IsPickingOrPlacing() && !robot_->IsMoving() &&
              waitUntilTime_ < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())
          {
            // Stopped executing docking path. Did it successfully dock?
            if ((dockAction_ == DA_PICKUP_LOW || dockAction_ == DA_PICKUP_HIGH) && robot_->IsCarryingBlock()) {
              PRINT_INFO("Picked up block successful!\n");
                state_ = BEGIN_EXPLORING;
            } else {
              
              if((dockAction_ == DA_PLACE_HIGH) && !robot_->IsCarryingBlock()) {
                PRINT_INFO("Placed block successfully!\n");
              } else {
                PRINT_INFO("Dock failed! Aborting\n");
              }
              
              StartMode(BM_None);
              return;
            }
          }
          
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
