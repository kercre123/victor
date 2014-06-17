//
//  robot.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__robot__
#define __Products_Cozmo__robot__

#include <queue>

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/visionMarker.h"

#include "anki/planning/shared/path.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class BlockWorld;
    class IMessageHandler;
    class IPathPlanner;
    
    class Robot
    {
    public:
      enum OperationMode {
        MOVE_LIFT,
        FOLLOW_PATH,
        INITIATE_PRE_DOCK,
        DOCK
      };
      
      Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner);
      
      void Update();
      
      // Accessors
      const RobotID_t        GetID()          const;
      const Pose3d&          GetPose()        const;
      const Vision::Camera&  GetCamera()      const;
      Vision::Camera&        GetCamera();
      
      const f32              GetHeadAngle()   const;
      const f32              GetLiftAngle()   const;
      
      const Pose3d&          GetNeckPose()    const {return _neckPose;}
      const Pose3d&          GetHeadCamPose() const {return _headCamPose;}
      const Pose3d&          GetLiftPose()    const {return _liftPose;}  // At current lift position!
      
      void SetPose(const Pose3d &newPose);
      void SetHeadAngle(const f32& angle);
      void SetLiftAngle(const f32& angle);
      void SetCameraCalibration(const Vision::CameraCalibration& calib);
      
      void IncrementPoseFrameID() {++_frameId;}
      PoseFrameID_t GetPoseFrameID() const {return _frameId;}
      
      Result GetPathToPose(const Pose3d& pose, Planning::Path& path);
      Result ExecutePathToPose(const Pose3d& pose);
      IPathPlanner* GetPathPlanner() { return _pathPlanner; }
      
      // Clears the path that the robot is executing which also stops the robot
      Result ClearPath();

      // Removes the specified number of segments from the front and back of the path
      Result TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments);
      
      // Sends a path to the robot to be immediately executed
      Result ExecutePath(const Planning::Path& path);
      
      // True if wheel speeds are non-zero in most recent RobotState message
      bool IsMoving() const {return _isMoving;}
      void SetIsMoving(bool t) {_isMoving= t;}
      
      void SetCurrPathSegment(const s8 s) {_currPathSegment = s;}
      s8   GetCurrPathSegment() {return _currPathSegment;}
      bool IsTraversingPath() {return (_currPathSegment >= 0) || _isWaitingForReplan;}

      void SetCarryingBlock(Block* carryBlock) {_carryingBlock = carryBlock;}
      bool IsCarryingBlock() {return _carryingBlock != nullptr;}

      void SetPickingOrPlacing(bool t) {_isPickingOrPlacing = t;}
      bool IsPickingOrPlacing() {return _isPickingOrPlacing;}
      
      ///////// Motor commands  ///////////
      
      // Sends message to move lift at specified speed
      Result MoveLift(const f32 speed_rad_per_sec);
      
      // Sends message to move head at specified speed
      Result MoveHead(const f32 speed_rad_per_sec);
      
      // Sends a message to the robot to move the lift to the specified height
      Result MoveLiftToHeight(const f32 height_mm,
                              const f32 max_speed_rad_per_sec,
                              const f32 accel_rad_per_sec2);
      
      // Sends a message to the robot to move the head to the specified angle
      Result MoveHeadToAngle(const f32 angle_rad,
                             const f32 max_speed_rad_per_sec,
                             const f32 accel_rad_per_sec2);
      
      Result DriveWheels(const f32 lwheel_speed_mmps,
                         const f32 rwheel_speed_mmps);
      
      Result StopAllMotors();
      
      
      // Sends a message to the robot to dock with the specified marker of the
      // specified block that it should currently be seeing.
      Result DockWithBlock(Block* block,
                           const Vision::KnownMarker* marker,
                           const DockAction_t dockAction);
      
      // Sends a message to the robot to dock with the specified marker of the
      // specified block, which it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result DockWithBlock(Block* block,
                           const Vision::KnownMarker* marker,
                           const DockAction_t dockAction,
                           const u16 image_pixel_x,
                           const u16 image_pixel_y,
                           const u8 pixel_radius);

      // Transitions the block that robot was docking with to the one that it
      // is carrying, and puts it in the robot's pose chain, attached to the
      // lift. Returns RESULT_FAIL if the robot wasn't already docking with
      // a block.
      Result PickUpDockBlock();
      
      // TODO: Implement a method for placing the block we were carrying
      Result PlaceCarriedBlock(const TimeStamp_t atTime);
      
      // Turn on/off headlight LEDs
      Result SetHeadlight(u8 intensity);
      
      ///////// Messaging ////////
      // TODO: Most of these send functions should be private and wrapped in
      // relevant state modifying functions. e.g. SendStopAllMotors() should be
      // called from StopAllMotors().
      
      // Sync time with physical robot and trigger it robot to send back camera
      // calibration
      Result SendInit() const;
      
      // Send's robot's current pose
      Result SendAbsLocalizationUpdate() const;

      // Update the head angle on the robot
      Result SendHeadAngleUpdate() const;

      // Request camera snapshot from robot
      Result SendImageRequest(const ImageSendMode_t mode) const;

      // Run a test mode
      Result SendStartTestMode(const TestMode mode) const;
      
      Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const;
      Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 paddingScale = 0.f) const;
      
      
      // =========== Pose history =============
      // Returns ref to robot's pose history
      const RobotPoseHistory& GetPoseHistory() {return _poseHistory;}
      
      Result AddRawOdomPoseToHistory(const TimeStamp_t t,
                                     const PoseFrameID_t frameID,
                                     const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                     const f32 pose_angle,
                                     const f32 head_angle,
                                     const f32 lift_angle);
      
      Result AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                        const RobotPoseStamp& p);

      Result ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                             TimeStamp_t& t, RobotPoseStamp** p,
                                             HistPoseKey* key = nullptr,
                                             bool withInterpolation = false);

      Result GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p);
      Result GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key = nullptr);
      
      bool IsValidPoseKey(const HistPoseKey key) const;
      
      // Updates the current pose to the best estimate based on
      // historical poses including vision-based poses.
      // Returns true if the pose is successfully updated, false otherwise.
      bool UpdateCurrPoseFromHistory();
      
    protected:
      // The robot's identifier
      RobotID_t        _ID;
      
      // A reference to the MessageHandler that the robot uses for outgoing comms
      IMessageHandler* _msgHandler;
      
      // A reference to the BlockWorld the robot lives in
      BlockWorld*      _world;
      
      // Path Following
      IPathPlanner*    _pathPlanner;
      Planning::Path   _path;
      s8               _currPathSegment;
      bool             _isWaitingForReplan;
      
      Vision::Camera   _camera;
      
      // Geometry / Pose
      Pose3d           _pose;
      PoseFrameID_t    _frameId;
      
      const Pose3d _neckPose; // joint around which head rotates
      const Pose3d _headCamPose; // in canonical (untilted) position w.r.t. neck joint
      const Pose3d _liftBasePose; // around which the base rotates/lifts
      Pose3d _liftPose;     // current, w.r.t. liftBasePose

      f32 _currentHeadAngle;
      f32 _currentLiftAngle;
      
      static const Quad2f CanonicalBoundingBoxXY;
      
      // Pose history
      RobotPoseHistory _poseHistory;

      // State
      OperationMode _mode, _nextMode;
      bool SetOperationMode(OperationMode newMode);
      //bool _isCarryingBlock;
      bool _isPickingOrPlacing;
      Block* _carryingBlock;
      Block* _dockBlock;
      const Vision::KnownMarker* _dockMarker;
      bool _isMoving;
      
      // Leaves input liftPose's parent alone and computes its position w.r.t.
      // liftBasePose, given the angle
      static void ComputeLiftPose(const f32 atAngle, Pose3d& liftPose);
      
      ///////// Messaging ////////
      
      // Sends message to move lift at specified speed
      Result SendMoveLift(const f32 speed_rad_per_sec) const;
      
      // Sends message to move head at specified speed
      Result SendMoveHead(const f32 speed_rad_per_sec) const;
      
      // Sends a message to the robot to move the lift to the specified height
      Result SendSetLiftHeight(const f32 height_mm,
                               const f32 max_speed_rad_per_sec,
                               const f32 accel_rad_per_sec2) const;
      
      // Sends a message to the robot to move the head to the specified angle
      Result SendSetHeadAngle(const f32 angle_rad,
                              const f32 max_speed_rad_per_sec,
                              const f32 accel_rad_per_sec2) const;
      
      Result SendDriveWheels(const f32 lwheel_speed_mmps,
                             const f32 rwheel_speed_mmps) const;
      
      Result SendStopAllMotors() const;

      // Clears the path that the robot is executing which also stops the robot
      Result SendClearPath() const;

      // Removes the specified number of segments from the front and back of the path
      Result SendTrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments) const;
      
      // Sends a path to the robot to be immediately executed
      Result SendExecutePath(const Planning::Path& path) const;
      
      // Sends a message to the robot to dock with the specified block
      // that it should currently be seeing.
      /*
      Result SendDockWithBlock(const Vision::Marker::Code& markerType,
                               const f32 markerWidth_mm,
                               const DockAction_t dockAction) const;
      */
      
      // Sends a message to the robot to dock with the specified block
      // that it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result SendDockWithBlock(const Vision::Marker::Code& markerType,
                               const f32 markerWidth_mm,
                               const DockAction_t dockAction,
                               const u16 image_pixel_x,
                               const u16 image_pixel_y,
                               const u8 pixel_radius) const;

      // Turn on/off headlight LEDs
      Result SendHeadlight(u8 intensity);
      
      
    }; // class Robot

    //
    // Inline accessors:
    //
    inline const RobotID_t Robot::GetID(void) const
    { return _ID; }
    
    inline const Pose3d& Robot::GetPose(void) const
    { return _pose; }
    
    inline const Vision::Camera& Robot::GetCamera(void) const
    { return _camera; }
    
    inline Vision::Camera& Robot::GetCamera(void)
    { return _camera; }
    
//    inline Robot::OperationMode Robot::GetOperationMode() const
//    { return this->_mode; }
    
    inline void Robot::SetCameraCalibration(const Vision::CameraCalibration& calib)
    { _camera.SetCalibration(calib); }
    
//    inline bool Robot::hasOutgoingMessages() const
//    { return not this->messagesOut.empty(); }
    
    inline const f32 Robot::GetHeadAngle() const
    { return _currentHeadAngle; }
    
    inline const f32 Robot::GetLiftAngle() const
    { return _currentLiftAngle; }
    
    //
    // RobotManager class for keeping up with available robots, by their ID
    //
    // TODO: Singleton or not?
#define USE_SINGLETON_ROBOT_MANAGER 0
    
    class RobotManager
    {
    public:
    
#if USE_SINGLETON_ROBOT_MANAGER
      // Return singleton instance
      static RobotManager* getInstance();
#else
      RobotManager();
#endif

      // Sets pointers to other managers
      // TODO: Change these to interface pointers so they can't be NULL
      Result Init(IMessageHandler* msgHandler, BlockWorld* blockWorld, IPathPlanner* pathPlanner);
      
      // Get the list of known robot ID's
      std::vector<RobotID_t> const& GetRobotIDList() const;
      
      // Get a pointer to a robot by ID
      Robot* GetRobotByID(const RobotID_t robotID);
      
      // Check whether a robot exists
      bool DoesRobotExist(const RobotID_t withID) const;
      
      // Add / remove robots
      void AddRobot(const RobotID_t withID);
      void RemoveRobot(const RobotID_t withID);
      
      // Call each Robot's Update() function
      void UpdateAllRobots();
      
      // Return a
      // Return the number of availale robots
      size_t GetNumRobots() const;
      
    protected:
      
#if USE_SINGLETON_ROBOT_MANAGER
      RobotManager(); // protected constructor for singleton class
      
      static RobotManager* singletonInstance_;
#endif
      
      IMessageHandler* _msgHandler;
      BlockWorld*      _blockWorld;
      IPathPlanner*    _pathPlanner;
      
      std::map<RobotID_t,Robot*> _robots;
      std::vector<RobotID_t>     _IDs;
      
    }; // class RobotManager
    
#if USE_SINGLETON_ROBOT_MANAGER
    inline RobotManager* RobotManager::getInstance()
    {
      if(0 == singletonInstance_) {
        singletonInstance_ = new RobotManager();
      }
      return singletonInstance_;
    }
#endif
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__robot__
