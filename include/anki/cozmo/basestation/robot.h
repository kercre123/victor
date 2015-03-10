//
//  robot.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//
/**
 * File: robot.h
 *
 * Author: Andrew Stein
 * Date:   8/23/13
 *
 * Description: Defines a Robot representation on the Basestation, which is 
 *              in charge of communicating with (and mirroring the state of)
 *              a physical (hardware) robot.  
 *
 *              Convention: Set*() methods do not actually command the physical
 *              robot to do anything; they simply update some aspect of the 
 *              state or internal representation of the Basestation Robot. 
 *              To command the robot to "do" something, methods beginning with
 *              other action words, or add IAction objects to its ActionList.
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef ANKI_COZMO_BASESTATION_ROBOT_H
#define ANKI_COZMO_BASESTATION_ROBOT_H

#include <queue>

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"

#include "anki/planning/shared/path.h"

#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/activeBlockTypes.h"

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
#include "anki/cozmo/basestation/visionProcessingThread.h"

#include "anki/cozmo/basestation/actionContainers.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/soundManager.h"

#define ASYNC_VISION_PROCESSING 0

namespace Anki {
  namespace Cozmo {
    
    // Forward declarations:
    class IRobotMessageHandler;
    class IPathPlanner;
    class MatPiece;
    class PathDolerOuter;
    class RobotPoseHistory;
    class RobotPoseStamp;
    
    class Robot
    {
    public:
      
      Robot(const RobotID_t robotID, IRobotMessageHandler* msgHandler);
      ~Robot();
      
      Result Update();
      
      Result UpdateFullRobotState(const MessageRobotState& msg);
      
      
      // Accessors
      const RobotID_t        GetID()           const;
      BlockWorld&            GetBlockWorld()   {return _blockWorld;}
      const BlockWorld&      GetBlockWorld() const { return _blockWorld;}
      
      //
      // Localization
      //
      bool                   IsLocalized()     const {return _localizedToID.IsSet();}
      const ObjectID&        GetLocalizedTo()  const {return _localizedToID;}
      void                   SetLocalizedTo(const ObjectID& toID);
      void                   Delocalize();
      
      Result LocalizeToMat(const MatPiece* matSeen, MatPiece* existinMatPiece);
      
      // Returns true if robot is not traversing a path and has no actions in its queue.
      bool   IsIdle() const { return !IsTraversingPath() && _actionList.IsEmpty(); }
      
      // True if wheel speeds are non-zero in most recent RobotState message
      bool   IsMoving() const {return _isMoving;}
      
      // True if we are on the sloped part of a ramp
      bool   IsOnRamp() const { return _onRamp; }
      
      // Set whether or not the robot is on a ramp
      Result SetOnRamp(bool t);
      
      // Just sets the ramp to use and in which direction, not whether robot is on it yet
      void   SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction);
      
      //
      // Camera / Vision
      //
      const Vision::Camera&             GetCamera() const;
      Vision::Camera&                   GetCamera();
      void                              SetCameraCalibration(const Vision::CameraCalibration& calib);
	    const Vision::CameraCalibration&  GetCameraCalibration() const;
     
      Result QueueObservedMarker(const MessageVisionMarker& msg);
      
      // Get a *copy* of the current image on this robot's vision processing thread
      bool GetCurrentImage(Vision::Image& img, TimeStamp_t newerThan);
      
      //
      // Pose (of the robot or its parts)
      //
      const Pose3d&          GetPose()         const;
      const f32              GetHeadAngle()    const;
      const f32              GetLiftAngle()    const;
      const Pose3d&          GetLiftPose()     const {return _liftPose;}  // At current lift position!
      const PoseFrameID_t    GetPoseFrameID()  const {return _frameId;}
      const Pose3d*          GetWorldOrigin() const { return _worldOrigin; }

      // These change the robot's internal (basestation) representation of its
      // pose, head angle, and lift angle, but do NOT actually command the
      // physical robot to do anything!
      void SetPose(const Pose3d &newPose);
      void SetHeadAngle(const f32& angle);
      void SetLiftAngle(const f32& angle);
      
      // Get the bounding quad of the robot at its current or a given pose
      Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const; // at current pose
      Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 paddingScale = 0.f) const; // at specific pose
      
      // Return current height of lift's gripper
      f32 GetLiftHeight() const;
      
      // Return current bounding height of the robot, taking into account whether lift
      // is raised
      f32 GetHeight() const;
      
      // Wheel speeds, mm/sec
      f32 GetLeftWheelSpeed() const { return _leftWheelSpeed_mmps; }
      f32 GetRigthWheelSpeed() const { return _rightWheelSpeed_mmps; }
      
      //
      // Path Following
      //

      // Clears the path that the robot is executing which also stops the robot
      // (so this also aborts any current path)
      Result ClearPath();
      
      // Removes the specified number of segments from the front and back of the path
      Result TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments);

      // Return a path to the given pose, internally selecting the best planner
      // to use to generate it.
      Result GetPathToPose(const Pose3d& pose, Planning::Path& path);
      
      // Same as above, but allows the planner to also select the "best" of the
      // given poses, returning the index of which one it selected.
      Result GetPathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex, Planning::Path& path);

      // Sends a path to the robot to be immediately executed
      Result ExecutePath(const Planning::Path& path, const bool useManualSpeed = false);
      
      // Executes a test path defined in latticePlanner
      void ExecuteTestPath();
      
      IPathPlanner* GetPathPlanner() { return _selectedPathPlanner; }
      
      bool IsTraversingPath()   const {return (_currPathSegment >= 0) || (_lastSentPathID > _lastRecvdPathID);}
      
      u16  GetLastRecvdPathID() const {return _lastRecvdPathID;}
      u16  GetLastSentPathID()  const {return _lastSentPathID;}

      
      
      //
      // Object Docking / Carrying
      //

      const ObjectID&             GetDockObject()     const {return _dockObjectID;}
      const ObjectID&             GetCarryingObject() const {return _carryingObjectID;}
      const Vision::KnownMarker*  GetCarryingMarker() const {return _carryingMarker; }

      bool IsCarryingObject()   const {return _carryingObjectID.IsSet(); }
      bool IsPickingOrPlacing() const {return _isPickingOrPlacing;}
      bool IsPickedUp()         const {return _isPickedUp;}
      
      void SetCarryingObject(ObjectID carryObjectID);
      void UnSetCarryingObject();
      
      // Tell the physical robot to dock with the specified marker
      // of the specified object that it should currently be seeing.
      // If pixel_radius == u8_MAX, the marker can be seen anywhere in the image,
      // otherwise the marker's center must be seen within pixel_radius of the
      // specified image coordinates.
      // marker2 needs to be specified when dockAction == DA_CROSS_BRIDGE to indiciate
      // the expected marker on the end of the bridge. Otherwise, it is ignored.
      Result DockWithObject(const ObjectID objectID,
                                const Vision::KnownMarker* marker,
                                const Vision::KnownMarker* marker2,
                                const DockAction_t dockAction,
                                const u16 image_pixel_x,
                                const u16 image_pixel_y,
                                const u8 pixel_radius,
                                const bool useManualSpeed = false);
      
      // Same as above but without specifying image location for marker
      Result DockWithObject(const ObjectID objectID,
                            const Vision::KnownMarker* marker,
                            const Vision::KnownMarker* marker2,
                            const DockAction_t dockAction,
                            const bool useManualSpeed = false);
      
      // Transitions the object that robot was docking with to the one that it
      // is carrying, and puts it in the robot's pose chain, attached to the
      // lift. Returns RESULT_FAIL if the robot wasn't already docking with
      // an object.
      Result SetDockObjectAsAttachedToLift();
      
      // Same as above, but with specified object
      Result SetObjectAsAttachedToLift(const ObjectID& dockObjectID,
                                       const Vision::KnownMarker* dockMarker);
      
      // Places the object that the robot was carrying in its current position
      // w.r.t. the world, and removes it from the lift pose chain so it is no
      // longer "attached" to the robot.
      Result SetCarriedObjectAsUnattached();
      
      Result StopDocking();
      
      //
      // Proximity Sensors
      //
      u8   GetProxSensorVal(ProxSensor_t sensor)    const {return _proxVals[sensor];}
      bool IsProxSensorBlocked(ProxSensor_t sensor) const {return _proxBlocked[sensor];}
      
      // Pose of where objects are assumed to be with respect to robot pose when
      // obstacles are detected by proximity sensors
      static const Pose3d ProxDetectTransform[NUM_PROX];
      
      
      //
      // Vision
      //
      Result ProcessImage(const Vision::Image& image);
      Result StartFaceTracking(u8 timeout_sec);
      Result StopFaceTracking();
      Result StartLookingForMarkers();
      Result StopLookingForMarkers();
      
      // Save the next image passed into ProcessImage() to file
      void SaveNextImage();
      
      // =========== Actions Commands =============
      
      // Return a reference to the robot's action list for directly adding things
      // to do, either "now" or in queues.
      // TODO: This seems simpler than writing/maintaining wrappers, but maybe that would be better?
      ActionList& GetActionList() { return _actionList; }
      
      // Below are low-level actions to tell the robot to do something "now"
      // without using the ActionList system:
      
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
      
      // If robot observes the given object ID, it will adjust its head angle to look
      // at last-observed marker. Fails if objectID doesn't exist.
      Result EnableTrackHeadToObject(const u32 objectID);
      Result DisableTrackHeadToObject();
      
      const ObjectID& GetTrackHeadToObject() const { return _trackHeadToObjectID; }
      
      Result DriveWheels(const f32 lwheel_speed_mmps,
                         const f32 rwheel_speed_mmps);
      
      Result StopAllMotors();
      
      // Send a message to the robot to place whatever it is carrying on the
      // ground right where it is. Returns RESULT_FAIL if robot is not carrying
      // anything.
      Result PlaceObjectOnGround(const bool useManualSpeed = false);
      
      // Plays specified animation numLoops times.
      // If numLoops == 0, animation repeats forever.
      Result PlayAnimation(const char* animName, const u32 numLoops = 1);
      
      // Return the ID matching the given name, or -1 if no animation with
      // that name is known.
      s32 GetAnimationID(const std::string& animationName) const;
      
      // Ask the UI to play a sound for us
      Result PlaySound(SoundID_t soundID, u8 numLoops, u8 volume);
      void   StopSound();
      
      // Plays transition animation once, then plays state animatin in a loop
      Result TransitionToStateAnimation(const char *transitionAnimName,
                                        const char *stateAnimName);
      
      Result StopAnimation();
      
      // (Re-)Read the animation JSON file and send it to the physical robot
      Result ReadAnimationFile();
      
      // Returns true if the robot is currently playing an animation, according
      // to most recent state message.
      bool IsAnimating() const;
      
      Result SyncTime();
      
      // Turn on/off headlight LEDs
      Result SetHeadlight(u8 intensity);
      
      Result RequestImage(const ImageSendMode_t mode,
                          const Vision::CameraResolution resolution) const;
      
      Result RequestIMU(const u32 length_ms) const;

      // Tell the robot to start a given test mode
      Result StartTestMode(const TestMode mode, s32 p1, s32 p2, s32 p3) const;

      // Start a Behavior in BehaviorManager
      void StartBehaviorMode(BehaviorManager::Mode mode);
     
      // Set a particular behavior state (up to BehaviorManager to ignore if
      // state is not valid for current mode)
      void SetBehaviorState(BehaviorManager::BehaviorState state);
      
      // For debugging robot parameters:
      Result SetHeadControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      Result SetLiftControllerGains(const f32 kp, const f32 ki, const f32 maxIntegralError);
      Result SendVisionSystemParams(VisionSystemParams_t p);
      Result SendFaceDetectParams(FaceDetectParams_t p);
      
      // =========== Pose history =============
      
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

      TimeStamp_t GetLastMsgTimestamp() const;
      
      bool IsValidPoseKey(const HistPoseKey key) const;
      
      // Updates the current pose to the best estimate based on
      // historical poses including vision-based poses. Will use the specified
      // parent pose to store the pose.
      // Returns true if the pose is successfully updated, false otherwise.
      bool UpdateCurrPoseFromHistory(const Pose3d& wrtParent);

      
      // ============= Reactions =============
      using ReactionCallback = std::function<Result(Robot*,Vision::ObservedMarker*)>;
      using ReactionCallbackIter = std::list<ReactionCallback>::const_iterator;
      
      // Add a callback function to be run as a "reaction" when the robot
      // sees the specified VisionMarker. The returned iterator can be
      // used to remove the callback via the method below.
      ReactionCallbackIter AddReactionCallback(const Vision::Marker::Code code, ReactionCallback callback);
      
      // Remove a previously-added callback using the iterator returned by
      // AddReactionCallback above.
      void RemoveReactionCallback(const Vision::Marker::Code code, ReactionCallbackIter callbackToRemove);
      
      // ========= Lights ==========
      void SetDefaultLights(const u32 eye_left_color, const u32 eye_right_color);
     
      
      // =========  Block messages  ============
      // Set the LED colors/flashrates individually (ordered by BlockLEDPosition)
      Result SetBlockLights(const u8 blockID,
                            const std::array<u32,NUM_BLOCK_LEDS>& color,
                            const std::array<u32,NUM_BLOCK_LEDS>& onPeriod_ms,
                            const std::array<u32,NUM_BLOCK_LEDS>& offPeriod_ms);
      
      // Set all LEDs of the specified block to the same color/flashrate
      Result SetBlockLights(const u8 blockID, const u32 color, const u32 onPeriod_ms, const u32 offPeriod_ms);
      
      // =========  Other State  ============
      f32 GetBatteryVoltage() const { return _battVoltage; }
      
      // Abort everything the robot is doing, including path following, actions,
      // animations, and docking. This is like the big red E-stop button.
      // TODO: Probably need a more elegant way of doing this.
      Result AbortAll();
      
    protected:
      
      // The robot's identifier
      RobotID_t         _ID;
      
      // Timestamp of last robotStateMessage (so we can check to see if we've lost connection)
      double       _lastStateMsgTime_sec;
      
      // A reference to the MessageHandler that the robot uses for outgoing comms
      IRobotMessageHandler* _msgHandler;
            
      // A reference to the BlockWorld the robot lives in
      BlockWorld        _blockWorld;
      
      VisionProcessingThread _visionProcessor;
#     if !ASYNC_VISION_PROCESSING
      Vision::Image     _image;
      MessageRobotState _robotStateForImage;
      bool              _haveNewImage;
#     endif
      
      BehaviorManager  _behaviorMgr;
      
      //ActionQueue      _actionQueue;
      ActionList       _actionList;
      
      // Path Following. There are two planners, only one of which can
      // be selected at a time
      IPathPlanner*    _selectedPathPlanner;
      IPathPlanner*    _longPathPlanner;
      IPathPlanner*    _shortPathPlanner;
      s8               _currPathSegment;
      u8               _numFreeSegmentSlots;
      u16              _lastSentPathID;
      u16              _lastRecvdPathID;
      PathDolerOuter*  _pdo;
      
      // This functions sets _selectedPathPlanner to the appropriate
      // planner
      void SelectPlanner(const Pose3d& targetPose);
      
	    // Robot stores the calibration, camera just gets a reference to it
      // This is so we can share the same calibration data across multiple
      // cameras (e.g. those stored inside the pose history)
      Vision::CameraCalibration _cameraCalibration;
      Vision::Camera            _camera;
      
      // Proximity sensors
      u8               _proxVals[NUM_PROX];
      bool             _proxBlocked[NUM_PROX];
      
      // Geometry / Pose
      std::list<Pose3d>_poseOrigins; // placeholder origin poses while robot isn't localized
      Pose3d*          _worldOrigin;
      Pose3d           _pose;
      PoseFrameID_t    _frameId;
      ObjectID         _localizedToID;       // ID of mat object robot is localized to
      bool             _localizedToFixedMat; // false until robot sees a _fixed_ mat
      
      Result UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin);
      
      const Pose3d     _neckPose;     // joint around which head rotates
      const Pose3d     _headCamPose;  // in canonical (untilted) position w.r.t. neck joint
      const Pose3d     _liftBasePose; // around which the base rotates/lifts
      Pose3d           _liftPose;     // current, w.r.t. liftBasePose

      f32              _currentHeadAngle;
      f32              _currentLiftAngle;
      
      f32              _leftWheelSpeed_mmps;
      f32              _rightWheelSpeed_mmps;
      
      static const Quad2f CanonicalBoundingBoxXY;
      
      // Ramping
      bool             _onRamp;
      ObjectID         _rampID;
      Point2f          _rampStartPosition;
      f32              _rampStartHeight;
      Ramp::TraversalDirection _rampDirection;
      
      // State
      bool             _isPickingOrPlacing;
      bool             _isPickedUp;
      bool             _isMoving;
      bool             _isAnimating;
      f32              _battVoltage;
      
      // Pose history
      Result ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                             TimeStamp_t& t, RobotPoseStamp** p,
                                             HistPoseKey* key = nullptr,
                                             bool withInterpolation = false);
      
      Result GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p);
      Result GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key = nullptr);
      Result GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose);
      
      RobotPoseHistory* _poseHistory;
      
      // Leaves input liftPose's parent alone and computes its position w.r.t.
      // liftBasePose, given the angle
      static void ComputeLiftPose(const f32 atAngle, Pose3d& liftPose);
      
      // Docking / Carrying
      // Note that we don't store a pointer to the object because it
      // could deleted, but it is ok to hang onto a pointer to the
      // marker on that block, so long as we always verify the object
      // exists and is still valid (since, therefore, the marker must
      // be as well)
      ObjectID                    _dockObjectID;
      const Vision::KnownMarker*  _dockMarker;
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryingMarker;
      
      // Object to keep head tracked to this object whenever it is observed
      ObjectID                    _trackHeadToObjectID;
      
      /*
      // Plan a path to the pre-ascent/descent pose (depending on current
      // height of the robot) and then go up or down the ramp.
      Result ExecuteRampingSequence(Ramp* ramp);
      
      // Plan a path to the nearest (?) pre-crossing pose of the specified bridge
      // object, then cross it.
      Result ExecuteBridgeCrossingSequence(ActionableObject* object);
       */
      
      // A place to store reaction callback functions, indexed by the type of
      // vision marker that triggers them
      std::map<Vision::Marker::Code, std::list<ReactionCallback> > _reactionCallbacks;
      
      // Flag for saving next processed image to file
      bool _saveNextImageToFile;
      
      ///////// Modifiers ////////
      
      void SetCurrPathSegment(const s8 s)     {_currPathSegment = s;}
      void SetNumFreeSegmentSlots(const u8 n) {_numFreeSegmentSlots = n;}
      void SetLastRecvdPathID(u16 path_id)    {_lastRecvdPathID = path_id;}
      void SetPickingOrPlacing(bool t)        {_isPickingOrPlacing = t;}
      void SetPickedUp(bool t);
      void SetProxSensorData(const ProxSensor_t sensor, u8 value, bool blocked) {_proxVals[sensor] = value; _proxBlocked[sensor] = blocked;}

      ///////// Animation /////////
      
      CannedAnimationContainer _cannedAnimations;
      
      
      ///////// Messaging ////////
      // These methods actually do the creation of messages and sending
      // (via MessageHandler) to the physical robot
      
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
      Result SendExecutePath(const Planning::Path& path, const bool useManualSpeed) const;
      
      // Turn on/off headlight LEDs
      Result SendHeadlight(u8 intensity);
      
      // Sync time with physical robot and trigger it robot to send back camera
      // calibration
      Result SendSyncTime() const;
      
      // Send's robot's current pose
      Result SendAbsLocalizationUpdate() const;
      
      // Update the head angle on the robot
      Result SendHeadAngleUpdate() const;
      
      // Request camera snapshot from robot
      Result SendImageRequest(const ImageSendMode_t mode,
                              const Vision::CameraResolution resolution) const;
      
      // Request imu log from robot
      Result SendIMURequest(const u32 length_ms) const;
      
      // Run a test mode
      Result SendStartTestMode(const TestMode mode, s32 p1, s32 p2, s32 p3) const;
      
      Result SendPlaceObjectOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed);
      
      // Play animation
      // If numLoops == 0, animation repeats forever.
      Result SendPlayAnimation(const char* animName, const u32 numLoops = 0);
      
      Result SendTransitionToStateAnimation(const char *transitionAnimName,
                                            const char *stateAnimName);
      
      Result SendDockWithObject(const Vision::KnownMarker* marker,
                                const Vision::KnownMarker* marker2,
                                const DockAction_t dockAction,
                                const u16 image_pixel_x,
                                const u16 image_pixel_y,
                                const u8 pixel_radius,
                                const bool useManualSpeed);
      
      Result SendStartFaceTracking(const u8 timeout_sec);
      Result SendStopFaceTracking();
      Result SendPing();
      
      Result SendAbortDocking();
      Result SendAbortAnimation();

      
      // =========  Block messages  ============
      Result SendFlashBlockIDs();
      Result SendSetBlockLights(const u8 blockID, const u32 color, const u32 onPeriod_ms, const u32 offPeriod_ms);
      void ActiveBlockLightTest(const u8 blockID);  // For testing
      
      
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
    
    inline void Robot::SetLocalizedTo(const ObjectID& toID)
    { _localizedToID = toID;}
    
    inline void Robot::SetCameraCalibration(const Vision::CameraCalibration& calib)
    {
      _cameraCalibration = calib;
      _camera.SetSharedCalibration(&_cameraCalibration);
      
#if ASYNC_VISION_PROCESSING
      // Now that we have camera calibration, we can start the vision
      // processing thread
      _visionProcessor.Start(_cameraCalibration);
#else
      _visionProcessor.SetCameraCalibration(_cameraCalibration);
#endif
    }

    inline const Vision::CameraCalibration& Robot::GetCameraCalibration() const
    { return _cameraCalibration; }
    
    inline const f32 Robot::GetHeadAngle() const
    { return _currentHeadAngle; }
    
    inline const f32 Robot::GetLiftAngle() const
    { return _currentLiftAngle; }
    
    inline void Robot::SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction) {
      _rampID = rampID;
      _rampDirection = direction;
    }

    inline void Robot::SetCarryingObject(ObjectID carryObjectID) {
      _carryingObjectID = carryObjectID;
    }
    
    inline void Robot::UnSetCarryingObject() {
      _carryingObjectID.UnSet();
    }

    inline Result Robot::SetDockObjectAsAttachedToLift(){
      return SetObjectAsAttachedToLift(_dockObjectID, _dockMarker);
    }
    
    inline bool Robot::IsAnimating() const {
      return _isAnimating;
    }
    
    
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOT_H
