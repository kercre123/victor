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
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/utils/fileManagement.h"

#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/imageIO.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "messageHandler.h"
#include "ramp.h"
#include "vizManager.h"

#include <fstream>

#define MAX_DISTANCE_FOR_SHORT_PLANNER 40.0f
#define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f

#define DISPLAY_PROX_OVERLAY 1


namespace Anki {
  namespace Cozmo {
    
#pragma mark --- RobotManager Class Implementations ---

   
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
    
    const std::map<Robot::State, std::string> Robot::StateNames = {
      {IDLE,                  "IDLE"},
      {FOLLOWING_PATH,        "FOLLOWING_PATH"},
      {BEGIN_DOCKING,         "BEGIN_DOCKING"},
      {DOCKING,               "DOCKING"},
      {PLACE_OBJECT_ON_GROUND, "PLACE_OBJECT_ON_GROUND"}
    };
    
    
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
    , _saveImages(false)
    , _camera(robotID)
    , _proxLeft(0), _proxFwd(0), _proxRight(0)
    , _proxFwdBlocked(false), _proxSidesBlocked(false)
    , _poseOrigins(1)
    , _worldOrigin(&_poseOrigins.front())
    , _pose(-M_PI_2, Z_AXIS_3D, {{0.f, 0.f, 0.f}}, _worldOrigin, "Robot_" + std::to_string(_ID))
    , _frameId(0)
    , _localizedToFixedMat(false)
    , _onRamp(false)
    , _neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &_pose, "RobotNeck")
    , _headCamPose({0,0,1,  -1,0,0,  0,-1,0},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &_neckPose, "RobotHeadCam")
    , _liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &_pose, "RobotLiftBase")
    , _liftPose(0.f, Y_AXIS_3D, {{LIFT_ARM_LENGTH, 0.f, 0.f}}, &_liftBasePose, "RobotLift")
    , _currentHeadAngle(0)
    , _currentLiftAngle(0)
    , _isPickingOrPlacing(false)
    , _isPickedUp(false)
    , _state(IDLE)
    , _carryingMarker(nullptr)
    , _dockMarker(nullptr)
    , _dockMarker2(nullptr)
    {
      SetHeadAngle(_currentHeadAngle);
      pdo_ = new PathDolerOuter(msgHandler, robotID);
      _shortPathPlanner = new FaceAndApproachPlanner;
      _selectedPathPlanner = _longPathPlanner;
      
      _poseOrigins.front().SetName("Robot" + std::to_string(_ID) + "_PoseOrigin0");
      
    } // Constructor: Robot

    Robot::~Robot()
    {
      delete pdo_;
      pdo_ = nullptr;

      delete _shortPathPlanner;
      _shortPathPlanner = nullptr;
      _selectedPathPlanner = nullptr;
    }
    
    Result Robot::UpdateFullRobotState(const MessageRobotState& msg)
    {
      Result lastResult = RESULT_OK;
      
      // Update head angle
      SetHeadAngle(msg.headAngle);
      
      // Update lift angle
      SetLiftAngle(msg.liftAngle);

      // Update proximity sensor values
      SetProxSensorData(msg.proxLeft, msg.proxForward, msg.proxRight,
                        msg.status & IS_PROX_FORWARD_BLOCKED,
                        msg.status & IS_PROX_SIDE_BLOCKED);

      if(DISPLAY_PROX_OVERLAY) {
        // printf("displaying: prox L,F,R (%2u, %2u, %2u), blocked: (%d,%d,%d)\n",
        //        msg.proxLeft, msg.proxForward, msg.proxRight,
        //        msg.status & IS_PROX_SIDE_BLOCKED,
        //        msg.status & IS_PROX_FORWARD_BLOCKED,
        //        msg.status & IS_PROX_SIDE_BLOCKED);

        VizManager::getInstance()->SetText(0,   // TODO:(bn) id??
                                           Anki::NamedColors::GREEN,
                                           "prox: (%2u, %2u, %2u) %d%d%d",
                                           _proxLeft, _proxFwd, _proxRight,
                                           IsProxSidesBlocked(),
                                           IsProxForwardBlocked(),
                                           IsProxSidesBlocked());
      }
      
      // Get ID of last/current path that the robot executed
      SetLastRecvdPathID(msg.lastPathID);
      
      // Update other state vars
      SetCurrPathSegment( msg.currPathSegment );
      SetNumFreeSegmentSlots(msg.numFreeSegmentSlots);
      
      //robot->SetCarryingBlock( msg.status & IS_CARRYING_BLOCK ); // Still needed?
      SetPickingOrPlacing( msg.status & IS_PICKING_OR_PLACING );
      
      SetPickedUp( msg.status & IS_PICKED_UP );
      
      // TODO: Make this a parameters somewhere?
      const f32 WheelSpeedToConsiderStopped = 2.f;
      if(std::abs(msg.lwheel_speed_mmps) < WheelSpeedToConsiderStopped &&
         std::abs(msg.rwheel_speed_mmps) < WheelSpeedToConsiderStopped)
      {
        _isMoving = false;
      } else {
        _isMoving = true;
      }
      
      
      Pose3d newPose;
      
      if(IsOnRamp()) {
        // Don't update pose history while on a ramp.
        // Instead, just compute how far the robot thinks it has gone (in the plane)
        // and compare that to where it was when it started traversing the ramp.
        // Adjust according to the angle of the ramp we know it's on.
        
        const f32 distanceTraveled = (Point2f(msg.pose_x, msg.pose_y) - _rampStartPosition).Length();
        
        Ramp* ramp = dynamic_cast<Ramp*>(_world->GetObjectByIDandFamily(_dockObjectID, BlockWorld::ObjectFamily::RAMPS));
        if(ramp == nullptr) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.NoRampWithID",
                            "Updating robot %d's state while on a ramp, but Ramp object with ID=%d not found in the world.\n",
                            _ID, _dockObjectID.GetValue());
          return RESULT_FAIL;
        }
        
        // Progress must be along ramp's direction (init assuming ascent)
        Radians headingAngle = ramp->GetPose().GetRotationAngle<'Z'>() + M_PI;
        
        // Initialize tilt angle assuming we are ascending
        Radians tiltAngle = ramp->GetAngle();
        
        if(_dockAction == DA_RAMP_DESCEND) {
          tiltAngle    *= -1.f;
          headingAngle -= M_PI;
        }
        else if(_dockAction != DA_RAMP_ASCEND) {
            PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.UnexpectedRampDockActaion",
                              "Robot is on a ramp, expecting the dock action to be either "
                              "RAMP_ASCEND or RAMP_DESCEND, not %d.\n", _dockAction);
            return RESULT_FAIL;
        }

        const f32 heightAdjust = distanceTraveled*sin(tiltAngle.ToFloat());
        const Point3f newTranslation(_rampStartPosition.x() + distanceTraveled*cos(headingAngle.ToFloat()),
                                     _rampStartPosition.y() + distanceTraveled*sin(headingAngle.ToFloat()),
                                     _rampStartHeight + heightAdjust);
        
        const RotationMatrix3d R_heading(headingAngle, Z_AXIS_3D);
        const RotationMatrix3d R_tilt(-tiltAngle, Y_AXIS_3D);
        
        newPose = Pose3d(R_tilt*R_heading, newTranslation, _pose.GetParent());
        //SetPose(newPose); // Done by UpdateCurrPoseFromHistory() below
        
      } else {
        // This is "normal" mode, where we udpate pose history based on the
        // reported odometry from the physical robot
        
        // Ignore physical robot's notion of z from the message? (msg.pose_z)
        const f32 pose_z = GetPose().GetWithRespectToOrigin().GetTranslation().z();
        
        // Need to put the odometry update in terms of the current robot origin
        newPose = Pose3d(msg.pose_angle, Z_AXIS_3D, {{msg.pose_x, msg.pose_y, pose_z}}, _worldOrigin);
        newPose = newPose.GetWithRespectToOrigin();
        
      } // if/else on ramp
      
      
      // Add to history
      lastResult = AddRawOdomPoseToHistory(msg.timestamp,
                                           msg.pose_frame_id,
                                           newPose.GetTranslation().x(),
                                           newPose.GetTranslation().y(),
                                           newPose.GetTranslation().z(),
                                           newPose.GetRotationAngle<'Z'>().ToFloat(),
                                           msg.headAngle,
                                           msg.liftAngle);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                            "AddRawOdomPoseToHistory failed for timestamp=%d\n", msg.timestamp);
        return lastResult;
      }
      
      if(UpdateCurrPoseFromHistory(*_worldOrigin) == false) {
        lastResult = RESULT_FAIL;
      }
      
      /*
       PRINT_NAMED_INFO("Robot.UpdateFullRobotState.OdometryUpdate",
       "Robot %d's pose updated to (%.3f, %.3f, %.3f) @ %.1fdeg based on "
       "msg at time=%d, frame=%d saying (%.3f, %.3f) @ %.1fdeg\n",
       _ID, _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
       _pose.GetRotationAngle<'Z'>().getDegrees(),
       msg.timestamp, msg.pose_frame_id,
       msg.pose_x, msg.pose_y, msg.pose_angle*180.f/M_PI);
       */
      
      return lastResult;
      
    } // UpdateFullRobotState()
    
    Result Robot::Update(void)
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
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime)
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
                    ExecutePath(_path);
                    break;
                  } // case DID_PLAN:
                    
                  case IPathPlanner::PLAN_NEEDED_BUT_GOAL_FAILURE:
                  {
                    ClearPath();
                    
                    PRINT_NAMED_INFO("Robot.Update.NewGoalForReplanNeeded",
                                     "Replan failed due to bad goal.\n");
                    
                    if(_reExecSequenceFcn) {
                      PRINT_NAMED_INFO("Robot.Update.Retrying",
                                       "Retrying previous ExecuteSequence call.\n");
                      _reExecSequenceFcn();
                    } else {
                      PRINT_NAMED_INFO("Robot.Update.NoReExecFcn", "Aborting path.\n");
                      SetState(IDLE);
                    }
                    break;
                  } // PLAN_NEEDED_BUT_GOAL_FAILURE:
                    
                  case IPathPlanner::PLAN_NEEDED_BUT_START_FAILURE:
                  {
                    PRINT_NAMED_INFO("Robot.Update.NewStartForReplanNeeded",
                                     "Replan failed during docking due to bad start. Will try again, and hope robot moves.\n");
                    break;
                  }
                    
                  case IPathPlanner::PLAN_NEEDED_BUT_PLAN_FAILURE:
                  {
                    PRINT_NAMED_INFO("Robot.Update.NewEnvironmentForReplanNeeded",
                                     "Replan failed during docking due to a planner failure. Will try again, and hope environment changes.\n");
                    // clear the path, but don't change the state
                    ClearPath();
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
                PRINT_NAMED_INFO("Robot.Update.FollowToNextState", "lastPathID %d\n", _lastRecvdPathID);
                SetState(_nextState);
              } else {
                PRINT_NAMED_INFO("Robot.Update.FollowPathStateButNotTraversingPath",
                                 "Robot's state is FOLLOWING_PATH, but IsTraversingPath() returned false. currPathSegment = %d\n",
                                 _currPathSegment);
              }
            }
            
            // Visualize path if robot has just started traversing it.
            // Clear the path when it has stopped.
            if ((wasTraversingPath && !IsTraversingPath()) ||
                _pose.IsSameAs(_goalPose, _goalDistanceThreshold, _goalAngleThreshold))
            {
              PRINT_INFO("Robot %d finished following path.\n", _ID);
              ClearPath(); // clear path and indicate that we are not replanning
              SetState(_nextState);
            }
          } // if waitUntilTime has passed
          break;
        } // case FOLLOWING_PATH
      
        case BEGIN_DOCKING:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime) {
            // TODO: Check that the marker was recently seen at roughly the expected image location
            // ...
            //const Point2f& imgCorners = dockMarker_->GetImageCorners().computeCentroid();
            // For now, just docking to the marker no matter where it is in the image.
            
            // Make sure the object we were docking with still exists in the world
            ActionableObject* dockObject = dynamic_cast<ActionableObject*>(_world->GetObjectByID(_dockObjectID));
            if(dockObject == nullptr) {
              PRINT_NAMED_ERROR("Robot.Update.ActionObjectNotFound",
                                "Action object with ID=%d no longer exists in the world. Returning to IDLE state.\n",
                                _dockObjectID.GetValue());
              SetState(IDLE);
              return RESULT_OK; // Robot updated successfully, so not FAIL?
            }
            
            // Verify that we ended up near enough a PreActionPose of the right type
            std::vector<ActionableObject::PoseMarkerPair_t> preActionPoseMarkerPairs;
            dockObject->GetCurrentPreActionPoses(preActionPoseMarkerPairs, {_goalPoseActionType});
            
            const Point2f currentXY(GetPose().GetTranslation().x(),
                                    GetPose().GetTranslation().y());
            
            float closestDistSq = std::numeric_limits<float>::max();
            for(auto const& preActionPair : preActionPoseMarkerPairs) {
              const Point2f preActionXY(preActionPair.first.GetTranslation().x(),
                                        preActionPair.first.GetTranslation().y());
              const float distSq = (currentXY - preActionXY).LengthSq();
              if(distSq < closestDistSq)
                closestDistSq = distSq;
            }
            
            const float distanceToGoal = sqrtf(closestDistSq);
            if(distanceToGoal > MAX_DISTANCE_TO_PREDOCK_POSE) {
              PRINT_NAMED_INFO("Robot.Update.TooFarFromGoal", "Robot is too far from pre-action pose (%.1fmm), re-docking\n", distanceToGoal);
              _reExecSequenceFcn();
            } else {
              PRINT_NAMED_INFO("Robot.Update.BeginDocking",
                               "Robot is within %.1fmm of the nearest pre-action pose, "
                               "docking with marker %d = %s (action = %d).\n",
                               distanceToGoal, _dockMarker->GetCode(),
                               Vision::MarkerTypeStrings[_dockMarker->GetCode()], _dockAction);
              
              DockWithObject(_dockObjectID, _dockMarker, _dockMarker2, _dockAction);
              _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5;
              SetState(DOCKING);
            }
          } // if _waitUntilTime has passed
          
          break;
        } // case BEGIN_DOCKING
          
        case PLACE_OBJECT_ON_GROUND:
        {
          if (BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _waitUntilTime && !IsMoving()) {
            if(IsCarryingObject() == false) {
              PRINT_NAMED_ERROR("Robot.Update.NotCarryingObject",
                                "Robot %d in PLACE_OBJECT_ON_GROUND state but not carrying object.\n", _ID);
              break;
            }
            
            _dockAction = DA_PLACE_LOW;
            _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.5f;
            
            // Compute pose of placeOnGroundPose (which is the pose of the marker on the carried object
            // that faces the robot when the object is placed in the desired pose on the ground)
            // relative to robot to yield the one-time docking error signal for placing the carried object on the ground.
            Pose3d relPose;
            if (!_placeOnGroundPose.GetWithRespectTo(_pose, relPose)) {
              PRINT_NAMED_ERROR("Robot.Update.PlaceObjectOnGroundPoseError", "\n");
            }
            
            PRINT_INFO("dockMarker wrt to robot pose: %f %f %f ang: %f\n", relPose.GetTranslation().x(), relPose.GetTranslation().y(), relPose.GetTranslation().z(), relPose.GetRotationAngle<'Z'>().ToFloat());
            
            SendPlaceObjectOnGround(relPose.GetTranslation().x(), relPose.GetTranslation().y(), relPose.GetRotationAngle<'Z'>().ToFloat() );

            SetState(DOCKING);
          }
          break;
        } // case PLACE_OBJECT_ON_GROUND
          
        case DOCKING:
        {
          if (!IsPickingOrPlacing() && !IsMoving() &&
              _waitUntilTime < BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())
          {
            // Stopped executing docking path, and should have backed out by now,
            // and have head pointed at an angle to see where we just placed or
            // picked up from. So we will check if we see a block with the same
            // ID/Type as the one we were supposed to be picking or placing, in the
            // right position.
            
            Result lastResult = RESULT_OK;
            
            switch(_dockAction)
            {
              case DA_PICKUP_LOW:
              case DA_PICKUP_HIGH:
              {
                lastResult = VerifyObjectPickup();
                if(lastResult != RESULT_OK) {
                  PRINT_NAMED_ERROR("Robot.Update.VerifyObjectPickupFailed",
                                    "VerifyObjectPickup returned error code %x.\n",
                                    lastResult);
                }
                break;
              } // case PICKUP
                
              case DA_PLACE_LOW:
              case DA_PLACE_HIGH:
              {                  
                lastResult = VerifyObjectPlacement();
                if(lastResult != RESULT_OK) {
                  PRINT_NAMED_ERROR("Robot.Update.VerifyObjectPlacementFailed",
                                    "VerifyObjectPlacement returned error code %x.\n",
                                    lastResult);
                }
                break;
              } // case PLACE
                
              case DA_RAMP_ASCEND:
              case DA_RAMP_DESCEND:
              {
                // TODO: Need to do some kind of verification here?
                PRINT_NAMED_INFO("Robot.Update.RampAscentOrDescentComplete",
                                 "Robot has completed going up/down ramp.\n");
                break;
              } // case RAMP
                
              case DA_CROSS_BRIDGE:
              {
                // TODO: Need some kind of verificaiton here?
                PRINT_NAMED_INFO("Robot.Update.BridgeCrossingComplete",
                                 "Robot has completed crossing a bridge.\n");
                break;
              }
                
              default:
                PRINT_NAMED_ERROR("Robot.Update", "Reached unknown dockAction case.\n");
                assert(false);
                return RESULT_FAIL;
                
            } // switch(_dockAction)
            
            SetState(IDLE);
          }
          break;
        } // case DOCKING
          
        default:
          PRINT_NAMED_ERROR("Robot::Update", "Transitioned to unknown state %d!\n", _state);
          assert(false);
          _state = IDLE;
          return RESULT_FAIL;
          
      } // switch(state_)
      
      
      // Visualize path if robot has just started traversing it.
      // Clear the path when it has stopped.
      if (!IsPickingOrPlacing()) {
        if (!wasTraversingPath && IsTraversingPath() && _path.GetNumSegments() > 0) {
          VizManager::getInstance()->DrawPath(_ID,_path,NamedColors::EXECUTED_PATH);
          wasTraversingPath = true;
        } else if (wasTraversingPath && !IsTraversingPath()){
          ClearPath(); // clear path and indicate that we are not replanning
          VizManager::getInstance()->ErasePath(_ID);
          VizManager::getInstance()->EraseAllPlannerObstacles(true);
          VizManager::getInstance()->EraseAllPlannerObstacles(false);
          wasTraversingPath = false;
        }
      }

      return RESULT_OK;
      
    } // Update()
    

    Result Robot::SetState(const State nextState)
    {
      Result result;
      
      switch(nextState) {
        case IDLE:
        case FOLLOWING_PATH:
        case BEGIN_DOCKING:
        case DOCKING:
        case PLACE_OBJECT_ON_GROUND:
          PRINT_INFO("Robot %d switching from state %s to state %s.\n", _ID,
                     Robot::StateNames.at(_state).c_str(),
                     Robot::StateNames.at(nextState).c_str());
          _state = nextState;
          result = RESULT_OK;
          break;

        default:
          PRINT_NAMED_ERROR("Robot::SetState", "Trying to transition to invalid state %d!\n", nextState);
          result = RESULT_FAIL;
      }

      return result;
    }

    bool Robot::IsValidHeadAngle(f32 head_angle, f32* clipped_valid_head_angle) const {
      if(head_angle < MIN_HEAD_ANGLE - HEAD_ANGLE_LIMIT_MARGIN) {
        //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too small.\n", head_angle);
        if (clipped_valid_head_angle) {
          *clipped_valid_head_angle = MIN_HEAD_ANGLE;
        }
        return false;
      }
      else if(head_angle > MAX_HEAD_ANGLE + HEAD_ANGLE_LIMIT_MARGIN) {
        //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too large.\n", head_angle);
        if (clipped_valid_head_angle) {
          *clipped_valid_head_angle = MAX_HEAD_ANGLE;
        }
        return false;
      }
      
      if (clipped_valid_head_angle) {
        *clipped_valid_head_angle = head_angle;
      }
      return true;
    }
    

    
    void Robot::SetPose(const Pose3d &newPose)
    {
      // Update our current pose and keep the name consistent
      const std::string name = _pose.GetName();
      _pose = newPose;
      _pose.SetName(name);
      
    } // SetPose()
    
    void Robot::SetHeadAngle(const f32& angle)
    {
      if (!IsValidHeadAngle(angle, &_currentHeadAngle)) {
        PRINT_NAMED_WARNING("HeadAngleOOB","angle %f  (TODO: Send correction or just recalibrate?)\n", angle);
      }
      
      // Start with canonical (untilted) headPose
      Pose3d newHeadPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-_currentHeadAngle, Y_AXIS_3D);
      newHeadPose.RotateBy(Rvec);
      newHeadPose.SetName("Camera");
      
      // Update the head camera's pose
      _camera.SetPose(newHeadPose);
      
    } // set_headAngle()

    void Robot::ComputeLiftPose(const f32 atAngle, Pose3d& liftPose)
    {
      // Reset to canonical position
      liftPose.SetRotation(atAngle, Y_AXIS_3D);
      liftPose.SetTranslation({{LIFT_ARM_LENGTH, 0.f, 0.f}});
      
      // Rotate to the given angle
      RotationVector3d Rvec(-atAngle, Y_AXIS_3D);
      liftPose.RotateBy(Rvec);
    }
    
    void Robot::SetLiftAngle(const f32& angle)
    {
      // TODO: Add lift angle limits?
      _currentLiftAngle = angle;
      
      Robot::ComputeLiftPose(_currentLiftAngle, _liftPose);

      CORETECH_ASSERT(_liftPose.GetParent() == &_liftBasePose);
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

      float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

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

    void Robot::ExecuteTestPath()
    {
      Planning::Path p;
      _longPathPlanner->GetTestPath(GetPose(), p);
      _path = p;
      ExecutePath(p);
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
    
    
    
    Result Robot::LocalizeToMat(const MatPiece* matSeen, MatPiece* existingMatPiece)
    {
      Result lastResult;
      
      if(matSeen == nullptr) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatSeenNullPointer", "\n");
        return RESULT_FAIL;
      } else if(existingMatPiece == nullptr) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.ExistingMatPieceNullPointer", "\n");
        return RESULT_FAIL;
      }
      
      PRINT_NAMED_INFO("Robot.LocalizeToMat.MatSeenChain",
                       "%s\n", matSeen->GetPose().GetNamedPathToOrigin(true).c_str());
      
      PRINT_NAMED_INFO("Robot.LocalizeToMat.ExistingMatChain",
                       "%s\n", existingMatPiece->GetPose().GetNamedPathToOrigin(true).c_str());
      
      
      // Get computed RobotPoseStamp at the time the mat was observed.
      RobotPoseStamp* posePtr = nullptr;
      if ((lastResult = GetComputedPoseAt(matSeen->GetLastObservedTime(), &posePtr)) != RESULT_OK) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.CouldNotFindHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
        return lastResult;
      }
      
      // The computed historical pose is always stored w.r.t. the robot's world
      // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
      // will work correctly
      Pose3d robotPoseAtObsTime = posePtr->GetPose();
      robotPoseAtObsTime.SetParent(_worldOrigin);
      
      /*
       // Get computed Robot pose at the time the mat was observed (note that this
       // also makes the pose have the robot's current world origin as its parent
       Pose3d robotPoseAtObsTime;
       if(robot->GetComputedPoseAt(matSeen->GetLastObservedTime(), robotPoseAtObsTime) != RESULT_OK) {
       PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.CouldNotComputeHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
       return false;
       }
       */
      
      // Get the pose of the robot with respect to the observed mat piece
      Pose3d robotPoseWrtMat;
      if(robotPoseAtObsTime.GetWithRespectTo(matSeen->GetPose(), robotPoseWrtMat) == false) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatPoseOriginMisMatch",
                          "Could not get RobotPoseStamp w.r.t. matPose.\n");
        return RESULT_FAIL;
      }
      
      // Make the computed robot pose use the existing mat piece as its parent
      robotPoseWrtMat.SetParent(&existingMatPiece->GetPose());
      //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));
      
      // Don't snap to horizontal or discrete Z levels when we see a mat marker
      // while on a ramp
      if(IsOnRamp() == false)
      {
        // If there is any significant rotation, make sure that it is roughly
        // around the Z axis
        Radians rotAngle;
        Vec3f rotAxis;
        robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
        const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
        const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
        if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane",
                              "Robot's Z axis is not well aligned with the world Z axis. "
                              "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)\n",
                              rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
        }
        
        // Snap to purely horizontal rotation and surface of the mat
        if(existingMatPiece->IsPoseOn(robotPoseWrtMat, 0, 10.f)) {
          Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
          robotPoseWrtMat_trans.z() = existingMatPiece->GetDrivingSurfaceHeight();
          robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
        }
        robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D );
        
      } // if robot is on ramp
      
      if(!_localizedToFixedMat && !existingMatPiece->IsMoveable()) {
        // If we have not yet seen a fixed mat, and this is a fixed mat, rejigger
        // the origins so that we use it as the world origin
        PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingToFixedMat",
                         "Localizing robot %d to fixed %s mat for the first time.\n",
                         GetID(), existingMatPiece->GetType().GetName().c_str());
        
        if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", GetID());
          return lastResult;
        }
        
        _localizedToFixedMat = true;
      }
      else if(IsLocalized() == false) {
        // If the robot is not yet localized, it is about to be, so we need to
        // update pose origins so that anything it has seen so far becomes rooted
        // to this mat's origin (whether mat is fixed or not)
        PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingRobotFirstTime",
                         "Localizing robot %d for the first time (to %s mat).\n",
                         GetID(), existingMatPiece->GetType().GetName().c_str());
        
        if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", GetID());
          return lastResult;
        }
        
        if(!existingMatPiece->IsMoveable()) {
          // If this also happens to be a fixed mat, then we have now localized
          // to a fixed mat
          _localizedToFixedMat = true;
        }
      }
      
      
      // Add the new vision-based pose to the robot's history. Note that we use
      // the pose w.r.t. the origin for storing poses in history.
      //RobotPoseStamp p(robot->GetPoseFrameID(), robotPoseWrtMat.GetWithRespectToOrigin(), posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToOrigin();
      if((lastResult = AddVisionOnlyPoseToHistory(existingMatPiece->GetLastObservedTime(),
                                                  robotPoseWrtOrigin.GetTranslation().x(),
                                                  robotPoseWrtOrigin.GetTranslation().y(),
                                                  robotPoseWrtOrigin.GetTranslation().z(),
                                                  robotPoseWrtOrigin.GetRotationAngle<'Z'>().ToFloat(),
                                                  posePtr->GetHeadAngle(),
                                                  posePtr->GetLiftAngle())) != RESULT_OK)
      {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedAddingVisionOnlyPoseToHistory", "\n");
        return lastResult;
      }
      
      
      // Update the computed historical pose as well so that subsequent block
      // pose updates use obsMarkers whose camera's parent pose is correct.
      // Note again that we store the pose w.r.t. the origin in history.
      // TODO: Should SetPose() do the flattening w.r.t. origin?
      posePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      
      // Compute the new "current" pose from history which uses the
      // past vision-based "ground truth" pose we just computed.
      if(UpdateCurrPoseFromHistory(existingMatPiece->GetPose()) == false) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedUpdateCurrPoseFromHistory", "\n");
        return RESULT_FAIL;
      }
      
      // Mark the robot as now being localized to this mat
      // NOTE: this should be _after_ calling AddVisionOnlyPoseToHistory, since
      //    that function checks whether the robot is already localized
      SetLocalizedTo(existingMatPiece->GetID());
      
      
      
      PRINT_INFO("Using %s mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
                 existingMatPiece->GetType().GetName().c_str(),
                 existingMatPiece->GetID().GetValue(), GetID(),
                 GetPose().GetTranslation().x(),
                 GetPose().GetTranslation().y(),
                 GetPose().GetTranslation().z(),
                 GetPose().GetRotationAngle<'Z'>().getDegrees(),
                 GetPose().GetRotationAxis().x(),
                 GetPose().GetRotationAxis().y(),
                 GetPose().GetRotationAxis().z());
      
      // Send the ground truth pose that was computed instead of the new current
      // pose and let the robot deal with updating its current pose based on the
      // history that it keeps.
      SendAbsLocalizationUpdate();
      
      return RESULT_OK;
      
    } // LocalizeToMat()
    
    
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
      Result lastResult = RESULT_FAIL;
      
      if (path.GetNumSegments() == 0) {
        PRINT_NAMED_WARNING("Robot.ExecutePath.EmptyPath", "\n");
        lastResult = RESULT_OK;
      } else {
        
        // TODO: Clear currently executing path or write to buffered path?
        lastResult = ClearPath();
        if(lastResult == RESULT_OK) {
          ++_lastSentPathID;
          pdo_->SetPath(path);
          lastResult = SendExecutePath(path);
        }
      }

      SetState(FOLLOWING_PATH);
      
      // for when the path is complete, can be overridden by caller if needed
      _nextState = IDLE;
      
      return lastResult;
    }
    
    
    Result Robot::ExecuteRampingSequence(Ramp* ramp)
    {
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteRampingSequence.NullPointer",
                          "Given ramp object pointer is null.\n");
        return RESULT_FAIL;
      }
      
      //
      // Other checks? Do we have to be carrying a block or have lift up?
      //
      
      _dockObjectID = ramp->GetID();
      
      // Choose ascent or descent
      // TODO: Better selection criteria for ascent vs. descent?
      f32 headAngle = 0.f;
      if(GetPose().GetTranslation().z() < ramp->GetPose().GetTranslation().z()) {
        _goalPose   = ramp->GetPreAscentPose();
        _dockMarker = ramp->GetFrontMarker();
        _dockAction = DA_RAMP_ASCEND;
        headAngle   = DEG_TO_RAD(-10);
      } else {
        _goalPose   = ramp->GetPreDescentPose();
        _dockMarker = ramp->GetTopMarker();
        _dockAction = DA_RAMP_DESCEND;
        headAngle   = MIN_HEAD_ANGLE;
      }
      
      // Unused for ramping:
      _dockMarker2 = nullptr;
      
      // Make the goal pose be w.r.t. the robot's current origin
      if(_goalPose.GetWithRespectTo(GetPose().FindOrigin(), _goalPose) == false) {
        PRINT_NAMED_ERROR("Robot.ExecuteRampingSequence.GoalPoseHasWrongOrigin",
                          "Goal pose provided by specified ramp does not share "
                          "Robot %d's origin.\n", _ID);
        return RESULT_FAIL;
      }
      
      _goalDistanceThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
      _goalAngleThreshold    = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;
      
      Result lastResult = ExecutePathToPose(_goalPose, headAngle);
      if(lastResult != RESULT_OK) {
        return lastResult;
      }
      
      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
      //SetState(FOLLOWING_PATH); // This should be done by ExecutePathToPose
      assert(_state == FOLLOWING_PATH);
      _nextState = BEGIN_DOCKING;
      
      // So we know how to check for success and what to do in case of failure:
      _goalPoseActionType = PreActionPose::ENTRY;
      _reExecSequenceFcn  = [this]() { return this->ExecuteTraversalSequence(this->_dockObjectID); };
      
      return RESULT_OK;
      
    } // ExecuteRampingSequence()
    

    
    Result Robot::SetOnRamp(bool t)
    {
      if(t == _onRamp) {
        // Nothing to do
        return RESULT_OK;
      }
      
      // We are either transition onto or off of a ramp
      
      Ramp* ramp = dynamic_cast<Ramp*>(_world->GetObjectByIDandFamily(_dockObjectID, BlockWorld::ObjectFamily::RAMPS));
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetOnRamp.NoRampWithID",
                          "Robot %d is transitioning on/off of a ramp, but Ramp object with ID=%d not found in the world.\n",
                          _ID, _dockObjectID.GetValue());
        return RESULT_FAIL;
      }
      
      const bool transitioningOnto = (t == true);
      
      if(transitioningOnto) {
        // Record start (x,y) position coming from robot so basestation can
        // compute actual (x,y,z) position from upcoming odometry updates
        // coming from robot (which do not take slope of ramp into account)
        _rampStartPosition = {{_pose.GetTranslation().x(), _pose.GetTranslation().y()}};
        _rampStartHeight   = _pose.GetTranslation().z();
        
        PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOntoRamp",
                         "Robot %d transitioning onto ramp %d, using start (%.1f,%.1f,%.1f)\n",
                         _ID, ramp->GetID().GetValue(), _rampStartPosition.x(), _rampStartPosition.y(), _rampStartHeight);
        
      } else {
        // Just do an absolute pose update, setting the robot's position to
        // where we "know" he should be when he finishes ascending or
        // descending the ramp
        switch(_dockAction)
        {
          case DA_RAMP_ASCEND:
            SetPose(ramp->GetPostAscentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
            break;
            
          case DA_RAMP_DESCEND:
            SetPose(ramp->GetPostDescentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
            break;
            
          default:
            PRINT_NAMED_ERROR("Robot.SetOnRamp.UnexpectedRampDockActaion",
                              "When transitioning on/off ramp, expecting the dock action to be either "
                              "RAMP_ASCEND or RAMP_DESCEND, not %d.\n", _dockAction);
            return RESULT_FAIL;
        }
        
        const TimeStamp_t timeStamp = _poseHistory.GetNewestTimeStamp();
        
        PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOffRamp",
                         "Robot %d transitioning off of ramp %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d\n",
                         _ID, ramp->GetID().GetValue(),
                         _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
                         _pose.GetRotationAngle<'Z'>().getDegrees(),
                         timeStamp);
        
        // We are creating a new pose frame at the top of the ramp
        //IncrementPoseFrameID();
        ++_frameId;
        Result lastResult = SendAbsLocalizationUpdate(_pose,
                                                      timeStamp,
                                                      _frameId);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.SetOnRamp.SendAbsLocUpdateFailed",
                            "Robot %d failed to send absolute localization update.\n", _ID);
          return lastResult;
        }

      } // if/else transitioning onto ramp
      
      _onRamp = t;
      
      return RESULT_OK;
      
    } // SetOnPose()
    
    
    Result Robot::ExecuteTraversalSequence(ObjectID objectID)
    {
      ActionableObject* object = dynamic_cast<ActionableObject*>(_world->GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteTraversalSequence.ObjectNotFound",
                          "Could not get object with ID = %d from world.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      if(object->GetType() == MatPiece::Type::LONG_BRIDGE ||
         object->GetType() == MatPiece::Type::SHORT_BRIDGE)
      {
        return ExecuteBridgeCrossingSequence(object);
      }
      else if(object->GetType() == Ramp::Type) {
        Ramp* ramp = dynamic_cast<Ramp*>(object);
        assert(ramp != nullptr);
        return ExecuteRampingSequence(ramp);
      }
      else {
        PRINT_NAMED_WARNING("Robot.ExecuteTraversalSequence.CannoTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that is not defined.\n", _ID,
                            object->GetID().GetValue(), object->GetType().GetName().c_str());
        return RESULT_OK;
      }
    } // ExecuteTraversalSequence()
    
    
    Result Robot::ExecuteBridgeCrossingSequence(ActionableObject *bridge)
    {
      if(bridge == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteBridgeCrossingSequence.NullPointer",
                          "Given bridge object pointer is null.\n");
        return RESULT_FAIL;
      }
      
      std::vector<ActionableObject::PoseMarkerPair_t> preCrossingPosePoseMarkerPairs;
      bridge->GetCurrentPreActionPoses(preCrossingPosePoseMarkerPairs, {PreActionPose::ENTRY});
      
      if(preCrossingPosePoseMarkerPairs.empty()) {
        PRINT_NAMED_ERROR("Robot.ExecuteBridgeCrossingSequence.NoPreCrossingPoses",
                          "Bridge did not provide any pre-crossing poses!\n");
        return RESULT_FAIL;
      }
      
      // Let the planner choose which pre-crossing pose to use. Create a vector of
      // pose options
      size_t selectedIndex = preCrossingPosePoseMarkerPairs.size();
      std::vector<Pose3d> preCrossingPoses;
      preCrossingPoses.reserve(preCrossingPosePoseMarkerPairs.size());
      for(auto const& preCrossingPair : preCrossingPosePoseMarkerPairs) {
        preCrossingPoses.emplace_back(preCrossingPair.first);
        
        // Make the pre-crossing poses be w.r.t. the robot's current origin
        if(preCrossingPoses.back().GetWithRespectTo(GetPose().FindOrigin(), preCrossingPoses.back()) == false) {
          PRINT_NAMED_ERROR("Robot.ExecutePreCrossingSequence.PreCrossingPoseHasWrongOrigin",
                            "Pre-crossing pose does not share Robot %d's origin.\n", _ID);
          return RESULT_FAIL;
        }
      }
      
      Result lastResult = ExecutePathToPose(preCrossingPoses, DEG_TO_RAD(-15), selectedIndex);
      if(lastResult != RESULT_OK) {
        return lastResult;
      }
      
      assert(selectedIndex < preCrossingPoses.size());
      
      _dockAction = DA_CROSS_BRIDGE;
      _dockObjectID = bridge->GetID();

      _goalPose = preCrossingPoses[selectedIndex];
      _dockMarker = &(preCrossingPosePoseMarkerPairs[selectedIndex].second);
      
      // Use the unchosen pre-crossing pose marker (the one at the other end of
      // the bridge) as dockMarker2
      assert(preCrossingPoses.size() == 2);
      size_t indexForOtherEnd = 1 - selectedIndex;
      assert(indexForOtherEnd == 0 || indexForOtherEnd == 1);
      _dockMarker2 = &(preCrossingPosePoseMarkerPairs[indexForOtherEnd].second);
      
      _goalDistanceThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
      _goalAngleThreshold    = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD;
      
      PRINT_INFO("Executing path to nearest pre-crossing pose for selected bridge: (%.2f, %.2f) @ %.1fdeg\n",
                 _goalPose.GetTranslation().x(),
                 _goalPose.GetTranslation().y(),
                 _goalPose.GetRotationAngle<'Z'>().getDegrees());
      
      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
      assert(_state == FOLLOWING_PATH);
      _nextState = BEGIN_DOCKING;
      
      // So we know how to check for success and what to do in case of failure:
      _goalPoseActionType = PreActionPose::ENTRY;
      _reExecSequenceFcn  = [this]() { return this->ExecuteTraversalSequence(this->_dockObjectID); };
      
      return lastResult;
      
    } // ExecuteBridgeCrossingSequence()
    
    
  
    Result Robot::ExecuteDockingSequence(ObjectID objectIDtoDockWith)
    {
      Result lastResult = RESULT_OK;
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_world->GetObjectByID(objectIDtoDockWith));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteDockingSequence.DockObjectDoesNotExist",
                          "Robot %d asked to dock with Object ID=%d, but it does not exist.",
                          _ID, objectIDtoDockWith.GetValue());

        return RESULT_FAIL;
      }
      
      // Choose docking action based on block's position and whether we are
      // carrying a block
      const f32 dockBlockHeight = object->GetPose().GetTranslation().z();
      _dockAction = DA_PICKUP_LOW;
      if (dockBlockHeight > 0.5f*ROBOT_BOUNDING_Z) { //  dockObject->GetSize().z()) {
        if(IsCarryingObject()) {
          PRINT_INFO("Already carrying object. Can't dock to high object. Aborting.\n");
          return RESULT_FAIL;
          
        } else {
          _dockAction = DA_PICKUP_HIGH;
        }
      } else if (IsCarryingObject()) {
        _dockAction = DA_PLACE_HIGH;
      }
      
      _dockObjectID = objectIDtoDockWith;
      _dockMarker = nullptr; // should get set to a predock pose below
      _dockMarker2 = nullptr; // unused for regular object docking
      
      std::vector<ActionableObject::PoseMarkerPair_t> preDockPoseMarkerPairs;
      //object->GetPreDockPoses(object->GetDefaultPreDockDistance(), preDockPoseMarkerPairs);
      object->GetCurrentPreActionPoses(preDockPoseMarkerPairs, {PreActionPose::DOCKING});
      
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
        
        // Make the predock poses be w.r.t. the robot's current origin
        if(preDockPoses.back().GetWithRespectTo(GetPose().FindOrigin(), preDockPoses.back()) == false) {
          PRINT_NAMED_ERROR("Robot.ExecuteDockingSequence.PreDockPoseHasWrongOrigin",
                            "Pre-dock pose does not share Robot %d's origin.\n", _ID);
          return RESULT_FAIL;
        }

      }
      
      _goalDistanceThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM;
      _goalAngleThreshold    = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD; 
      
      lastResult = ExecutePathToPose(preDockPoses, DEG_TO_RAD(-15), selectedIndex);
      if(lastResult != RESULT_OK) {
        return lastResult;
      }
      
      _goalPose = preDockPoses[selectedIndex];
      _dockMarker = &(preDockPoseMarkerPairs[selectedIndex].second);
      
      
      PRINT_INFO("Executing path to nearest pre-dock pose: (%.2f, %.2f) @ %.1fdeg\n",
                 _goalPose.GetTranslation().x(),
                 _goalPose.GetTranslation().y(),
                 _goalPose.GetRotationAngle().getDegrees());
      
      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
      //SetState(FOLLOWING_PATH); // This should be done by ExecutePathToPose above
      assert(_state == FOLLOWING_PATH);
      _nextState = BEGIN_DOCKING;
      
      // So we know how to check for success and what to do in case of failure:
      _goalPoseActionType = PreActionPose::DOCKING;
      _reExecSequenceFcn  = [this]() { return this->ExecuteDockingSequence(this->_dockObjectID); };
      
      return lastResult;
      
    } // ExecuteDockingSequence()
    
    Result Robot::ExecutePlaceObjectOnGroundSequence()
    {
      if(IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceObjectOnGroundSequence.NotCarryingObject",
                          "Robot %d was told to place a block on the ground, but "
                          "it is not carrying a block.\n", _ID);
        return RESULT_FAIL;
      }
      
      // Grab a pointer to the block we are supposedly carrying
      ActionableObject* carryingObject = dynamic_cast<ActionableObject*>(_world->GetObjectByID(_carryingObjectID));
      if(carryingObject == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceObjectOnGroundSequence.CarryObjectDoesNotExist",
                          "Robot %d thinks it is carrying a block with ID=%d, but that "
                          "block does not exist in the world.\n",
                          _ID, _carryingObjectID.GetValue());
        
        return RESULT_FAIL;
      }
      
      SetState(PLACE_OBJECT_ON_GROUND);
      
      // TODO: How to correctly set _dockMarker in case of placement failure??
      _dockMarker = &carryingObject->GetMarkers().front(); // GetMarker(Block::FRONT_FACE);
      
      
      // Set desired position of marker (via placeOnGroundPose) to be exactly where robot is now
      f32 markerAngle = _pose.GetRotationAngle<'Z'>().ToFloat();
      
      Vec3f markerPt(_pose.GetTranslation().x() + (ORIGIN_TO_LOW_LIFT_DIST_MM * cosf(markerAngle)),
                     _pose.GetTranslation().y() + (ORIGIN_TO_LOW_LIFT_DIST_MM * sinf(markerAngle)),
                     _pose.GetTranslation().z());
      
      _placeOnGroundPose.SetTranslation(markerPt);
      _placeOnGroundPose.SetRotation(_pose.GetRotationMatrix());
      
      
      return RESULT_OK;
      
    } // ExecutePlaceObjectOnGroundSequence()
    
  
    Result Robot::ExecutePlaceObjectOnGroundSequence(const Pose3d& atPose)
    {
      Result lastResult = RESULT_OK;
      
      if(IsCarryingObject() == false) {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceObjectOnGroundSequence.NotCarryingObject",
                          "Robot %d was told to place an object on the ground, but "
                          "it is not carrying a object.\n", _ID);
        return RESULT_FAIL;
      }
      
      // Grab a pointer to the block we are supposedly carrying
      ActionableObject* carryingObject = dynamic_cast<ActionableObject*>(_world->GetObjectByID(_carryingObjectID));
      if(carryingObject == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceObjectOnGroundSequence.CarryObjectDoesNotExist",
                          "Robot %d thinks it is carrying an object with ID=%d, but that "
                          "object does not exist in the world.\n",
                          _ID, _carryingObjectID.GetValue());
        
        return RESULT_FAIL;
      }
      
      // Temporarily move the block being carried to the specified pose so we can
      // get pre-dock poses for it
      const Pose3d origCarryObjectPose(carryingObject->GetPose());
      carryingObject->SetPose(atPose);
      
      // Get "pre-dock" poses that match the marker that we are docked to,
      // which in this case aren't really for docking but instead where we
      // want the robot to end up in order for the block to be
      // at the requested pose.
      std::vector<Block::PoseMarkerPair_t> preDockPoseMarkerPairs;
      carryingObject->GetCurrentPreActionPoses(preDockPoseMarkerPairs,
                                               {PreActionPose::DOCKING},
                                               {_carryingMarker->GetCode()});
      
      if (preDockPoseMarkerPairs.empty()) {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceBlockOnGroundSequence.NoPreDockPoses",
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
      
      // Note that even though we are not docking with a block, we need this
      // in the case that placement fails and we need to re-pickup the block
      // we're carrying.
      _dockMarker = &(preDockPoseMarkerPairs[selectedIndex].second);

      
      // Compute the pose of the marker in world coords
      if (!_dockMarker->GetPose().GetWithRespectTo(*_worldOrigin, _placeOnGroundPose))
      {
        PRINT_NAMED_ERROR("Robot.ExecutePlaceBlockOnGround.DockMarkerPoseFail", "\n");
      } else {
        // _placeOnGroundPose tranlsation is in world coords.
        // Now update rotation to reflect absolute angle of the marker's normal in world coords.
        std::vector<Vec3f> markerNormalVec;
        std::vector<Vec3f> relMarkerNormalVec;
        
        // Vector pointing toward inside of block along the marker normal (i.e. +ve y-axis)
        relMarkerNormalVec.emplace_back(0,1,0);
        
        _placeOnGroundPose.ApplyTo(relMarkerNormalVec, markerNormalVec);
        
        //PRINT_INFO("markerNormal: %f %f %f\n", markerNormalVec[0].x(), markerNormalVec[0].y(), markerNormalVec[0].z());
        
        Radians markerAbsAngle = atan2(markerNormalVec[0].y() - _placeOnGroundPose.GetTranslation().y(),
                                       markerNormalVec[0].x() - _placeOnGroundPose.GetTranslation().x());
        _placeOnGroundPose.SetRotation(markerAbsAngle, Z_AXIS_3D);
        
        PRINT_INFO("PlaceOnGroundPose: %f %f %f, ang: %f\n", _placeOnGroundPose.GetTranslation().x(), _placeOnGroundPose.GetTranslation().y(), _placeOnGroundPose.GetTranslation().z(), _placeOnGroundPose.GetRotationAngle<'Z'>().ToFloat());
      }
      
      
      // Put the carrying block back in its original pose (attached to lift)
      carryingObject->SetPose(origCarryObjectPose);
      
      PRINT_INFO("Executing path to nearest pre-dock pose: (%.2f, %.2f) @ %.1fdeg\n",
                 _goalPose.GetTranslation().x(),
                 _goalPose.GetTranslation().y(),
                 _goalPose.GetRotationAngle().getDegrees());
      
      _waitUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.5f;
      //SetState(FOLLOWING_PATH); // This should be done by ExecutePathToPose above
      assert(_state == FOLLOWING_PATH);
      _nextState = PLACE_OBJECT_ON_GROUND;
      
      return lastResult;
      
    } // ExecutePlaceObjectOnGroundSequence(atPose)
    
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing.
    Result Robot::DockWithObject(const ObjectID objectID,
                                 const Vision::KnownMarker* marker,
                                 const Vision::KnownMarker* marker2,
                                 const DockAction_t dockAction)
    {
      return DockWithObject(objectID, marker, marker2, dockAction, 0, 0, u8_MAX);
    }
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing. If pixel_radius == u8_MAX,
    // the marker can be seen anywhere in the image (same as above function), otherwise the
    // marker's center must be seen at the specified image coordinates
    // with pixel_radius pixels.
    Result Robot::DockWithObject(const ObjectID objectID,
                                 const Vision::KnownMarker* marker,
                                 const Vision::KnownMarker* marker2,
                                 const DockAction_t dockAction,
                                 const u16 image_pixel_x,
                                 const u16 image_pixel_y,
                                 const u8 pixel_radius)
    {
      ActionableObject* object = dynamic_cast<ActionableObject*>(_world->GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.DockWithObject.ObjectDoesNotExist",
                          "Object with ID=%d no longer exists for docking.\n", objectID.GetValue());
        return RESULT_FAIL;
      }

      CORETECH_ASSERT(marker != nullptr);
      
      _dockObjectID = objectID;
      _dockMarker  = marker;
      
      if(_dockMarker2 == nullptr) {
        marker2 = _dockMarker;
      }
      
      _dockObjectOrigPose = object->GetPose();
      
      // Dock marker has to be a child of the dock block
      if(_dockMarker->GetPose().GetParent() != &object->GetPose()) {
        PRINT_NAMED_ERROR("Robot.DockWithObject.MarkerNotOnObject",
                          "Specified dock marker must be a child of the specified dock object.\n");
        return RESULT_FAIL;
      }
      
      return SendDockWithObject(_dockMarker->GetCode(), marker2->GetCode(), _dockMarker->GetSize(), dockAction,
                                image_pixel_x, image_pixel_y, pixel_radius);
    }
    
    
    Result Robot::PickUpDockObject()
    {
      if(!_dockObjectID.IsSet()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoDockObjectIDSet",
                          "No docking object ID set, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      if(_dockMarker == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoDockMarkerSet",
                          "No docking marker set, but told to pick up object.\n");
        return RESULT_FAIL;
      }
      
      if(IsCarryingObject()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                          "Already carrying an object, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_world->GetObjectByID(_dockObjectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                          "Dock object with ID=%d no longer exists for picking up.\n", _dockObjectID.GetValue());
        return RESULT_FAIL;
      }
      
      _carryingObjectID = _dockObjectID;
      _carryingMarker   = _dockMarker;

      // Base the object's pose relative to the lift on how far away the dock
      // marker is from the center of the block
      // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
      Pose3d objectPoseWrtLiftPose;
      if(object->GetPose().GetWithRespectTo(_liftPose, objectPoseWrtLiftPose) == false) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                          "Object robot is picking up and robot's lift must share a common origin.\n");
        return RESULT_FAIL;
      }
      
      objectPoseWrtLiftPose.SetTranslation({{_dockMarker->GetPose().GetTranslation().Length() +
        LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f}});
      
      // make part of the lift's pose chain so the object will now be relative to
      // the lift and move with the robot
      objectPoseWrtLiftPose.SetParent(&_liftPose);

      // Don't reset these until we've verified the block was actually picked up
      //_dockBlockID = ANY_OBJECT;
      //_dockMarker  = nullptr;
      
      object->SetPose(objectPoseWrtLiftPose);
      object->SetIsBeingCarried(true);
      
      return RESULT_OK;
      
    } // PickUpDockBlock()
    
    
    Result Robot::VerifyObjectPickup()
    {
      // We should _not_ still see a object with the
      // same type as the one we were supposed to pick up in that
      // block's original position because we should now be carrying it.
      Vision::ObservableObject* carryObject = _world->GetObjectByID(_carryingObjectID);
      if(carryObject == nullptr) {
        PRINT_NAMED_ERROR("Robot.VerifyObjectPickup.CarryObjectNoLongerExists",
                          "Object %d we were carrying no longer exists in the world.\n",
                          _carryingObjectID.GetValue());
        return RESULT_FAIL;
      }
      
      const BlockWorld::ObjectsMapByID_t& objectsWithType = _world->GetExistingObjectsByType(carryObject->GetType());
      Pose3d P_diff;
      bool objectInOriginalPoseFound = false;
      for(auto object : objectsWithType) {
        // TODO: Make thresholds parameters
        // TODO: is it safe to always have useAbsRotation=true here?
        if(object.second->GetPose().IsSameAs_WithAmbiguity(_dockObjectOrigPose, carryObject->
                                                           GetRotationAmbiguities(),
                                                           15.f, DEG_TO_RAD(25), true, P_diff))
        {
          objectInOriginalPoseFound = true;
          break;
        }
      }
      
      if(objectInOriginalPoseFound)
      {
        // Must not actually be carrying the object I thought I was!
        _world->ClearObject(_carryingObjectID);
        _carryingObjectID.UnSet();
        PRINT_INFO("Object pick-up FAILED! (Still seeing object in same place.)\n");
      } else {
        _carryingObjectID = _dockObjectID;  // Already set?
        _carryingMarker   = _dockMarker;   //   "
        _dockObjectID.UnSet();
        _dockMarker       = nullptr;
        PRINT_INFO("Object pick-up SUCCEEDED!\n");
      }
      
      return RESULT_OK;
      
    } // VerifyObjectPickup()
    
    
    Result Robot::PlaceCarriedObject() //const TimeStamp_t atTime)
    {
      if(!_carryingObjectID.IsSet()) {
        PRINT_NAMED_WARNING("Robot.PlaceCarriedObject.CarryingObjectNotSpecified",
                            "No carrying object set, but told to place one.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_world->GetObjectByID(_carryingObjectID));
      
      if(object == nullptr)
      {
        // This really should not happen.  How can a object being carried get deleted?
        PRINT_NAMED_ERROR("Robot.PlaceCarriedObject.CarryingObjectDoesNotExist",
                          "Carrying object with ID=%d no longer exists.\n", _carryingObjectID.GetValue());
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
      liftBasePoseAtTime.SetParent(&histPosePtr->GetPose());
      
      Pose3d liftPoseAtTime;
      Robot::ComputeLiftPose(histPosePtr->GetLiftAngle(), liftPoseAtTime);
      liftPoseAtTime.SetParent(&liftBasePoseAtTime);
      
      Pose3d blockPoseAtTime(_carryingBlock->GetPose());
      blockPoseAtTime.SetParent(&liftPoseAtTime);
       
      _carryingBlock->SetPose(blockPoseAtTime.GetWithRespectTo(Pose3d::World));
      */
      
      Pose3d placedPose;
      if(object->GetPose().GetWithRespectTo(_pose.FindOrigin(), placedPose) == false) {
        PRINT_NAMED_ERROR("Robot.PlaceCarriedObject.OriginMisMatch",
                          "Could not get carrying object's pose relative to robot's origin.\n");
        return RESULT_FAIL;
      }
      object->SetPose(placedPose);
      
      object->SetIsBeingCarried(false);
      
      PRINT_NAMED_INFO("Robot.PlaceCarriedObject.ObjectPlaced",
                       "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).\n",
                       _ID, object->GetID().GetValue(),
                       object->GetPose().GetTranslation().x(),
                       object->GetPose().GetTranslation().y(),
                       object->GetPose().GetTranslation().z());

      // Don't reset _carryingObjectID here, because we want to know
      // the last object we were carrying so we can verify we see it
      // after placement. Once we *verify* we've placed it, we'll
      // do this.
      //_carryingObjectID = ANY_OBJECT;
      //_carryingMarker   = nullptr;
      
      return RESULT_OK;
      
    } // PlaceCarriedObject()
    
    
    Result Robot::VerifyObjectPlacement()
    {
      
      // In place mode, we _should_ now see an object with the ID of the
      // one we were carrying, in the place we think we left it when
      // we placed it.
      // TODO: check to see it ended up in the right place?
      Vision::ObservableObject* object = _world->GetObjectByID(_carryingObjectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.VerifyObjectPlacement.CarryObjectNoLongerExists",
                          "Object %d we were carrying no longer exists in the world.\n",
                          _carryingObjectID.GetValue());
        return RESULT_FAIL;
      }
      else if(object->GetLastObservedTime() > (GetLastMsgTimestamp()-500))
      {
        // We've seen the object in the last half second (which could
        // not be true if we were still carrying it)
        _carryingObjectID.UnSet();
        _carryingMarker   = nullptr;
        _dockObjectID.UnSet();
        _dockMarker       = nullptr;
        PRINT_INFO("Object placement SUCCEEDED!\n");
      } else {
        // TODO: correct to assume we are still carrying the object?
        _dockObjectID     = _carryingObjectID;
        _carryingObjectID.UnSet();
        PickUpDockObject(); // re-pickup block to attach it to the lift again
        PRINT_INFO("Object placement FAILED!\n");
        
      }
      
      return RESULT_OK;
      
    } // VerifyObjectPlacement()

    
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

    
    Result Robot::SendDockWithObject(const Vision::Marker::Code& markerType,
                                     const Vision::Marker::Code& markerType2,
                                     const f32 markerWidth_mm,
                                     const DockAction_t dockAction,
                                     const u16 image_pixel_x,
                                     const u16 image_pixel_y,
                                     const u8 pixel_radius) const
    {
      MessageDockWithObject m;
      m.markerWidth_mm = markerWidth_mm;
      CORETECH_ASSERT(markerType <= u8_MAX);
      m.markerType = static_cast<u8>(markerType);
      m.markerType2 = static_cast<u8>(markerType2);
      m.dockAction = dockAction;
      m.image_pixel_x = image_pixel_x;
      m.image_pixel_y = image_pixel_y;
      m.pixel_radius = pixel_radius;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendPlaceObjectOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle)
    {
      MessagePlaceObjectOnGround m;
      
      m.rel_angle = rel_angle;
      m.rel_x_mm  = rel_x;
      m.rel_y_mm  = rel_y;
      
      return _msgHandler->SendMessage(_ID, m);
    } // SendPlaceBlockOnGround()
    
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
    
    Result Robot::SendAbsLocalizationUpdate(const Pose3d&        pose,
                                            const TimeStamp_t&   t,
                                            const PoseFrameID_t& frameId) const
    {
      // TODO: Add z?
      MessageAbsLocalizationUpdate m;
      
      m.timestamp = t;
      
      m.pose_frame_id = frameId;
      
      m.xPosition = pose.GetTranslation().x();
      m.yPosition = pose.GetTranslation().y();
      
      m.headingAngle = pose.GetRotationMatrix().GetAngleAroundZaxis().ToFloat();
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendAbsLocalizationUpdate() const
    {
      // Look in history for the last vis pose and send it.
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.GetLatestVisionOnlyPose(t, p) == RESULT_FAIL) {
        PRINT_NAMED_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
        return RESULT_FAIL;
      }

      return SendAbsLocalizationUpdate(p.GetPose().GetWithRespectToOrigin(), t, p.GetFrameId());
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

    Result Robot::SendIMURequest(const u32 length_ms) const
    {
      MessageIMURequest m;
      m.length_ms = length_ms;
      
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
      const RotationMatrix2d R(atPose.GetRotationMatrix().GetAngleAroundZaxis());
      
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
      Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
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
    
    Result Robot::SendSetVisionSystemParams(VisionSystemParams_t p)
    {
      MessageSetVisionSystemParams m;
      m.minExposureTime = p.minExposureTime;
      m.maxExposureTime = p.maxExposureTime;
      m.percentileToMakeHigh = p.percentileToMakeHigh;
      m.integerCountsIncrement = p.integerCountsIncrement;
      m.highValue = p.highValue;
      return _msgHandler->SendMessage(_ID,m);
    }

    Result Robot::SendPlayAnimation(const AnimationID_t id, const u32 numLoops)
    {
      if (id < ANIM_NUM_ANIMATIONS) {
        MessagePlayAnimation m;
        m.animationID = id;
        m.numLoops = numLoops;
        return _msgHandler->SendMessage(_ID, m);
      }
      return RESULT_FAIL;
    }
    
    Result Robot::ProcessImageChunk(const MessageImageChunk &msg)
    {
      static u8 imgID = 0;
      static u32 totalImgSize = 0;
      static u8 data[ 320*240 ];
      static u32 dataSize = 0;
      static u32 width;
      static u32 height;
      
      //PRINT_INFO("Img %d, chunk %d, size %d, res %d, dataSize %d\n",
      //           msg.imageId, msg.chunkId, msg.chunkSize, msg.resolution, dataSize);
      
      // Check that resolution is supported
      if (msg.resolution != Vision::CAMERA_RES_QVGA &&
          msg.resolution != Vision::CAMERA_RES_QQVGA &&
          msg.resolution != Vision::CAMERA_RES_QQQVGA &&
          msg.resolution != Vision::CAMERA_RES_QQQQVGA &&
          msg.resolution != Vision::CAMERA_RES_VERIFICATION_SNAPSHOT
          ) {
        return RESULT_FAIL;
      }
      
      // If msgID has changed, then start over.
      if (msg.imageId != imgID) {
        imgID = msg.imageId;
        dataSize = 0;
        width = Vision::CameraResInfo[msg.resolution].width;
        height = Vision::CameraResInfo[msg.resolution].height;
        totalImgSize = width * height;
      }
      
      // Msgs are guaranteed to be received in order so just append data to array
      memcpy(data + dataSize, msg.data.data(), msg.chunkSize);
      dataSize += msg.chunkSize;
      
      // When dataSize matches the expected size, print to file
      if (dataSize >= totalImgSize) {
        if (_saveImages) {
          
          // Make sure image capture folder exists
          if (!DirExists(AnkiUtil::kP_IMG_CAPTURE_DIR)) {
            if (!MakeDir(AnkiUtil::kP_IMG_CAPTURE_DIR)) {
              PRINT_NAMED_WARNING("Robot.ProcessImageChunk.CreateDirFailed","\n");
            }
          }
          
          // Create image file
          char imgCaptureFilename[64];
          snprintf(imgCaptureFilename, sizeof(imgCaptureFilename), "%s/robot%d_img%d.pgm", AnkiUtil::kP_IMG_CAPTURE_DIR, GetID(), imgID);
          PRINT_INFO("Printing image to %s\n", imgCaptureFilename);
          Vision::WritePGM(imgCaptureFilename, data, width, height);
        }
        VizManager::getInstance()->SendGreyImage(data, (Vision::CameraResolution)msg.resolution);
      }
      
      return RESULT_OK;
    }
    
    Result Robot::ProcessIMUDataChunk(MessageIMUDataChunk const& msg)
    {
      static u8 imuSeqID = 0;
      static u32 dataSize = 0;
      static s8 imuData[6][1024];  // first ax, ay, az, gx, gy, gz
      
      // If seqID has changed, then start over.
      if (msg.seqId != imuSeqID) {
        imuSeqID = msg.seqId;
        dataSize = 0;
      }
      
      // Msgs are guaranteed to be received in order so just append data to array
      memcpy(imuData[0] + dataSize, msg.aX.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[1] + dataSize, msg.aY.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[2] + dataSize, msg.aZ.data(), IMU_CHUNK_SIZE);
      
      memcpy(imuData[3] + dataSize, msg.gX.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[4] + dataSize, msg.gY.data(), IMU_CHUNK_SIZE);
      memcpy(imuData[5] + dataSize, msg.gZ.data(), IMU_CHUNK_SIZE);
      
      dataSize += IMU_CHUNK_SIZE;
      
      // When dataSize matches the expected size, print to file
      if (msg.chunkId == msg.totalNumChunks - 1) {
        
        // Make sure image capture folder exists
        if (!Anki::DirExists(AnkiUtil::kP_IMU_LOGS_DIR)) {
          if (!MakeDir(AnkiUtil::kP_IMU_LOGS_DIR)) {
            PRINT_NAMED_WARNING("Robot.ProcessIMUDataChunk.CreateDirFailed","\n");
          }
        }
        
        // Create image file
        char logFilename[64];
        snprintf(logFilename, sizeof(logFilename), "%s/robot%d_imu%d.m", AnkiUtil::kP_IMU_LOGS_DIR, GetID(), imuSeqID);
        PRINT_INFO("Printing imu log to %s (dataSize = %d)\n", logFilename, dataSize);
        
        std::ofstream oFile(logFilename);
        for (u32 axis = 0; axis < 6; ++axis) {
          oFile << "imuData" << axis << " = [";
          for (u32 i=0; i<dataSize; ++i) {
            oFile << (s32)(imuData[axis][i]) << " ";
          }
          oFile << "];\n\n";
        }
        oFile.close();
      }
      
      return RESULT_OK;
    }

    
    
    void Robot::SaveImages(bool on)
    {
      _saveImages = on;
    }
    
    bool Robot::IsSavingImages() const
    {
      return _saveImages;
    }
    
    
    // ============ Pose history ===============
    
    Result Robot::AddRawOdomPoseToHistory(const TimeStamp_t t,
                                          const PoseFrameID_t frameID,
                                          const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                          const f32 pose_angle,
                                          const f32 head_angle,
                                          const f32 lift_angle)
    {
      return _poseHistory.AddRawOdomPose(t, frameID, pose_x, pose_y, pose_z, pose_angle, head_angle, lift_angle);
    }
    
    
    Result Robot::UpdateWorldOrigin(const Pose3d& newPoseWrtNewOrigin)
    {
      // Reverse the connection between origin and robot, and connect the new
      // reversed connection
      //CORETECH_ASSERT(p.GetPose().GetParent() == _poseOrigin);
      //Pose3d originWrtRobot = _pose.GetInverse();
      //originWrtRobot.SetParent(&newPoseOrigin);
      
      // TODO: We should only be doing this (modifying what _worldOrigin points to) when it is one of the placeHolder poseOrigins, not if it is a mat!
      std::string origName(_worldOrigin->GetName());
      *_worldOrigin = _pose.GetInverse();
      _worldOrigin->SetParent(&newPoseWrtNewOrigin);
      
      
      // Connect the old origin's pose to the same root the robot now has.
      // It is no longer the robot's origin, but for any of its children,
      // it is now in the right coordinates.
      if(_worldOrigin->GetWithRespectTo(newPoseWrtNewOrigin.FindOrigin(), *_worldOrigin) == false) {
        PRINT_NAMED_ERROR("Robot.UpdateWorldOrigin.NewLocalizationOriginProblem",
                          "Could not get pose origin w.r.t. new origin pose.\n");
        return RESULT_FAIL;
      }
      
      _worldOrigin->SetName(origName);
      
      // Now make the robot's origin point to the new origin
      // TODO: avoid the icky const_cast here...
      _worldOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());
      
      return RESULT_OK;
      
    } // UpdateWorldOrigin()
    
    
    Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                             const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                             const f32 pose_angle,
                                             const f32 head_angle,
                                             const f32 lift_angle)
    {
      /*
      if(!IsLocalized()) {

        // If we aren't localized yet, we are about to be, by virtue of this pose
        // stamp.  So we need to take the current origin (which may be the
        // the pose parent of observed objects) and update it
        
        // Reverse the connection between origin and robot
        //CORETECH_ASSERT(p.GetPose().GetParent() == _poseOrigin);
        *_poseOrigin = _pose.GetInverse();
        _poseOrigin->SetParent(&p.GetPose());
        
        // Connect the old origin's pose to the same root the robot now has.
        // It is no longer the robot's origin, but for any of its children,
        // it is now in the right coordinates.
        if(_poseOrigin->GetWithRespectTo(p.GetPose().FindOrigin(), *_poseOrigin) == false) {
          PRINT_NAMED_ERROR("Robot.AddVisionOnlyPoseToHistory.NewLocalizationOriginProblem",
                            "Could not get pose origin w.r.t. RobotPoseStamp's pose.\n");
          return RESULT_FAIL;
        }
        
        // Now make the robot's origin point to the robot's pose's parent.
        // TODO: avoid the icky const_cast here...
        _poseOrigin = const_cast<Pose3d*>(p.GetPose().GetParent());
      }
      */
      
      // We have a new ("ground truth") key frame. Increment the pose frame!
      //IncrementPoseFrameID();
      ++_frameId;
      
      return _poseHistory.AddVisionOnlyPose(t, _frameId,
                                            pose_x, pose_y, pose_z,
                                            pose_angle,
                                            head_angle,
                                            lift_angle);
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

    Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose)
    {
      RobotPoseStamp* poseStamp;
      Result lastResult = GetComputedPoseAt(t_request, &poseStamp);
      if(lastResult == RESULT_OK) {
        // Grab the pose stored in the pose stamp we just found, and hook up
        // its parent to the robot's current world origin (since pose history
        // doesn't keep track of pose parent chains)
        pose = poseStamp->GetPose();
        pose.SetParent(_worldOrigin);
      }
      return lastResult;
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
    
    bool Robot::UpdateCurrPoseFromHistory(const Pose3d& wrtParent)
    {
      bool poseUpdated = false;
      
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.ComputePoseAt(_poseHistory.GetNewestTimeStamp(), t, p) == RESULT_OK) {
        if (p.GetFrameId() == GetPoseFrameID()) {
          
          // Grab a copy of the pose from history, which has been flattened (i.e.,
          // made with respect to whatever its origin was when it was stored).
          // We just assume for now that is the same as the _current_ world origin
          // (bad assumption? or will differing frame IDs help us?), and make that
          // chaining connection so that we can get the pose w.r.t. the requested
          // parent.
          Pose3d histPoseWrtCurrentWorld(p.GetPose());
          histPoseWrtCurrentWorld.SetParent(&wrtParent.FindOrigin());
          
          Pose3d newPose;
          if((histPoseWrtCurrentWorld.GetWithRespectTo(wrtParent, newPose))==false) {
            PRINT_NAMED_ERROR("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
                              "Could not update robot %d's current pose from history w.r.t. specified pose %s.",
                              _ID, wrtParent.GetName().c_str());
          } else {
            SetPose(newPose);
            poseUpdated = true;
          }
           
        }
      }
      
      return poseUpdated;
    }
    
    
    void Robot::SetDefaultLights(const u32 eye_left_color, const u32 eye_right_color)
    {
      MessageSetDefaultLights m;
      m.eye_left_color = eye_left_color;
      m.eye_right_color = eye_right_color;
      _msgHandler->SendMessage(GetID(), m);
    }
    
    
  } // namespace Cozmo
} // namespace Anki
