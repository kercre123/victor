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
#include "anki/common/basestation/platformPathManager.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/utils/fileManagement.h"

#include "anki/vision/CameraSettings.h"
#include "anki/vision/basestation/imageIO.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "actionQueue.h"
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
    
    Result RobotManager::Init(IMessageHandler* msgHandler)
    {
      _msgHandler  = msgHandler;
      //_pathPlanner = pathPlanner;
      
      return RESULT_OK;
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      _robots[withID] = new Robot(withID, _msgHandler);
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
    
    
    Robot::Robot(const RobotID_t robotID, IMessageHandler* msgHandler)
    : _ID(robotID)
    , _msgHandler(msgHandler)
    , _blockWorld(this)
    , _behaviorMgr(this)
    , _currPathSegment(-1)
    , _goalDistanceThreshold(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM)
    , _goalAngleThreshold(DEG_TO_RAD(10))
    , _lastSentPathID(0)
    , _lastRecvdPathID(0)
    , _wasTraversingPath(false)
    , _forceReplanOnNextWorldChange(false)
    , _saveImages(false)
    , _camera(robotID)
    , _proxLeft(0), _proxFwd(0), _proxRight(0)
    , _proxFwdBlocked(false), _proxSidesBlocked(false)
    , _frameId(0)
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
    , _carryingMarker(nullptr)
    {
      _pose.SetName("Robot_" + std::to_string(_ID));
      
      // Initializes _pose, _poseOrigins, and _worldOrigin:
      Delocalize();
      
      // The call to Delocalize() will increment frameID, but we want it to be
      // initialzied to 0, to match the physical robot's initialization
      _frameId = 0;
      
      // Read planner motion primitives
      // TODO: Use different motions primitives depending on the type/personality of this robot
      Json::Value mprims;
      const std::string subPath("coretech/planning/matlab/cozmo_mprim.json");
      const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, subPath);
      
      Json::Reader reader;
      std::ifstream jsonFile(jsonFilename);
      reader.parse(jsonFile, mprims);
      jsonFile.close();
      
      SetHeadAngle(_currentHeadAngle);
      pdo_ = new PathDolerOuter(msgHandler, robotID);
      _longPathPlanner  = new LatticePlanner(&_blockWorld, mprims);
      _shortPathPlanner = new FaceAndApproachPlanner;
      _selectedPathPlanner = _longPathPlanner;
      
    } // Constructor: Robot

    Robot::~Robot()
    {
      delete pdo_;
      pdo_ = nullptr;

      delete _longPathPlanner;
      _longPathPlanner = nullptr;
      
      delete _shortPathPlanner;
      _shortPathPlanner = nullptr;
      
      _selectedPathPlanner = nullptr;
    }
    
    void Robot::SetPickedUp(bool t)
    {
      if(_isPickedUp == false && t == true) {
        // Robot is being picked up: de-localize it
        Delocalize();
      }
      _isPickedUp = t;
    }
    
    void Robot::Delocalize()
    {
      _localizedToID.UnSet();
      _localizedToFixedMat = false;
      
      // Add a new pose origin to use until the robot gets localized again
      _poseOrigins.emplace_back();
      _poseOrigins.back().SetName("Robot" + std::to_string(_ID) + "_PoseOrigin" + std::to_string(_poseOrigins.size() - 1));
      _worldOrigin = &_poseOrigins.back();
      
      _pose.SetRotation(0, Z_AXIS_3D);
      _pose.SetTranslation({{0.f, 0.f, 0.f}});
      _pose.SetParent(_worldOrigin);
      
      _poseHistory.Clear();
      ++_frameId;
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

        VizManager::getInstance()->SetText(VizManager::TextLabelType::PROX_SENSORS,
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
        
        // Sanity check:
        CORETECH_ASSERT(_rampID.IsSet());
        
        // Don't update pose history while on a ramp.
        // Instead, just compute how far the robot thinks it has gone (in the plane)
        // and compare that to where it was when it started traversing the ramp.
        // Adjust according to the angle of the ramp we know it's on.
        
        const f32 distanceTraveled = (Point2f(msg.pose_x, msg.pose_y) - _rampStartPosition).Length();
        
        Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld.GetObjectByIDandFamily(_rampID, BlockWorld::ObjectFamily::RAMPS));
        if(ramp == nullptr) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.NoRampWithID",
                            "Updating robot %d's state while on a ramp, but Ramp object with ID=%d not found in the world.\n",
                            _ID, _rampID.GetValue());
          return RESULT_FAIL;
        }
        
        // Progress must be along ramp's direction (init assuming ascent)
        Radians headingAngle = ramp->GetPose().GetRotationAngle<'Z'>() + M_PI;
        
        // Initialize tilt angle assuming we are ascending
        Radians tiltAngle = ramp->GetAngle();
        
        switch(_rampDirection)
        {
          case Ramp::DESCENDING:
            tiltAngle    *= -1.f;
            headingAngle -= M_PI;
            break;
          case Ramp::ASCENDING:
            break;
            
          default:
            PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.UnexpectedRampDirection",
                              "Robot is on a ramp, expecting the ramp direction to be either "
                              "ASCEND or DESCENDING, not %d.\n", _rampDirection);
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
        f32 pose_z = 0.f;

        if(msg.pose_frame_id == GetPoseFrameID()) {
          // Frame IDs match. Use the robot's current Z (but w.r.t. world origin)
          pose_z = GetPose().GetWithRespectToOrigin().GetTranslation().z();
        } else {
          // This is an old odometry update from a previous pose frame ID. We
          // need to look up the correct Z value to use for putting this
          // message's (x,y) odometry info into history. Since it comes from
          // pose history, it will already be w.r.t. world origin, since that's
          // how we store everything in pose history.
          RobotPoseStamp p;
          lastResult = _poseHistory.GetLastPoseWithFrameID(msg.pose_frame_id, p);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.GetLastPoseWithFrameIdError",
                              "Failed to get last pose from history with frame ID=%d.\n", msg.pose_frame_id);
            return lastResult;
          }
          pose_z = p.GetPose().GetTranslation().z();
        }
        
        // Need to put the odometry update in terms of the current robot origin
        newPose = Pose3d(msg.pose_angle, Z_AXIS_3D, {{msg.pose_x, msg.pose_y, pose_z}}, _worldOrigin);
        
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
    
    
    Result Robot::QueueObservedMarker(const MessageVisionMarker& msg)
    {
      Result lastResult = RESULT_OK;
      
      Vision::Camera camera(GetCamera());
      
      if(!camera.IsCalibrated()) {
        PRINT_NAMED_WARNING("MessageHandler::CalibrationNotSet",
                            "Received VisionMarker message from robot before "
                            "camera calibration was set on Basestation.");
        return RESULT_FAIL;
      }
      
      // Get corners
      Quad2f corners;
      
      corners[Quad::TopLeft].x()     = msg.x_imgUpperLeft;
      corners[Quad::TopLeft].y()     = msg.y_imgUpperLeft;
      
      corners[Quad::BottomLeft].x()  = msg.x_imgLowerLeft;
      corners[Quad::BottomLeft].y()  = msg.y_imgLowerLeft;
      
      corners[Quad::TopRight].x()    = msg.x_imgUpperRight;
      corners[Quad::TopRight].y()    = msg.y_imgUpperRight;
      
      corners[Quad::BottomRight].x() = msg.x_imgLowerRight;
      corners[Quad::BottomRight].y() = msg.y_imgLowerRight;
      
      
      // Get historical robot pose at specified timestamp to get
      // head angle and to attach as parent of the camera pose.
      TimeStamp_t t;
      RobotPoseStamp* p = nullptr;
      HistPoseKey poseKey;
      lastResult = ComputeAndInsertPoseIntoHistory(msg.timestamp, t, &p, &poseKey);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("MessageHandler.ProcessMessageVisionMarker.HistoricalPoseNotFound",
                            "Time: %d, hist: %d to %d\n",
                            msg.timestamp, _poseHistory.GetOldestTimeStamp(),
                            _poseHistory.GetNewestTimeStamp());
        return lastResult;
      }
      
      // Compute pose from robot body to camera
      // Start with canonical (untilted) headPose
      Pose3d camPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-p->GetHeadAngle(), Y_AXIS_3D);
      camPose.RotateBy(Rvec);
      
      // Precompute with robot body to neck pose
      camPose.PreComposeWith(_neckPose);
      
      // Set parent pose to be the historical robot pose
      camPose.SetParent(&(p->GetPose()));
      
      camPose.SetName("PoseHistoryCamera_" + std::to_string(msg.timestamp));
      
      // Update the head camera's pose
      camera.SetPose(camPose);
      
      // Create observed marker
      Vision::ObservedMarker marker(t, msg.markerType, corners, camera);
      
      // Queue the marker for processing by the blockWorld
      _blockWorld.QueueObservedMarker(poseKey, marker);
      
      // React to the marker if there is a callback for it
      auto reactionIter = _reactionCallbacks.find(marker.GetCode());
      if(reactionIter != _reactionCallbacks.end()) {
        // Run each reaction for this code, in order:
        for(auto & reactionCallback : reactionIter->second) {
          lastResult = reactionCallback(this, &marker);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_WARNING("Robot.Update.ReactionCallbackFailed",
                                "Reaction callback failed for robot %d observing marker with code %d.\n",
                                GetID(), marker.GetCode());
          }
        }
      }
      
      // Visualize the marker in 3D
      // TODO: disable this block when not debugging / visualizing
      if(true){
        
        // Note that this incurs extra computation to compute the 3D pose of
        // each observed marker so that we can draw in the 3D world, but this is
        // purely for debug / visualization
        u32 quadID = 0;
        
        // When requesting the markers' 3D corners below, we want them
        // not to be relative to the object the marker is part of, so we
        // will request them at a "canonical" pose (no rotation/translation)
        const Pose3d canonicalPose;
        
        /*
         // Block Markers
         std::set<const Vision::ObservableObject*> const& blocks = blockLibrary_.GetObjectsWithMarker(marker);
         for(auto block : blocks) {
         std::vector<Vision::KnownMarker*> const& blockMarkers = block->GetMarkersWithCode(marker.GetCode());
         
         for(auto blockMarker : blockMarkers) {
         
         Pose3d markerPose = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
         blockMarker->Get3dCorners(canonicalPose));
         markerPose = markerPose.GetWithRespectTo(Pose3d::World);
         VizManager::getInstance()->DrawQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
         }
         }
         */
        
        // Mat Markers
        std::set<const Vision::ObservableObject*> const& mats = _blockWorld.GetObjectLibrary(BlockWorld::ObjectFamily::MATS).GetObjectsWithMarker(marker);
        for(auto mat : mats) {
          std::vector<Vision::KnownMarker*> const& matMarkers = mat->GetMarkersWithCode(marker.GetCode());
          
          for(auto matMarker : matMarkers) {
            Pose3d markerPose = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     matMarker->Get3dCorners(canonicalPose));
            if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
              VizManager::getInstance()->DrawMatMarker(quadID++, matMarker->Get3dCorners(markerPose), ::Anki::NamedColors::RED);
            } else {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                  "Cannot visualize a Mat marker whose pose origin is not the camera's origin that saw it.\n");
            }
          }
        }
        
      } // 3D marker visualization
      
      return lastResult;
      
    } // QueueObservedMarker()
    
    
    Result Robot::Update(void)
    {
      ////////// Update the robot's blockworld //////////
      
      uint32_t numBlocksObserved = 0;
      _blockWorld.Update(numBlocksObserved);
      
      
      ///////// Update the behavior manager ///////////
      
      // TODO: This object encompasses, for the time-being, what some higher level
      // module(s) would do.  e.g. Some combination of game state, build planner,
      // personality planner, etc.
      
      _behaviorMgr.Update();
      
      
      
      //////// Update Robot's State Machine /////////////
      
      IAction* currentAction = _actionQueue.GetCurrentAction();
      if(currentAction == nullptr) {
        
        VizManager::getInstance()->SetText(VizManager::TextLabelType::ACTION, NamedColors::RED, "<No Current Action>");
        
      } else {
        
        const IAction::ActionResult result = currentAction->Update();
        
        VizManager::getInstance()->SetText(VizManager::TextLabelType::ACTION, NamedColors::RED,
                                           currentAction->GetStatus().c_str());
        
        switch(result)
        {
          case IAction::SUCCESS:
            PRINT_NAMED_INFO("Robot.Update.CurrentActionSucceeded",
                             "Robot %d succeeded running action %s. Popping it off queue.\n",
                             GetID(), currentAction->GetName().c_str());
            _actionQueue.PopCurrentAction();
            break;
            
          case IAction::RUNNING:
            // Still running... (Nothing to do?)
            break;
            
          case IAction::FAILURE_PROCEED:
            PRINT_NAMED_INFO("Robot.Update.CurrentActionFailedProceeding",
                             "Robot %d failed running action %s. Popping it off queue and proceeding.\n",
                             GetID(), currentAction->GetName().c_str());

            _actionQueue.PopCurrentAction();
            break;
            
          case IAction::FAILURE_ABORT:
          case IAction::FAILURE_TIMEOUT: // TODO: handle this case differently?
            PRINT_NAMED_INFO("Robot.Update.CurrentActionFailedAborting",
                             "Robot %d failed running action %s. Clearing remainder of action queue.\n",
                             GetID(), currentAction->GetName().c_str());
            
            _actionQueue.Clear();
            break;
            
          case IAction::FAILURE_RETRY:
            PRINT_NAMED_INFO("Robot.Update.CurrentActionFailedRetrying",
                             "Robot %d failed running action %s. Retrying.\n",
                             GetID(), currentAction->GetName().c_str());
            
            if(currentAction->Retry() != RESULT_OK) {
              // No retry function, just clear the queue.
              _actionQueue.Clear();
            }
            break;
            
          default:
            PRINT_NAMED_ERROR("Robot.Update.InvalidActionResultCase",
                              "Reached unexpected default case for ActionResult.\n");
            return RESULT_FAIL;
            
        } // switch(result)
      } // if/else actionQueue not empty
      
      
      
      ////////////  Update Path Traversal /////////////
      
      // Note that we check IsPickOrPlacing() here because the physical robot
      // could be following an internally-generated Dubins path for docking, but
      // we don't want to react to that here.
      if(IsTraversingPath() && !IsPickingOrPlacing())
      {
        // If the robot is traversing a path, consider replanning it
        if(_blockWorld.DidBlocksChange())
        {
          Planning::Path newPath;
          switch(_selectedPathPlanner->GetPlan(newPath, GetPose(), _forceReplanOnNextWorldChange))
          {
            case IPathPlanner::DID_PLAN:
            {
              // clear path, but flag that we are replanning
              ClearPath();
              _wasTraversingPath = false;
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
              PRINT_NAMED_INFO("Robot.Update.NewGoalForReplanNeeded",
                               "Replan failed due to bad goal. Aborting path.\n");

              ClearPath();
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
        
      } // if IsTraversingPath()
      
      /////////// Update visualization ////////////
      
      // Draw observed markers, but only if images are being streamed
      _blockWorld.DrawObsMarkers();
      
      // Draw All Objects by calling their Visualize() methods.
      _blockWorld.DrawAllObjects();
      
      // Always draw robot w.r.t. the origin, not in its current frame
      Pose3d robotPoseWrtOrigin = GetPose().GetWithRespectToOrigin();
      
      // Triangle pose marker
      VizManager::getInstance()->DrawRobot(GetID(), robotPoseWrtOrigin);
      
      // Full Webots CozmoBot model
      VizManager::getInstance()->DrawRobot(GetID(), robotPoseWrtOrigin, GetHeadAngle(), GetLiftAngle());
      
      // Robot bounding box
      using namespace Quad;
      Quad2f quadOnGround2d = GetBoundingQuadXY(robotPoseWrtOrigin);
      const f32 zHeight = robotPoseWrtOrigin.GetTranslation().z() + 0.5f;
      Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     zHeight),
                            Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  zHeight),
                            Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    zHeight),
                            Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), zHeight));
      
      static const ColorRGBA ROBOT_BOUNDING_QUAD_COLOR(0.0f, 0.8f, 0.0f, 0.75f);
      VizManager::getInstance()->DrawRobotBoundingBox(GetID(), quadOnGround3d, ROBOT_BOUNDING_QUAD_COLOR);
      
      if(IsCarryingObject()) {
        ActionableObject* carryBlock = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(GetCarryingObject()));
        if(carryBlock == nullptr) {
          PRINT_NAMED_ERROR("BlockWorldController.CarryBlockDoesNotExist",
                            "Robot %d is marked as carrying block %d but that block no longer exists.\n",
                            GetID(), GetCarryingObject().GetValue());
          UnSetCarryingObject();
        } else {
          carryBlock->Visualize();
        }
      }
      
      // Visualize path if robot has just started traversing it.
      // Clear the path when it has stopped.
      if (!IsPickingOrPlacing()) {
        if (!_wasTraversingPath && IsTraversingPath() && _path.GetNumSegments() > 0) {
          VizManager::getInstance()->DrawPath(_ID,_path,NamedColors::EXECUTED_PATH);
          _wasTraversingPath = true;
        } else if (_wasTraversingPath && !IsTraversingPath()){
          ClearPath(); // clear path and indicate that we are not replanning
          VizManager::getInstance()->ErasePath(_ID);
          VizManager::getInstance()->EraseAllPlannerObstacles(true);
          VizManager::getInstance()->EraseAllPlannerObstacles(false);
          _wasTraversingPath = false;
        }
      }

      return RESULT_OK;
      
    } // Update()

    
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

    /*
    Result Robot::ExecutePathToPose(const Pose3d& pose)
    {
      return ExecutePathToPose(pose, GetHeadAngle());
    }
     */
    
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

    Result Robot::ExecutePathToPose(const Pose3d& pose) //, const Radians headAngle)
    {
      Planning::Path p;
      Result lastResult = GetPathToPose(pose, p);
      
      if(lastResult == RESULT_OK)
      {
        _path = p;
        _goalPose = pose;
      
        //MoveHeadToAngle(headAngle.ToFloat(), 5, 10);
        
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
      Planning::Path p;
      Result lastResult = GetPathToPose(poses, selectedIndex, p);
      
      if(lastResult == RESULT_OK) {
        _path = p;
        _goalPose = poses[selectedIndex];
        
        //MoveHeadToAngle(poses[selectedIndex].GetHeadAngle().ToFloat(), 5, 10);
        
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
      
      /* Useful for Debug:
      PRINT_NAMED_INFO("Robot.LocalizeToMat.MatSeenChain",
                       "%s\n", matSeen->GetPose().GetNamedPathToOrigin(true).c_str());
      
      PRINT_NAMED_INFO("Robot.LocalizeToMat.ExistingMatChain",
                       "%s\n", existingMatPiece->GetPose().GetNamedPathToOrigin(true).c_str());
      */
      
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
      
      if(!_localizedToFixedMat && !existingMatPiece->IsMoveable()) {
        // If we have not yet seen a fixed mat, and this is a fixed mat, rejigger
        // the origins so that we use it as the world origin
        PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingToFirstFixedMat",
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
      
      return lastResult;
    }
    
    
    Result Robot::ExecuteRampingSequence(Ramp* ramp)
    {
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteRampingSequence.NullPointer",
                          "Given ramp object pointer is null.\n");
        return RESULT_FAIL;
      }
      
      _actionQueue.QueueAtEnd(new DriveToObjectAction(*this, ramp->GetID(), PreActionPose::ENTRY));

      // TODO: Have the actions set up retries themselves?
      AscendOrDescendRampAction* ascendOrDescendAction = new AscendOrDescendRampAction(*this, ramp->GetID());
      auto retryLambda = [ramp](Robot& robot) { return robot.ExecuteRampingSequence(ramp); };
      ascendOrDescendAction->SetRetryFunction(retryLambda);
      
      _actionQueue.QueueAtEnd(ascendOrDescendAction);
      
      return RESULT_OK;
      
    } // ExecuteRampingSequence()
    

    
    Result Robot::SetOnRamp(bool t)
    {
      if(t == _onRamp) {
        // Nothing to do
        return RESULT_OK;
      }
      
      // We are either transition onto or off of a ramp
      
      Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld.GetObjectByIDandFamily(_rampID, BlockWorld::ObjectFamily::RAMPS));
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetOnRamp.NoRampWithID",
                          "Robot %d is transitioning on/off of a ramp, but Ramp object with ID=%d not found in the world.\n",
                          _ID, _rampID.GetValue());
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
        switch(_rampDirection)
        {
          case Ramp::ASCENDING:
            SetPose(ramp->GetPostAscentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
            break;
            
          case Ramp::DESCENDING:
            SetPose(ramp->GetPostDescentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
            break;
            
          default:
            PRINT_NAMED_ERROR("Robot.SetOnRamp.UnexpectedRampDirection",
                              "When transitioning on/off ramp, expecting the ramp direction to be either "
                              "ASCENDING or DESCENDING, not %d.\n", _rampDirection);
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

      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.ExecuteTraversalSequence.ObjectNotFound",
                          "Could not get actionable object with ID = %d from world.\n", objectID.GetValue());
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
        PRINT_NAMED_WARNING("Robot.ExecuteTraversalSequence.CannotTraverseObjectType",
                            "Robot %d was asked to traverse object ID=%d of type %s, but "
                            "that traversal is not defined.\n", _ID,
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
    
      _actionQueue.QueueAtEnd(new DriveToObjectAction(*this, bridge->GetID(), PreActionPose::ENTRY));
      
      CrossBridgeAction* crossBridgeAction = new CrossBridgeAction(*this, bridge->GetID());
      auto retryLambda = [bridge](Robot& robot) { return robot.ExecuteBridgeCrossingSequence(bridge); };
      crossBridgeAction->SetRetryFunction(retryLambda);
      
      _actionQueue.QueueAtEnd(crossBridgeAction);
      
      return RESULT_OK;
    
    } // ExecuteBridgeCrossingSequence()
    
    
  
    Result Robot::ExecuteDockingSequence(ObjectID objectIDtoDockWith)
    {
      Result lastResult = RESULT_OK;
      
      _actionQueue.QueueAtEnd(new DriveToObjectAction(*this, objectIDtoDockWith, PreActionPose::DOCKING));
      
      PickUpObjectAction* pickUpAction = new PickUpObjectAction(*this, objectIDtoDockWith);
      auto retryLambda = [objectIDtoDockWith](Robot& robot) { return robot.ExecuteDockingSequence(objectIDtoDockWith); };
      pickUpAction->SetRetryFunction(retryLambda);
      
      _actionQueue.QueueAtEnd(pickUpAction);
      
      return lastResult;
      
    } // ExecuteDockingSequence()
    
    
    Result Robot::ExecutePlaceObjectOnGroundSequence()
    {
      _actionQueue.QueueAtEnd(new PutDownObjectAction(*this));
      
      return RESULT_OK;
      
    } // ExecutePlaceObjectOnGroundSequence()
    
  
    Result Robot::ExecutePlaceObjectOnGroundSequence(const Pose3d& atPose)
    {
      Result lastResult = RESULT_OK;
      
      // TODO: Better way to set atPose's origin/parent?
      Pose3d atPoseWithParent(atPose);
      atPoseWithParent.SetParent(GetWorldOrigin());
      
      // TODO: Better way to set final height off the ground
      atPoseWithParent.SetTranslation({{atPose.GetTranslation().x(), atPose.GetTranslation().y(), GetPose().GetTranslation().z() + 22.f}});
      
      _actionQueue.QueueAtEnd(new DriveToPlaceCarriedObjectAction(*this, atPoseWithParent));
      _actionQueue.QueueAtEnd(new PutDownObjectAction(*this));
      
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
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.DockWithObject.ObjectDoesNotExist",
                          "Object with ID=%d no longer exists for docking.\n", objectID.GetValue());
        return RESULT_FAIL;
      }

      CORETECH_ASSERT(marker != nullptr);

      // Need to store these so that when we receive notice from the physical
      // robot that it has picked up an object we can transition the docking
      // object to being carried, using PickUpDockObject()
      _dockObjectID = objectID;
      _dockMarker  = marker;
      
      // Dock marker has to be a child of the dock block
      if(marker->GetPose().GetParent() != &object->GetPose()) {
        PRINT_NAMED_ERROR("Robot.DockWithObject.MarkerNotOnObject",
                          "Specified dock marker must be a child of the specified dock object.\n");
        return RESULT_FAIL;
      }
      
      const Vision::Marker::Code code1 = marker->GetCode();
      const Vision::Marker::Code code2 = (marker2 == nullptr ? code1 : marker2->GetCode());
                                          
      return SendDockWithObject(code1, code2, marker->GetSize(), dockAction,
                                image_pixel_x, image_pixel_y, pixel_radius);
    }
    
    
    Result Robot::PickUpObject(const ObjectID& objectID, const Vision::KnownMarker* objectMarker)
    {
      if(!objectID.IsSet()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectIDNotSet",
                          "No docking object ID set, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      if(objectMarker == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoDockMarkerSet",
                          "No docking marker set, but told to pick up object.\n");
        return RESULT_FAIL;
      }
      
      if(IsCarryingObject()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                          "Already carrying an object, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                          "Dock object with ID=%d no longer exists for picking up.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      _carryingObjectID = objectID;
      _carryingMarker   = objectMarker;

      // Base the object's pose relative to the lift on how far away the dock
      // marker is from the center of the block
      // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
      Pose3d objectPoseWrtLiftPose;
      if(object->GetPose().GetWithRespectTo(_liftPose, objectPoseWrtLiftPose) == false) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                          "Object robot is picking up and robot's lift must share a common origin.\n");
        return RESULT_FAIL;
      }
      
      objectPoseWrtLiftPose.SetTranslation({{objectMarker->GetPose().GetTranslation().Length() +
        LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f}});
      
      // make part of the lift's pose chain so the object will now be relative to
      // the lift and move with the robot
      objectPoseWrtLiftPose.SetParent(&_liftPose);
      
      object->SetPose(objectPoseWrtLiftPose);
      object->SetIsBeingCarried(true);
      
      return RESULT_OK;
      
    } // PickUpDockBlock()
    
    
    Result Robot::PlaceCarriedObject()
    {
      if(IsCarryingObject() == false) {
        PRINT_NAMED_WARNING("Robot.PlaceCarriedObject.CarryingObjectNotSpecified",
                            "Robot not carrying object, but told to place one.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(_carryingObjectID));
      
      if(object == nullptr)
      {
        // This really should not happen.  How can a object being carried get deleted?
        PRINT_NAMED_ERROR("Robot.PlaceCarriedObject.CarryingObjectDoesNotExist",
                          "Carrying object with ID=%d no longer exists.\n", _carryingObjectID.GetValue());
        return RESULT_FAIL;
      }
     
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

      _carryingObjectID.UnSet();
      _carryingMarker = nullptr;
      
      return RESULT_OK;
      
    } // PlaceCarriedObject()
    
    
    Result Robot::SetHeadlight(u8 intensity)
    {
      return SendHeadlight(intensity);
    }
    
    
    void Robot::StartBehaviorMode(BehaviorManager::Mode mode)
    {
      _behaviorMgr.StartMode(mode);
    }
    
    
    void Robot::SelectNextObjectOfInterest()
    {
      _behaviorMgr.SelectNextObjectOfInterest();
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
    
    
    Result Robot::UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin)
    {
      // Reverse the connection between origin and robot, and connect the new
      // reversed connection
      //CORETECH_ASSERT(p.GetPose().GetParent() == _poseOrigin);
      //Pose3d originWrtRobot = _pose.GetInverse();
      //originWrtRobot.SetParent(&newPoseOrigin);
      
      // TODO: get rid of nasty const_cast somehow
      Pose3d* newOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());
      newOrigin->SetParent(nullptr);
      
      // TODO: We should only be doing this (modifying what _worldOrigin points to) when it is one of the placeHolder poseOrigins, not if it is a mat!
      std::string origName(_worldOrigin->GetName());
      *_worldOrigin = _pose.GetInverse();
      _worldOrigin->SetParent(&newPoseWrtNewOrigin);
      
      // Connect the old origin's pose to the same root the robot now has.
      // It is no longer the robot's origin, but for any of its children,
      // it is now in the right coordinates.
      if(_worldOrigin->GetWithRespectTo(*newOrigin, *_worldOrigin) == false) {
        PRINT_NAMED_ERROR("Robot.UpdateWorldOrigin.NewLocalizationOriginProblem",
                          "Could not get pose origin w.r.t. new origin pose.\n");
        return RESULT_FAIL;
      }
      
      //_worldOrigin->PreComposeWith(*newOrigin);
      
      // Preserve the old world origin's name, despite updates above
      _worldOrigin->SetName(origName);
      
      // Now make the robot's world origin point to the new origin
      _worldOrigin = newOrigin;
      
      newOrigin->SetRotation(0, Z_AXIS_3D);
      newOrigin->SetTranslation({{0,0,0}});
      
      
      return RESULT_OK;
      
    } // UpdateWorldOrigin()
    
    
    Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                             const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                             const f32 pose_angle,
                                             const f32 head_angle,
                                             const f32 lift_angle)
    {      
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
    
    
    Result Robot::QueueAction(IAction* action, const bool next)
    {
      Result lastResult = RESULT_OK;

      if(next) {
        lastResult = _actionQueue.QueueNext(action);
      } else {
        lastResult = _actionQueue.QueueAtEnd(action);
      }
      
      return lastResult;
    } // QueueAction()
    
    
    Robot::ReactionCallbackIter Robot::AddReactionCallback(const Vision::Marker::Code code, ReactionCallback callback)
    {
      //CoreTechPrint("_reactionCallbacks size = %lu\n", _reactionCallbacks.size());
      
      _reactionCallbacks[code].emplace_front(callback);
      
      return _reactionCallbacks[code].cbegin();
      
    } // AddReactionCallback()
    
    
    // Remove a preivously-added callback using the iterator returned by
    // AddReactionCallback above.
    void Robot::RemoveReactionCallback(const Vision::Marker::Code code, ReactionCallbackIter callbackToRemove)
    {
      _reactionCallbacks[code].erase(callbackToRemove);
      if(_reactionCallbacks[code].empty()) {
        _reactionCallbacks.erase(code);
      }
    } // RemoveReactionCallback()
    
    
  } // namespace Cozmo
} // namespace Anki
