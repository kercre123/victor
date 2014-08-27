//
//  robot.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef ANKI_COZMO_BASESTATION_ROBOT_H
#define ANKI_COZMO_BASESTATION_ROBOT_H

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
    class MatPiece;
    class PathDolerOuter;
    class Ramp;
    
    
    class Robot
    {
    public:
      // Any state added here, must also be added to the static StateNames map,
      // and to the list of valid states in the switch statement inside SetState().
      enum State {
        IDLE,
        FOLLOWING_PATH,
        BEGIN_DOCKING, // also used for ascending/descending ramps, and crossing bridges
        DOCKING,
        PLACE_OBJECT_ON_GROUND
      };
      
      static const std::map<State, std::string> StateNames;
      
      Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner);
      ~Robot();
      
      Result Update();
      
      // Accessors
      const RobotID_t        GetID()           const;
      const Pose3d&          GetPose()         const;
      
      bool                   IsLocalized()     const {return _localizedToID.IsSet();}
      const ObjectID&        GetLocalizedTo()  const {return _localizedToID;}
      void                   SetLocalizedTo(const ObjectID& toID);
      void                   Delocalize();
      
      const Vision::Camera&             GetCamera() const;
      Vision::Camera&                   GetCamera();
      void                              SetCameraCalibration(const Vision::CameraCalibration& calib);
	    const Vision::CameraCalibration&  GetCameraCalibration() const;
      
      const f32              GetHeadAngle()    const;
      const f32              GetLiftAngle()    const;

      // Returns true if head_angle is valid.
      // *valid_head_angle is made to equal the closest valid head angle to head_angle.
      bool IsValidHeadAngle(f32 head_angle, f32* valid_head_angle = nullptr) const;
      
      const Pose3d&          GetNeckPose()       const {return _neckPose;}
      const Pose3d&          GetHeadCamPose()    const {return _headCamPose;}
      const Pose3d&          GetLiftPose()       const {return _liftPose;}  // At current lift position!
      const State            GetState()          const;
      
      const ObjectID         GetDockObject()     const {return _dockObjectID;}
      const ObjectID         GetCarryingObject() const {return _carryingObjectID;}
      
      Result SetState(const State newState);
      
      void SetPose(const Pose3d &newPose);
      const Pose3d* GetWorldOrigin() const { return _worldOrigin; }
      
      void SetHeadAngle(const f32& angle);
      void SetLiftAngle(const f32& angle);
      
      //void IncrementPoseFrameID() {++_frameId;}
      PoseFrameID_t GetPoseFrameID() const {return _frameId;}
      
      Result LocalizeToMat(const MatPiece* matSeen, MatPiece* existinMatPiece);
      
      // Clears the path that the robot is executing which also stops the robot
      Result ClearPath();

      // Removes the specified number of segments from the front and back of the path
      Result TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments);
      
      // Sends a path to the robot to be immediately executed
      // Puts Robot in FOLLOWING_PATH state. Will transition to IDLE when path is complete.
      Result ExecutePath(const Planning::Path& path);

      // Compute a path to a pose and execute it
      // Puts Robot in FOLLOWING_PATH state. Will transition to IDLE when path is complete.
      Result GetPathToPose(const Pose3d& pose, Planning::Path& path);
      Result ExecutePathToPose(const Pose3d& pose);
      Result ExecutePathToPose(const Pose3d& pose, const Radians headAngle);
      
      // Same as above, but select from a set poses and return the selected index.
      Result GetPathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex, Planning::Path& path);
      Result ExecutePathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex);
      Result ExecutePathToPose(const std::vector<Pose3d>& poses, const Radians headAngle, size_t& selectedIndex);

      // executes a test path defined in latticePlanner
      void ExecuteTestPath();

      IPathPlanner* GetPathPlanner() { return _longPathPlanner; }

      void AbortCurrentPath();
      
      // TODO: Get rid of these Set...() functions below once the processing of
      // RobotState msg is moved into the Robot class.
      
      // True if wheel speeds are non-zero in most recent RobotState message
      bool IsMoving() const {return _isMoving;}
      
      // True if we are on the sloped part of a ramp
      bool   IsOnRamp() const { return _onRamp; }
      Result SetOnRamp(bool t);

      s8   GetCurrPathSegment() {return _currPathSegment;}
      bool IsTraversingPath() {return (_currPathSegment >= 0) || (_lastSentPathID > _lastRecvdPathID);}

      u8   GetNumFreeSegmentSlots() const {return _numFreeSegmentSlots;}
      
      u16  GetLastRecvdPathID() {return _lastRecvdPathID;}
      u16  GetLastSentPathID() {return _lastSentPathID;}

      void SetCarryingObject(ObjectID carryObjectID) {_carryingObjectID = carryObjectID;}
      void UnSetCarryingObject() { _carryingObjectID.UnSet(); }
      bool IsCarryingObject() {return _carryingObjectID.IsSet(); }
      
      bool IsPickingOrPlacing() {return _isPickingOrPlacing;}
      bool IsPickedUp() {return _isPickedUp;}

      u8 GetProxLeft() {return _proxLeft;}
      u8 GetProxForward() {return _proxFwd;}
      u8 GetProxRight() {return _proxRight;}
      bool IsProxLeftBlocked() {return _proxLeftBlocked;}
      bool IsProxForwardBlocked() {return _proxFwdBlocked;}
      bool IsProxRightBlocked() {return _proxRightBlocked;}
      
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
      
      // Plan a path to an available docking pose of the specified object, and
      // then dock with it.
      Result ExecuteDockingSequence(ObjectID objectIDtoDockWith);
      
      // Plan a path to the pre-entry pose of an object and then proceed to
      // "traverse" it, depending on its type. Supports, for example, Ramp and
      // Bridge objects.
      Result ExecuteTraversalSequence(ObjectID objectID);
      
      // Plan a path to place the object currently being carried at the specified
      // pose.
      Result ExecutePlaceObjectOnGroundSequence(const Pose3d& atPose);
      
      // Put the carried object down right where the robot is now
      Result ExecutePlaceObjectOnGroundSequence();
      
      // Sends a message to the robot to dock with the specified marker of the
      // specified object that it should currently be seeing.
      Result DockWithObject(const ObjectID objectID,
                            const Vision::KnownMarker* marker,
                            const Vision::KnownMarker* marker2,
                            const DockAction_t dockAction);
      
      // Sends a message to the robot to dock with the specified marker of the
      // specified object, which it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      // marker2 needs to be specified when dockAction == DA_CROSS_BRIDGE to indiciate
      // the expected marker on the end of the bridge. Otherwise, it is ignored.
      Result DockWithObject(const ObjectID objectID,
                            const Vision::KnownMarker* marker,
                            const Vision::KnownMarker* marker2,
                            const DockAction_t dockAction,
                            const u16 image_pixel_x,
                            const u16 image_pixel_y,
                            const u8 pixel_radius);

      // Transitions the block that robot was docking with to the one that it
      // is carrying, and puts it in the robot's pose chain, attached to the
      // lift. Returns RESULT_FAIL if the robot wasn't already docking with
      // a block.
      Result PickUpDockObject();
      
      Result VerifyObjectPickup();
      
      // Places the block that the robot was carrying in its current position
      // w.r.t. the world, and removes it from the lift pose chain so it is no
      // longer attached to the robot.  Note that IsCarryingBlock() will still
      // report true, until it is actually verified that the placement worked.
      Result PlaceCarriedObject(); //const TimeStamp_t atTime);
      
      Result VerifyObjectPlacement();
      
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

      // Request imu log from robot
      Result SendIMURequest(const u32 length_ms) const;
      
      // Run a test mode
      Result SendStartTestMode(const TestMode mode) const;
      
      // Get the bounding quad of the robot at its current or a given pose
      Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const; // at current pose
      Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 paddingScale = 0.f) const; // at specific pose
      
      // Set controller gains on robot
      Result SendHeadControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      Result SendLiftControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      
      // Set VisionSystem parameters
      Result SendSetVisionSystemParams(VisionSystemParams_t p);
      
      // Play animation
      // If numLoops == 0, animation repeats forever.
      Result SendPlayAnimation(const AnimationID_t id, const u32 numLoops = 0);
      
      // For processing image chunks arriving from robot.
      // Sends complete images to VizManager for visualization.
      // If _saveImages is true, then images are saved as pgm.
      Result ProcessImageChunk(const MessageImageChunk &msg);
      
      // For processing imu data chunks arriving from robot.
      // Writes the entire log of 3-axis accelerometer and 3-axis
      // gyro readings to a .m file in kP_IMU_LOGS_DIR so they
      // can be read in from Matlab. (See robot/util/imuLogsTool.m)
      Result ProcessIMUDataChunk(MessageIMUDataChunk const& msg);
      
      // Enable/Disable saving of images constructed from ImageChunk messages as pgm files.
      void SaveImages(bool on);
      bool IsSavingImages() const;
      
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
                                        const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                        const f32 pose_angle,
                                        const f32 head_angle,
                                        const f32 lift_angle);

      Result ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                             TimeStamp_t& t, RobotPoseStamp** p,
                                             HistPoseKey* key = nullptr,
                                             bool withInterpolation = false);

      Result GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p);
      Result GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key = nullptr);
      Result GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose);
      
      TimeStamp_t GetLastMsgTimestamp() const;
      
      bool IsValidPoseKey(const HistPoseKey key) const;
      
      // Updates the current pose to the best estimate based on
      // historical poses including vision-based poses. Will use the specified
      // parent pose to store the pose.
      // Returns true if the pose is successfully updated, false otherwise.
      bool UpdateCurrPoseFromHistory(const Pose3d& wrtParent);
      
      Result UpdateFullRobotState(const MessageRobotState& msg);
      
      // ========= Lights ==========
      void SetDefaultLights(const u32 eye_left_color, const u32 eye_right_color);
      
      
    protected:
      // The robot's identifier
      RobotID_t        _ID;
      
      // A reference to the MessageHandler that the robot uses for outgoing comms
      IMessageHandler* _msgHandler;
      
      // A reference to the BlockWorld the robot lives in
      BlockWorld*      _world;
      
      // Path Following. There are two planners, only one of which can
      // be selected at a time
      IPathPlanner*    _selectedPathPlanner;
      IPathPlanner*    _longPathPlanner;
      IPathPlanner*    _shortPathPlanner;
      Planning::Path   _path;
      s8               _currPathSegment;
      u8               _numFreeSegmentSlots;
      Pose3d           _goalPose;
      f32              _goalDistanceThreshold;
      Radians          _goalAngleThreshold;
      u16              _lastSentPathID;
      u16              _lastRecvdPathID;

      // This functions sets _selectedPathPlanner to the appropriate
      // planner
      void SelectPlanner(const Pose3d& targetPose);

      PathDolerOuter* pdo_;
      
      // if true and we are traversing a path, then next time the
      // block world changes, re-plan from scratch
      bool _forceReplanOnNextWorldChange;

      // Whether or not images that are construted from incoming ImageChunk messages
      // should be saved as PGM
      bool _saveImages;

	    // Robot stores the calibration, camera just gets a reference to it
      // This is so we can share the same calibration data across multiple
      // cameras (e.g. those stored inside the pose history)
      Vision::CameraCalibration _cameraCalibration;
      Vision::Camera            _camera;
      
      // Proximity sensors
      u8 _proxLeft, _proxFwd, _proxRight;
      bool _proxLeftBlocked, _proxFwdBlocked, _proxRightBlocked;
      
      // Geometry / Pose
      std::list<Pose3d>_poseOrigins; // placeholder origin poses while robot isn't localized
      Pose3d*          _worldOrigin;
      Pose3d           _pose;
      PoseFrameID_t    _frameId;
      ObjectID         _localizedToID;
      bool             _localizedToFixedMat; // false until robot sees a _fixed_ mat
      
      Result UpdateWorldOrigin(const Pose3d& newPoseWrtNewOrigin);
      
      bool             _onRamp;
      Point2f          _rampStartPosition;
      f32              _rampStartHeight;
      
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
      bool       _isPickingOrPlacing;
      bool       _isPickedUp;
      bool       _isMoving;
      State      _state, _nextState;
      
      ObjectID                 _carryingObjectID;
      const Vision::KnownMarker* _carryingMarker;
      
      // Leaves input liftPose's parent alone and computes its position w.r.t.
      // liftBasePose, given the angle
      static void ComputeLiftPose(const f32 atAngle, Pose3d& liftPose);
      
      // Docking
      // Note that we don't store a pointer to the object because it
      // could deleted, but it is ok to hang onto a pointer to the
      // marker on that block, so long as we always verify the object
      // exists and is still valid (since, therefore, the marker must
      // be as well)
      ObjectID                    _dockObjectID;
      const Vision::KnownMarker*  _dockMarker;
      const Vision::KnownMarker*  _dockMarker2;
      DockAction_t                _dockAction;
      Pose3d                      _dockObjectOrigPose;
      
      // Desired pose of marker (on carried block) wrt world when the block
      // has been placed on the ground.
      Pose3d                      _placeOnGroundPose;
      
      f32 _waitUntilTime;

      // Plan a path to the pre-ascent/descent pose (depending on current
      // height of the robot) and then go up or down the ramp.
      Result ExecuteRampingSequence(Ramp* ramp);
      
      // Plan a path to the nearest (?) pre-crossing pose of the specified bridge
      // object, then cross it.
      Result ExecuteBridgeCrossingSequence(ActionableObject* object);

      // What function to run when "Executing" a "Sequence" fails, and what
      // type of PreActionPose to look for to check for success at the end
      // of path planning
      std::function<Result()>    _reExecSequenceFcn;
      PreActionPose::ActionType  _goalPoseActionType;
      
      
      ///////// Modifiers ////////
      
      void SetCurrPathSegment(const s8 s) {_currPathSegment = s;}
      void SetNumFreeSegmentSlots(const u8 n) {_numFreeSegmentSlots = n;}
      void SetLastRecvdPathID(u16 path_id) {_lastRecvdPathID = path_id;}
      void SetPickingOrPlacing(bool t) {_isPickingOrPlacing = t;}
      void SetPickedUp(bool t);
      void SetProxSensorData(const u8 left, const u8 forward, const u8 right,
                             bool leftBlocked, bool fwdBlocked, bool rightBlocked) {_proxLeft=left; _proxFwd=forward; _proxRight=right; _proxLeftBlocked = leftBlocked; _proxFwdBlocked = fwdBlocked; _proxRightBlocked = rightBlocked;}

      
      ///////// Messaging ////////
      
      Result SendAbsLocalizationUpdate(const Pose3d&        pose,
                                       const TimeStamp_t&   t,
                                       const PoseFrameID_t& frameId) const;
      
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
      
      // Sends a message to the robot to dock with the specified object
      // that it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result SendDockWithObject(const Vision::Marker::Code& markerType,
                                const Vision::Marker::Code& markerType2,
                                const f32 markerWidth_mm,
                                const DockAction_t dockAction,
                                const u16 image_pixel_x,
                                const u16 image_pixel_y,
                                const u8 pixel_radius) const;

      Result SendPlaceObjectOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle);
      
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
    
    inline const Robot::State Robot::GetState() const
    { return _state; }
    
    inline void Robot::SetLocalizedTo(const ObjectID& toID)
    { _localizedToID = toID;}
    
    inline void Robot::SetCameraCalibration(const Vision::CameraCalibration& calib)
    {
      _cameraCalibration = calib;
      _camera.SetSharedCalibration(&_cameraCalibration);
    }

    inline const Vision::CameraCalibration& Robot::GetCameraCalibration() const
    { return _cameraCalibration; }
    
    inline const f32 Robot::GetHeadAngle() const
    { return _currentHeadAngle; }
    
    inline const f32 Robot::GetLiftAngle() const
    { return _currentLiftAngle; }
    
    
    //
    // RobotManager class for keeping up with available robots, by their ID
    //
    
    class RobotManager
    {
    public:
    
      RobotManager();

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
      
      IMessageHandler* _msgHandler;
      BlockWorld*      _blockWorld;
      IPathPlanner*    _pathPlanner;
      
      std::map<RobotID_t,Robot*> _robots;
      std::vector<RobotID_t>     _IDs;
      
    }; // class RobotManager
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOT_H
