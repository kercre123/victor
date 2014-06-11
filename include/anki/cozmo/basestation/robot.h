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
    
    class BlockDockingSystem
    {
    public:
      
    protected:
      
      
    }; // class BlockDockingSystem
    
    
    class Robot
    {
    public:
      enum State {
        IDLE,
        FOLLOWING_PATH,
        BEGIN_DOCKING,
        DOCKING
      };
      
      Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner);
      
      void Update();
      
      const RobotID_t get_ID() const;
      const Pose3d& get_pose() const;
      const Vision::Camera& get_camDown() const;
      const Vision::Camera& get_camHead() const;
      Vision::Camera& get_camHead(void);
      
      const f32 get_headAngle() const;
      const f32 get_liftAngle() const;
      
      const Pose3d& get_neckPose() const {return neckPose;}
      const Pose3d& get_headCamPose() const {return headCamPose;}
      
      void set_pose(const Pose3d &newPose);
      void set_headAngle(const f32& angle);
      void set_liftAngle(const f32& angle);
      void set_camCalibration(const Vision::CameraCalibration& calib);
      
      State GetState() const;
      void  SetState(const State newState);
      
      void IncrementPoseFrameID() {++frameId_;}
      PoseFrameID_t GetPoseFrameID() const {return frameId_;}
      
      void queueIncomingMessage(const u8 *msg, const u8 msgSize);
      bool hasOutgoingMessages() const;
      void getOutgoingMessage(u8 *msgOut, u8 &msgSize);
      
      Result GetPathToPose(const Pose3d& pose, Planning::Path& path);
      Result ExecutePathToPose(const Pose3d& pose);
      Result ExecutePathToPose(const Pose3d& pose, const Radians headAngle);
      
      // Clears the path that the robot is executing which also stops the robot
      Result ClearPath();

      // Removes the specified number of segments from the front and back of the path
      Result TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments);
      
      // Sends a path to the robot to be immediately executed
      Result ExecutePath(const Planning::Path& path);
      
      // True if wheel speeds are non-zero in most recent RobotState message
      bool IsMoving() const {return isMoving_;}
      void SetIsMoving(bool t) {isMoving_ = t;}
      
      void SetCurrPathSegment(const s8 s) {currPathSegment_ = s;}
      s8 GetCurrPathSegment() {return currPathSegment_;}
      bool IsTraversingPath() {return currPathSegment_ >= 0;}

      void SetCarryingBlock(bool t) {isCarryingBlock_ = t;}
      bool IsCarryingBlock() {return isCarryingBlock_;}

      void SetPickingOrPlacing(bool t) {isPickingOrPlacing_ = t;}
      bool IsPickingOrPlacing() {return isPickingOrPlacing_;}
      
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
      
      Result ExecuteDockingSequence(const Block* blockToDockWith);
      
      // Sends a message to the robot to dock with the specified block
      // that it should currently be seeing.
      Result DockWithBlock(const u8 markerType,
                           const f32 markerWidth_mm,
                           const DockAction_t dockAction);
      
      // Sends a message to the robot to dock with the specified block
      // that it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result DockWithBlock(const u8 markerType,
                           const f32 markerWidth_mm,
                           const DockAction_t dockAction,
                           const u16 image_pixel_x,
                           const u16 image_pixel_y,
                           const u8 pixel_radius);

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
      const RobotPoseHistory& GetPoseHistory() {return poseHistory_;}
      
      Result AddRawOdomPoseToHistory(const TimeStamp_t t,
                                     const PoseFrameID_t frameID,
                                     const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                     const f32 pose_angle,
                                     const f32 head_angle);
      
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
      void UpdateCurrPoseFromHistory();
      
    protected:
      // The robot's identifier
      RobotID_t     ID_;
      
      // A reference to the MessageHandler that the robot uses for outgoing comms
      IMessageHandler* msgHandler_;
      
      // A reference to the BlockWorld the robot lives in
      BlockWorld*   world_;
      
      IPathPlanner* pathPlanner_;
      Planning::Path path_;
      
      Pose3d pose;
      void updatePose();
      PoseFrameID_t frameId_;
      
      Vision::Camera camHead;

      const Pose3d neckPose; // joint around which head rotates
      const Pose3d headCamPose; // in canonical (untilted) position w.r.t. neck joint
      const Pose3d liftBasePose; // around which the base rotates/lifts

      f32 currentHeadAngle;
      f32 currentLiftAngle;
      
      s8 currPathSegment_;
      
      Pose3d goalPose_;
      Radians goalHeadAngle_;
      
      State state_, nextState_;
      //bool setOperationMode(OperationMode newMode);
      bool isCarryingBlock_;
      bool isPickingOrPlacing_;
      bool isMoving_;
      
      //std::vector<BlockMarker3d*>  visibleFaces;
      //std::vector<Block*>          visibleBlocks;
      
      // Pose history
      RobotPoseHistory poseHistory_;

      static const Quad2f CanonicalBoundingBoxXY;
      
      // Message handling
      typedef std::vector<u8> MessageType;
      typedef std::queue<MessageType> MessageQueue;
      MessageQueue messagesOut;
      
      // Docking
      const Block* dockBlock_;
      const Vision::KnownMarker *dockMarker_;
      DockAction_t dockAction_;
      
      f32 waitUntilTime_;
      
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
      Result SendDockWithBlock(const u8 markerType,
                               const f32 markerWidth_mm,
                               const DockAction_t dockAction) const;
      
      // Sends a message to the robot to dock with the specified block
      // that it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result SendDockWithBlock(const u8 markerType,
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
    inline const RobotID_t Robot::get_ID(void) const
    { return this->ID_; }
    
    inline const Pose3d& Robot::get_pose(void) const
    { return this->pose; }
    
    inline const Vision::Camera& Robot::get_camHead(void) const
    { return this->camHead; }
    
    inline Vision::Camera& Robot::get_camHead(void)
    { return this->camHead; }
    
    inline void Robot::SetState(const State newState)
    { state_ = newState; }
    
    inline Robot::State Robot::GetState(void) const
    { return state_; }
    
    inline void Robot::set_camCalibration(const Vision::CameraCalibration& calib)
    { this->camHead.SetCalibration(calib); }
    
    inline bool Robot::hasOutgoingMessages() const
    { return not this->messagesOut.empty(); }
    
    inline const f32 Robot::get_headAngle() const
    { return this->currentHeadAngle; }
    
    inline const f32 Robot::get_liftAngle() const
    { return this->currentLiftAngle; }
    
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
      
      IMessageHandler* msgHandler_;
      BlockWorld* blockWorld_;
      IPathPlanner* pathPlanner_;
      
      std::map<RobotID_t,Robot*> robots_;
      std::vector<RobotID_t>     ids_;
      
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
