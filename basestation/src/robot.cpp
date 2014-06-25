//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

// TODO:(bn) should these be a full path?
#include "pathPlanner.h"
#include "pathDolerOuter.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h"


#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/vision/CameraSettings.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "messageHandler.h"
#include "vizManager.h"

#define MAX_DISTANCE_FOR_SHORT_PLANNER 40.0f

namespace Anki {
  namespace Cozmo {
    
#pragma mark --- RobotManager Class Implementations ---

#if USE_SINGLETON_ROBOT_MANAGER
    RobotManager* RobotManager::singletonInstance_ = 0;
#endif
    
    RobotManager::RobotManager()
    {
      
    }
    
    Result RobotManager::Init(IMessageHandler* msgHandler, BlockWorld* blockWorld, IPathPlanner* pathPlanner)
    {
      _msgHandler  = msgHandler;
      _blockWorld  = blockWorld;
      _pathPlanner = pathPlanner;
      
      return RESULT_OK;
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      _robots[withID] = new Robot(withID, _msgHandler, _blockWorld, _pathPlanner);
      _IDs.push_back(withID);
    }
    
    std::vector<RobotID_t> const& RobotManager::GetRobotIDList() const
    {
      return _IDs;
    }
    
    // Get a pointer to a robot by ID
    Robot* RobotManager::GetRobotByID(const RobotID_t robotID)
    {
      if (_robots.find(robotID) != _robots.end()) {
        return _robots[robotID];
      }
      
      return nullptr;
    }
    
    size_t RobotManager::GetNumRobots() const
    {
      return _robots.size();
    }
    
    bool RobotManager::DoesRobotExist(const RobotID_t withID) const
    {
      return _robots.count(withID) > 0;
    }

    
    void RobotManager::UpdateAllRobots()
    {
      for (auto &r : _robots) {
        // Call update
        r.second->Update();
      }
    }
    
    
#pragma mark --- Robot Class Implementations ---
    
    Robot::Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner)
    : _ID(robotID)
    , _msgHandler(msgHandler)
    , _world(world)
    , _longPathPlanner(pathPlanner)
    , _currPathSegment(-1)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEG_TO_RAD(10))
    , _lastSentPathID(0)
    , _lastRecvdPathID(0)
    , _forceReplanOnNextWorldChange(false)
    , _poseOrigin(&Pose3d::AddOrigin())
    , _pose(-M_PI_2, Z_AXIS_3D, {{0.f, 0.f, 0.f}}, _poseOrigin) // Until this robot is localized be seeing a mat marker, create an origin for it to use as its pose parent
    , _frameId(0)
    , _isLocalized(false)
    , _neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &_pose)
    , _headCamPose({0,0,1,  -1,0,0,  0,-1,0},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &_neckPose)
    , _liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &_pose)
    , _liftPose(0.f, Y_AXIS_3D, {{LIFT_ARM_LENGTH, 0.f, 0.f}}, &_liftBasePose)
    , _currentHeadAngle(0)
    , _currentLiftAngle(0)
    , _isPickingOrPlacing(false)
    , _carryingBlockID(ANY_OBJECT)
    , _state(IDLE)
    , _dockBlockID(ANY_OBJECT)
    , _dockMarker(nullptr)
    {
      SetHeadAngle(_currentHeadAngle);
      pdo_ = new PathDolerOuter(msgHandler, robotID);
      _shortPathPlanner = new FaceAndApproachPlanner;
      _selectedPathPlanner = _longPathPlanner;
    } // Constructor: Robot

    Robot::~Robot()
    {
      delete pdo_;
      pdo_ = nullptr;

      delete _shortPathPlanner;
      _shortPathPlanner = nullptr;
      _selectedPathPlanner = nullptr;
    }
    
    void Robot::Update(void)
    {
      static bool wasTraversingPath = false;
      
      switch(_state)
      {
        case IDLE:
        {
          // Nothing to do in IDLE mode?
          break;
        } // case IDLE
          
        case FOLLOWING_PATH:
        {
          if(IsTraversingPath())
          {
            // If the robot is traversing a path, consider replanning it
            if(_world->DidBlocksChange())
            {
              Planning::Path newPath;
              switch(_selectedPathPlanner->GetPlan(newPath, GetPose(), _forceReplanOnNextWorldChange))
              {
                case IPathPlanner::DID_PLAN:
                {
                  // clear path, but flag that we are replanning
                  ClearPath();
                  wasTraversingPath = false;
                  _forceReplanOnNextWorldChange = false;
                  
                  PRINT_NAMED_INFO("Robot.Update.ClearPath", "sending message to clear old path\n");
                  MessageClearPath clearMessage;
                  _msgHandler->SendMessage(_ID, clearMessage);
                  
                  _path = newPath;
                  PRINT_NAMED_INFO("Robot.Update.UpdatePath", "sending new path to robot\n");
                  SendExecutePath(_path);
                  break;
                } // case DID_PLAN:
                  
                case IPathPlanner::PLAN_NEEDED_BUT_GOAL_FAILURE:
                {
                  ClearPath();
                  if(_nextState == BEGIN_DOCKING) {
                    PRINT_NAMED_INFO("Robot.Update.NewGoalForReplanNeededWhileDocking",
                                     "Replan failed during docking due to bad goal. Will try to update goal.");
                    ExecuteDockingSequence(_dockBlockID);
                  } else {
                    PRINT_NAMED_INFO("Robot.Update.NewGoalForReplanNeeded",
                                     "Replan failed due to bad goal. Aborting path.");
                    SetState(IDLE);
                  }
                  break;
                } // PLAN_NEEDED_BUT_GOAL_FAILURE:
                  
                case IPathPlanner::PLAN_NEEDED_BUT_START_FAILURE:
                {
                  PRINT_NAMED_INFO("Robot.Update.NewStartForReplanNeeded",
                                   "Replan failed during docking due to bad start. Will try again, and hope robot moves.");
                  break;
                }

                case IPathPlanner::PLAN_NEEDED_BUT_PLAN_FAILURE:
                {
                  PRINT_NAMED_INFO("Robot.Update.NewEnvironmentForReplanNeeded",
                                   "Replan failed during docking due to a planner failure. Will try again, and hope environment changes.");
                  _forceReplanOnNextWorldChange = true;
                  break;
                }
                  
                default:
                {
                  // Don't do anything just proceed with the current plan...
                  break;
                }
                  
              } // switch(GetPlan())
            } // if blocks changed

            if (GetLastRecvdPathID() == GetLastSentPathID()) {
              pdo_->Update(GetCurrPathSegment(), GetNumFreeSegmentSlots());
            }
          } else { // IsTraversingPath is false?
            
            // The last path sent was definitely received by the robot
            // and it is no longer executing it.
            if (_lastSentPathID == _lastRecvdPathID) {
              PRINT_NAMED_INFO("Robot.Update.FollowToIdle", "lastPathID %d\n", _lastRecvdPathID);
              SetState(IDLE);
            } else {
              PRINT_NAMED_INFO("Robot.Update.FollowPathStateButNotTraversingPath",
                               "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false. currPathSegment = %d\n",
                               _currPathSegment);
            }
          }
          
          // Visualize path if robot has just started traversing it.
          // Clear the path when it has stopped.
          if (!wasTraversingPath && IsTraversingPath() && _path.GetNumSegments() > 0) {
            VizManager::getInstance()->DrawPath(_ID,_path,VIZ_COLOR_EXECUTED_PATH);
            wasTraversingPath = true;
          }
          else if ((wasTraversingPath && !IsTraversingPath()) ||
                     _pose.IsSameAs(_goalPose, _goalDistanceThreshold, _goalAngleThreshold))
          {
            PRINT_INFO("Robot %d finished following path.\n", _ID);
            ClearPath(); // clear path and indicate that we are not replanning
            wasTraversingPath = false;
            SetState(_nextState);
            VizManager::getInstance()->EraseAllPlannerObstacles(true);
            VizManager::getInstance()->EraseAllPlannerObstacles(false);
          }
          break;
        } // case FOLLOWING_PATH
      
        case BEGIN_DOCKING:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime) {
            // TODO: Check that the marker was recently seen at roughly the expected image location
            // ...
            //const Point2f& imgCorners = dockMarker_->GetImageCorners().computeCentroid();
            // For now, just docking to the marker no matter where it is in the image.
            
            // Get dock action
            Block* dockBlock = _world->GetBlockByID(_dockBlockID);
            if(dockBlock == nullptr) {
              PRINT_NAMED_ERROR("Robot.Update.DockBlockGone", "Docking block with ID=%d no longer exists in the world. Returning to IDLE state.\n", _dockBlockID);
              SetState(IDLE);
              return;
            }
            
            const f32 dockBlockHeight = dockBlock->GetPose().get_translation().z();
            _dockAction = DA_PICKUP_LOW;
            if (dockBlockHeight > dockBlock->GetSize().z()) {
              if(IsCarryingBlock()) {
                PRINT_INFO("Already carrying block. Can't dock to high block. Aborting.\n");
                SetState(IDLE);
                return;
                
              } else {
                _dockAction = DA_PICKUP_HIGH;
              }
            } else if (IsCarryingBlock()) {
              _dockAction = DA_PLACE_HIGH;
            }
            
            // Start dock
            PRINT_INFO("Docking with marker %d (action = %d)\n", _dockMarker->GetCode(), _dockAction);
            DockWithBlock(_dockBlockID, _dockMarker, _dockAction);
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5;
            SetState(DOCKING);
          }
          break;
        } // case BEGIN_DOCKING
          
        case DOCKING:
        {
          if (!IsPickingOrPlacing() && !IsMoving() &&
              _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())
          {
            // Stopped executing docking path. Did it successfully dock?
            if ((_dockAction == DA_PICKUP_LOW || _dockAction == DA_PICKUP_HIGH) && IsCarryingBlock()) {
              PRINT_INFO("Picked up block successful!\n");
            } else {
              
              if((_dockAction == DA_PLACE_HIGH) && !IsCarryingBlock()) {
                PRINT_INFO("Placed block successfully!\n");
              } else {
                PRINT_INFO("Deleting block that I failed to dock to, and aborting.\n");
                _world->ClearBlock(_dockBlockID);
              }
            }
            
            _dockBlockID = ANY_OBJECT;
            _dockMarker  = nullptr;
            
            SetState(IDLE);
          }
          break;
        } // case DOCKING
          
        default:
          PRINT_NAMED_ERROR("Robot::Update", "Transitioned to unknown state %d!\n", _state);
          assert(false);
          _state = IDLE;
          return;
          
      } // switch(state_)
      
      
      // Visualize path if robot has just started traversing it.
      // Clear the path when it has stopped.
      if (!IsPickingOrPlacing()) {
        if (!wasTraversingPath && IsTraversingPath() && _path.GetNumSegments() > 0) {
          VizManager::getInstance()->DrawPath(_ID,_path,VIZ_COLOR_EXECUTED_PATH);
          wasTraversingPath = true;
        } else if (wasTraversingPath && !IsTraversingPath()){
          ClearPath(); // clear path and indicate that we are not replanning
          VizManager::getInstance()->ErasePath(_ID);
          wasTraversingPath = false;
        }
      }

      
    } // Update()

    void Robot::SetState(const State nextState)
    {
      // TODO: Provide string name lookup for each state
      PRINT_INFO("Robot %d switching from state %d to state %d.\n", _ID, _state, nextState);

      switch(nextState) {
        case IDLE:
        case FOLLOWING_PATH:
        case BEGIN_DOCKING:
        case DOCKING:
          break;

        default:
          PRINT_NAMED_ERROR("Robot::SetState", "Trying to transition to invalid state %d!\n", nextState);
          assert(false);
          return;
      }

      _state = nextState;
    }

    
    void Robot::SetPose(const Pose3d &newPose)
    {
      // Update our current pose and let the physical robot know where it is:
      _pose = newPose;
      
    } // set_pose()
    
    void Robot::SetHeadAngle(const f32& angle)
    {
      if(angle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too small. Clipping.\n", angle);
        _currentHeadAngle = MIN_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else if(angle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too large. Clipping.\n", angle);
        _currentHeadAngle = MAX_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else {
        _currentHeadAngle = angle;
      }
      
      // Start with canonical (untilted) headPose
      Pose3d newHeadPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-_currentHeadAngle, Y_AXIS_3D);
      newHeadPose.rotateBy(Rvec);
      
      // Update the head camera's pose
      _camera.SetPose(newHeadPose);
      
    } // set_headAngle()

    void Robot::ComputeLiftPose(const f32 atAngle, Pose3d& liftPose)
    {
      // Reset to canonical position
      liftPose.set_rotation(atAngle, Y_AXIS_3D);
      liftPose.set_translation({{LIFT_ARM_LENGTH, 0.f, 0.f}});
      
      // Rotate to the given angle
      RotationVector3d Rvec(-atAngle, Y_AXIS_3D);
      liftPose.rotateBy(Rvec);
    }
    
    void Robot::SetLiftAngle(const f32& angle)
    {
      // TODO: Add lift angle limits?
      _currentLiftAngle = angle;
      
      Robot::ComputeLiftPose(_currentLiftAngle, _liftPose);

      CORETECH_ASSERT(_liftPose.get_parent() == &_liftBasePose);
    }
        
    Result Robot::GetPathToPose(const Pose3d& targetPose, Planning::Path& path)
    {
      SelectPlanner(targetPose);

      IPathPlanner::EPlanStatus status = _selectedPathPlanner->GetPlan(path, GetPose(), targetPose);

      if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN)
        return RESULT_OK;
      else
        return RESULT_FAIL;
    }
    
    Result Robot::ExecutePathToPose(const Pose3d& pose)
    {
      return ExecutePathToPose(pose, GetHeadAngle());
    }
    
    void Robot::SelectPlanner(const Pose3d& targetPose)
    {
      Pose2d target2d(targetPose);
      Pose2d start2d(GetPose());

      float distSquared = pow(target2d.get_x() - start2d.get_x(), 2) + pow(target2d.get_y() - start2d.get_y(), 2);

      if(distSquared < MAX_DISTANCE_FOR_SHORT_PLANNER * MAX_DISTANCE_FOR_SHORT_PLANNER) {
        PRINT_NAMED_INFO("Robot.SelectPlanner", "distance^2 is %f, selecting short planner\n", distSquared);
        _selectedPathPlanner = _shortPathPlanner;
      }
      else {
        PRINT_NAMED_INFO("Robot.SelectPlanner", "distance^2 is %f, selecting long planner\n", distSquared);
        _selectedPathPlanner = _longPathPlanner;
      }
    }

    Result Robot::ExecutePathToPose(const Pose3d& pose, const Radians headAngle)
    {
      Planning::Path p;
      Result lastResult = GetPathToPose(pose, p);
      
      if(lastResult == RESULT_OK)
      {
        _path = p;
        _goalPose = pose;
      
        MoveHeadToAngle(headAngle.ToFloat(), 5, 10);
        
        lastResult = ExecutePath(p);
      }
      
      return lastResult;
    }
    
    Result Robot::GetPathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex, Planning::Path& path)
    {
      // Let the long path (lattice) planner do its thing and choose a target
      _selectedPathPlanner = _longPathPlanner;
      IPathPlanner::EPlanStatus status = _selectedPathPlanner->GetPlan(path, GetPose(), poses, selectedIndex);
      
      if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN)
      {
        // See if SelectPlanner selects the long path planner based on the pose it
        // selected
        SelectPlanner(poses[selectedIndex]);
        
        // If SelectPlanner would rather use the short path planner, let it get a
        // plan and use that one instead.
        if(_selectedPathPlanner != _longPathPlanner) {
          _selectedPathPlanner->GetPlan(path, GetPose(), poses[selectedIndex]);
        }
        
        if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN) {
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      } else {
        return RESULT_FAIL;
      }
      
    } // GetPathToPose(multiple poses)
    
    Result Robot::ExecutePathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex)
    {
      return ExecutePathToPose(poses, GetHeadAngle(), selectedIndex);
    }
    
    Result Robot::ExecutePathToPose(const std::vector<Pose3d>& poses, const Radians headAngle, size_t& selectedIndex)
    {
      Planning::Path p;
      Result lastResult = GetPathToPose(poses, selectedIndex, p);
      
      if(lastResult == RESULT_OK) {
        _path = p;
        _goalPose = poses[selectedIndex];
        
        MoveHeadToAngle(headAngle.ToFloat(), 5, 10);
        
        lastResult = ExecutePath(p);
      }
      
      return lastResult;
    }

    
    void Robot::AbortCurrentPath()
    {
      ClearPath();
      SetState(IDLE);
    }
    
    // =========== Motor commands ============
    
    // Sends message to move lift at specified speed
    Result Robot::MoveLift(const f32 speed_rad_per_sec)
    {
      return SendMoveLift(speed_rad_per_sec);
    }
    
    // Sends message to move head at specified speed
    Result Robot::MoveHead(const f32 speed_rad_per_sec)
    {
      return SendMoveHead(speed_rad_per_sec);
    }
    
    // Sends a message to the robot to move the lift to the specified height
    Result Robot::MoveLiftToHeight(const f32 height_mm,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2)
    {
      return SendSetLiftHeight(height_mm, max_speed_rad_per_sec, accel_rad_per_sec2);
    }
    
    // Sends a message to the robot to move the head to the specified angle
    Result Robot::MoveHeadToAngle(const f32 angle_rad,
                                  const f32 max_speed_rad_per_sec,
                                  const f32 accel_rad_per_sec2)
    {
      return SendSetHeadAngle(angle_rad, max_speed_rad_per_sec, accel_rad_per_sec2);
    }
    
    Result Robot::DriveWheels(const f32 lwheel_speed_mmps,
                              const f32 rwheel_speed_mmps)
    {
      return SendDriveWheels(lwheel_speed_mmps, rwheel_speed_mmps);
    }
    
    Result Robot::StopAllMotors()
    {
      return SendStopAllMotors();
    }

    Result Robot::TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments)
    {
      return SendTrimPath(numPopFrontSegments, numPopBackSegments);
    }
    
    // Clears the path that the robot is executing which also stops the robot
    Result Robot::ClearPath()
    {
      // TODO: SetState(IDLE) ?
      VizManager::getInstance()->ErasePath(_ID);
      pdo_->ClearPath();
      return SendClearPath();
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::ExecutePath(const Planning::Path& path)
    {
      if (path.GetNumSegments() == 0) {
        PRINT_NAMED_WARNING("Robot.ExecutePath.EmptyPath", "\n");
        return RESULT_OK;
      }
      
      // TODO: Clear currently executing path or write to buffered path?
      if (ClearPath() == RESULT_FAIL)
        return RESULT_FAIL;

      SetState(FOLLOWING_PATH);
      _nextState = IDLE; // for when the path is complete
      ++_lastSentPathID;
      
      return SendExecutePath(path);
    }
    
  
    Result Robot::ExecuteDockingSequence(ObjectID_t blockToDockWith)
    {
      Result lastResult = RESULT_OK;
      
      Block* block = _world->GetBlockByID(blockToDockWith);
      if(block == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteDockingSequence.DockBlockDoesNotExist",
                          "Robot %d asked to dock with Block ID=%d, but it does not exist.",
                          _ID, blockToDockWith);

        return RESULT_FAIL;
      }
      
      _dockBlockID = blockToDockWith;
      _dockMarker = nullptr; // should get set to a predock pose below
      
      std::vector<Block::PoseMarkerPair_t> preDockPoseMarkerPairs;
      block->GetPreDockPoses(PREDOCK_DISTANCE_MM, preDockPoseMarkerPairs);
      
      if (preDockPoseMarkerPairs.empty()) {
        PRINT_NAMED_ERROR("Robot.ExecuteDockingSequence.NoPreDockPoses",
                          "Dock block did not provide any pre-dock poses!\n");
        return RESULT_FAIL;
      }
      
      // Let the planner choose which pre-dock pose to use. Create a vector of
      // pose options
      size_t selectedIndex = preDockPoseMarkerPairs.size();
      std::vector<Pose3d> preDockPoses;
      preDockPoses.reserve(preDockPoseMarkerPairs.size());
      for(auto const& preDockPair : preDockPoseMarkerPairs) {
        preDockPoses.emplace_back(preDockPair.first);
      }
      
      _goalDistanceThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
      _goalAngleThreshold    = DEG_TO_RAD(10);
      
      lastResult = ExecutePathToPose(preDockPoses, DEG_TO_RAD(-15), selectedIndex);
      if(lastResult != RESULT_OK) {
        return lastResult;
      }
      
      _goalPose = preDockPoses[selectedIndex];
      _dockMarker = &(preDockPoseMarkerPairs[selectedIndex].second);
      
      
      PRINT_INFO("Executing path to nearest pre-dock pose: (%.2f, %.2f) @ %.1fdeg\n",
                 _goalPose.get_translation().x(),
                 _goalPose.get_translation().y(),
                 _goalPose.get_rotationAngle().getDegrees());
      
      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
      _state = FOLLOWING_PATH;
      _nextState = BEGIN_DOCKING;
      
      return lastResult;
      
    } // ExecuteDockingSequence()
    
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing.
    Result Robot::DockWithBlock(const ObjectID_t blockID,
                                const Vision::KnownMarker* marker,
                                const DockAction_t dockAction)
    {
      return DockWithBlock(blockID, marker, dockAction, 0, 0, u8_MAX);
    }
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing. If pixel_radius == u8_MAX,
    // the marker can be seen anywhere in the image (same as above function), otherwise the
    // marker's center must be seen at the specified image coordinates
    // with pixel_radius pixels.
    Result Robot::DockWithBlock(const ObjectID_t blockID,
                                const Vision::KnownMarker* marker,
                                const DockAction_t dockAction,
                                const u16 image_pixel_x,
                                const u16 image_pixel_y,
                                const u8 pixel_radius)
    {
      Block* block = _world->GetBlockByID(blockID);
      if(block == nullptr) {
        PRINT_NAMED_ERROR("Robot.DockWithBlock.BlockDoesNotExist", "Block with ID=%d no longer exists for docking.\n", blockID);
        return RESULT_FAIL;
      }

      CORETECH_ASSERT(marker != nullptr);
      
      _dockBlockID = blockID;
      _dockMarker  = marker;
      
      // Dock marker has to be a child of the dock block
      if(_dockMarker->GetPose().get_parent() != &block->GetPose()) {
        PRINT_NAMED_ERROR("Robot.DockWithBlock.MarkerNotOnBlock",
                          "Specified dock marker must be a child of the specified dock block.\n");
        return RESULT_FAIL;
      }
      
      return SendDockWithBlock(_dockMarker->GetCode(), _dockMarker->GetSize(), dockAction,
                               image_pixel_x, image_pixel_y, pixel_radius);
    }
    
    
    Result Robot::PickUpDockBlock()
    {
      if(_dockBlockID == ANY_OBJECT) {
        PRINT_NAMED_WARNING("Robot.NoDockBlockIDSet", "No docking block ID set, but told to pick one up.");
        return RESULT_FAIL;
      }
      
      Block* block = _world->GetBlockByID(_dockBlockID);
      if(block == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockBlock.BlockDoesNotExist",
                          "Dock block with ID=%d no longer exists for picking up.\n", _dockBlockID);
        return RESULT_FAIL;
      }
      
      _carryingBlockID = _dockBlockID;

      // Base the block's pose relative to the lift on how far away the dock
      // marker is from the center of the block
      // TODO: compute the height adjustment per block or at least use values from cozmoConfig.h
      Pose3d newPose(block->GetPose().get_rotationMatrix(),
                     {{_dockMarker->GetPose().get_translation().Length() +
                       LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f}});
      
      // make part of the lift's pose chain so the block will now be relative to
      // the lift and move with the robot
      newPose.set_parent(&_liftPose);

      _dockBlockID = ANY_OBJECT;
      _dockMarker  = nullptr;
      
      block->SetPose(newPose);
      block->SetIsBeingCarried(true);
      
      return RESULT_OK;
      
    } // PickUpDockBlock()
    
    
    Result Robot::PlaceCarriedBlock() //const TimeStamp_t atTime)
    {
      if(_carryingBlockID == ANY_OBJECT) {
        PRINT_NAMED_WARNING("Robot.PlaceCarriedBlock.CarryingBlockNotSpecified",
                          "No carrying block set, but told to place one.");
        return RESULT_FAIL;
      }
      
      Block* block = _world->GetBlockByID(_carryingBlockID);
      
      if(block == nullptr)
      {
        // This really should not happen.  How can a block being carried get deleted?
        PRINT_NAMED_ERROR("Robot.PlaceCarriedBlock.CarryingBlockDoesNotExist",
                          "Carrying block with ID=%d no longer exists.\n", _carryingBlockID);
        return RESULT_FAIL;
      }
      
      /*
      Result lastResult = RESULT_OK;
      
      TimeStamp_t histTime;
      RobotPoseStamp* histPosePtr = nullptr;
      if ((lastResult = ComputeAndInsertPoseIntoHistory(atTime, histTime, &histPosePtr)) != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.PlaceCarriedBlock.CouldNotComputeHistoricalPose", "Time %d\n", atTime);
        return lastResult;
      }
      
      Pose3d liftBasePoseAtTime(_liftBasePose);
      liftBasePoseAtTime.set_parent(&histPosePtr->GetPose());
      
      Pose3d liftPoseAtTime;
      Robot::ComputeLiftPose(histPosePtr->GetLiftAngle(), liftPoseAtTime);
      liftPoseAtTime.set_parent(&liftBasePoseAtTime);
      
      Pose3d blockPoseAtTime(_carryingBlock->GetPose());
      blockPoseAtTime.set_parent(&liftPoseAtTime);
       
      _carryingBlock->SetPose(blockPoseAtTime.getWithRespectTo(Pose3d::World));
      */
      
      Pose3d placedPose;
      if(block->GetPose().getWithRespectTo(_pose.FindOrigin(), placedPose) == false) {
        PRINT_NAMED_ERROR("Robot.PlaceCarriedBlock.OriginMisMatch",
                          "Could not get carrying block's pose relative to robot's origin.\n");
        return RESULT_FAIL;
      }
      block->SetPose(placedPose);
      
      block->SetIsBeingCarried(false);
      
      PRINT_NAMED_INFO("Robot.PlaceCarriedBlock.BlockPlaced",
                       "Robot %d successfully placed block %d at (%.2f, %.2f, %.2f).\n",
                       _ID, block->GetID(),
                       block->GetPose().get_translation().x(),
                       block->GetPose().get_translation().y(),
                       block->GetPose().get_translation().z());

      _carryingBlockID = ANY_OBJECT;
      
      return RESULT_OK;
      
    } // PlaceCarriedBlock()
    
    

    Result Robot::SetHeadlight(u8 intensity)
    {
      return SendHeadlight(intensity);
    }
    
    // ============ Messaging ================
    
    // Sync time with physical robot and trigger it robot to send back camera calibration
    Result Robot::SendInit() const
    {
      MessageRobotInit m;
      m.robotID  = _ID;
      m.syncTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Clears the path that the robot is executing which also stops the robot
    Result Robot::SendClearPath() const
    {
      MessageClearPath m;
      m.pathID = 0;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Removes the specified number of segments from the front and back of the path
    Result Robot::SendTrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments) const
    {
      MessageTrimPath m;
      m.numPopFrontSegments = numPopFrontSegments;
      m.numPopBackSegments = numPopBackSegments;

      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::SendExecutePath(const Planning::Path& path) const
    {
      pdo_->SetPath(path);

      // Send start path execution message
      MessageExecutePath m;
      m.pathID = _lastSentPathID;
      PRINT_NAMED_INFO("Robot::SendExecutePath", "sending start execution message\n");
      return _msgHandler->SendMessage(_ID, m);
    }
    
    /*
    Result Robot::SendDockWithBlock(const Vision::Marker::Code& markerType, const f32 markerWidth_mm, const DockAction_t dockAction) const
    {
      return SendDockWithBlock(markerType, markerWidth_mm, dockAction, 0, 0, u8_MAX);
    }
     */

    
    Result Robot::SendDockWithBlock(const Vision::Marker::Code& markerType,
                                    const f32 markerWidth_mm,
                                    const DockAction_t dockAction,
                                    const u16 image_pixel_x,
                                    const u16 image_pixel_y,
                                    const u8 pixel_radius) const
    {
      MessageDockWithBlock m;
      m.markerWidth_mm = markerWidth_mm;
      CORETECH_ASSERT(markerType <= u8_MAX);
      m.markerType = static_cast<u8>(markerType);
      m.dockAction = dockAction;
      m.image_pixel_x = image_pixel_x;
      m.image_pixel_y = image_pixel_y;
      m.pixel_radius = pixel_radius;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendMoveLift(const f32 speed_rad_per_sec) const
    {
      MessageMoveLift m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendMoveHead(const f32 speed_rad_per_sec) const
    {
      MessageMoveHead m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return _msgHandler->SendMessage(_ID,m);
    }

    Result Robot::SendSetLiftHeight(const f32 height_mm,
                                    const f32 max_speed_rad_per_sec,
                                    const f32 accel_rad_per_sec2) const
    {
      MessageSetLiftHeight m;
      m.height_mm = height_mm;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendSetHeadAngle(const f32 angle_rad,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2) const
    {
      MessageSetHeadAngle m;
      m.angle_rad = angle_rad;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps) const
    {
      MessageDriveWheels m;
      m.lwheel_speed_mmps = lwheel_speed_mmps;
      m.rwheel_speed_mmps = rwheel_speed_mmps;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendStopAllMotors() const
    {
      MessageStopAllMotors m;
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendAbsLocalizationUpdate() const
    {
      // TODO: Add z?
      MessageAbsLocalizationUpdate m;
      
      // Look in history for the last vis pose and send it.
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.GetLatestVisionOnlyPose(t, p) == RESULT_FAIL) {
        PRINT_NAMED_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
        return RESULT_FAIL;
      }

      m.timestamp = t;
      
      m.pose_frame_id = p.GetFrameId();
      
      m.xPosition = p.GetPose().get_translation().x();
      m.yPosition = p.GetPose().get_translation().y();

      m.headingAngle = p.GetPose().get_rotationMatrix().GetAngleAroundZaxis().ToFloat();
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendHeadAngleUpdate() const
    {
      MessageHeadAngleUpdate m;
      
      m.newAngle = _currentHeadAngle;
      
      return _msgHandler->SendMessage(_ID, m);
    }

    Result Robot::SendImageRequest(const ImageSendMode_t mode) const
    {
      MessageImageRequest m;
      
      m.imageSendMode = mode;
      m.resolution = IMG_STREAM_RES;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendStartTestMode(const TestMode mode) const
    {
      MessageStartTestMode m;
      
      m.mode = mode;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendHeadlight(u8 intensity)
    {
      MessageSetHeadlight m;
      m.intensity = intensity;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    const Quad2f Robot::CanonicalBoundingBoxXY({{ROBOT_BOUNDING_X_FRONT, -0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT,  0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X, -0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X,  0.5f*ROBOT_BOUNDING_Y}});
    
    Quad2f Robot::GetBoundingQuadXY(const f32 padding_mm) const
    {
      return GetBoundingQuadXY(_pose, padding_mm);
    }
    
    Quad2f Robot::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      const RotationMatrix2d R(atPose.get_rotationMatrix().GetAngleAroundZaxis());
      
      Quad2f boundingQuad(Robot::CanonicalBoundingBoxXY);
      if(padding_mm != 0.f) {
        Quad2f paddingQuad({{ padding_mm, -padding_mm}},
                           {{ padding_mm,  padding_mm}},
                           {{-padding_mm, -padding_mm}},
                           {{-padding_mm,  padding_mm}});
        boundingQuad += paddingQuad;
      }
      
      using namespace Quad;
      for(CornerName iCorner = FirstCorner; iCorner < NumCorners; ++iCorner) {
        // Rotate to given pose
        boundingQuad[iCorner] = R*Robot::CanonicalBoundingBoxXY[iCorner];
      }
      
      // Re-center
      Point2f center(atPose.get_translation().x(), atPose.get_translation().y());
      boundingQuad += center;
      
      return boundingQuad;
      
    } // GetBoundingBoxXY()
    

    Result Robot::SendHeadControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError)
    {
      MessageSetHeadControllerGains m;
      m.kp = kp;
      m.ki = ki;
      m.maxIntegralError = maxIntegralError;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendLiftControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError)
    {
      MessageSetLiftControllerGains m;
      m.kp = kp;
      m.ki = ki;
      m.maxIntegralError = maxIntegralError;
      return _msgHandler->SendMessage(_ID, m);
    }

    Result Robot::SendPlayAnimation(AnimationID_t id)
    {
      MessagePlayAnimation m;
      m.animationID = id;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    // ============ Pose history ===============
    
    Result Robot::AddRawOdomPoseToHistory(const TimeStamp_t t,
                                          const PoseFrameID_t frameID,
                                          const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                          const f32 pose_angle,
                                          const f32 head_angle,
                                          const f32 lift_angle,
                                          const Pose3d* pose_origin)
    {
      return _poseHistory.AddRawOdomPose(t, frameID, pose_x, pose_y, pose_z, pose_angle, head_angle, lift_angle, pose_origin);
    }
    
    Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                             const RobotPoseStamp& p)
    {
      if(!_isLocalized) {
        // If we aren't localized yet, we are about to be, by virtue of this pose
        // stamp.  So we need to take the current origin (which may be the
        // the pose parent of observed objects) and update it
        
        // Reverse the connection between origin and robot
        //CORETECH_ASSERT(p.GetPose().get_parent() == _poseOrigin);
        *_poseOrigin = _pose.getInverse();
        _poseOrigin->set_parent(&p.GetPose());
        
        // Connect the old origin's pose to the same root the robot now has.
        // It is no longer the robot's origin, but for any of its children,
        // it is now in the right coordinates.
        if(_poseOrigin->getWithRespectTo(p.GetPose().FindOrigin(), *_poseOrigin) == false) {
          PRINT_NAMED_ERROR("Robot.AddVisionOnlyPoseToHistory.NewLocalizationOriginProblem",
                            "Could not get pose origin w.r.t. RobotPoseStamp's pose.\n");
          return RESULT_FAIL;
        }
        
        // Now make the robot's origin point to the robot's pose's parent.
        // TODO: avoid the icky const cast here...
        _poseOrigin = const_cast<Pose3d*>(p.GetPose().get_parent());
        
        _isLocalized = true;
      }
      
      return _poseHistory.AddVisionOnlyPose(t, p);
    }

    Result Robot::ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                                  TimeStamp_t& t, RobotPoseStamp** p,
                                                  HistPoseKey* key,
                                                  bool withInterpolation)
    {
      return _poseHistory.ComputeAndInsertPoseAt(t_request, t, p, key, withInterpolation);
    }

    Result Robot::GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p)
    {
      return _poseHistory.GetVisionOnlyPoseAt(t_request, p);
    }

    Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key)
    {
      return _poseHistory.GetComputedPoseAt(t_request, p, key);
    }

    TimeStamp_t Robot::GetLastMsgTimestamp() const
    {
      return _poseHistory.GetNewestTimeStamp();
    }
    
    bool Robot::IsValidPoseKey(const HistPoseKey key) const
    {
      return _poseHistory.IsValidPoseKey(key);
    }
    
    bool Robot::UpdateCurrPoseFromHistory()
    {
      bool poseUpdated = false;
      
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.ComputePoseAt(_poseHistory.GetNewestTimeStamp(), t, p) == RESULT_OK) {
        if (p.GetFrameId() == GetPoseFrameID()) {
          _pose = p.GetPose();
          poseUpdated = true;
        }
      }
      
      return poseUpdated;
    }
    
    
  } // namespace Cozmo
} // namespace Anki
