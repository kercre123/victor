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

#include "anki/common/types.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/planning/shared/path.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/ledTypes.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/actionContainers.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/imageDeChunker.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "util/signals/simpleSignal.hpp"
#include "clad/types/robotStatusAndActions.h"
#include "clad/types/imageTypes.h"
#include <queue>
#include <unordered_map>
#include <time.h>
#include <utility>

namespace Anki {
  
// Forward declaration:
namespace Util {
namespace Data {
class DataPlatform;
}
}

enum class ERobotDriveToPoseStatus {
  // There was an internal error while planning
  Error,

  // computing the inital path (the robot is not moving)
  ComputingPath,

  // replanning based on an environment change. The robot is likely following the old path while this is
  // happening
  Replanning,

  // Following a planned path
  FollowingPath,

  // Stopped and waiting (not planning or following)
  Waiting,
};
  
namespace Cozmo {
  
// Forward declarations:
class IPathPlanner;
class MatPiece;
class PathDolerOuter;
class RobotPoseHistory;
class RobotPoseStamp;
class IExternalInterface;
struct RobotState;
class ActiveCube;

typedef enum {
  SAVE_OFF = 0,
  SAVE_ONE_SHOT,
  SAVE_CONTINUOUS
} SaveMode_t;
    
namespace RobotInterface {
  class MessageHandler;
  class EngineToRobot;
  class RobotToEngine;
  enum class EngineToRobotTag : uint8_t;
  enum class RobotToEngineTag : uint8_t;
} // end namespace RobotInterface

namespace ExternalInterface {
  class MessageEngineToGame;
}

// indent 2 spaces << that way !!!! coding standards !!!!
class Robot
{
public:
    
    Robot(const RobotID_t robotID, RobotInterface::MessageHandler* msgHandler,
          IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform);
    ~Robot();
    
    Result Update();
    
    Result UpdateFullRobotState(const RobotState& msg);
    
    bool HasReceivedRobotState() const;
    
    // Accessors
    const RobotID_t        GetID()         const;
    BlockWorld&            GetBlockWorld()       {return _blockWorld;}
    const BlockWorld&      GetBlockWorld() const {return _blockWorld;}
    
    FaceWorld&             GetFaceWorld()        {return _faceWorld;}
    const FaceWorld&       GetFaceWorld()  const {return _faceWorld;}
    
    //
    // Localization
    //
    bool                   IsLocalized()     const;
    void                   Delocalize();
      
    // Get the ID of the object we are localized to
    const ObjectID&        GetLocalizedTo()  const {return _localizedToID;}
      
    // Set the object we are localized to.
    // Use nullptr to UnSet the localizedTo object but still mark the robot
    // as localized (i.e. to "odometry").
    Result                 SetLocalizedTo(const ObservableObject* object);
  
    // Has the robot moved since it was last localized
    bool                   HasMovedSinceBeingLocalized() const;
  
    // Get the squared distance to the closest, most recently observed marker
    // on the object we are localized to
    f32                    GetLocalizedToDistanceSq() const;
  
    // TODO: Can this be removed in favor of the more general LocalizeToObject() below?
    Result LocalizeToMat(const MatPiece* matSeen, MatPiece* existinMatPiece);
      
    Result LocalizeToObject(const ObservableObject* seenObject,
                            ObservableObject* existingObject);
    
    // Returns true if robot is not traversing a path and has no actions in its queue.
    bool   IsIdle() const { return !IsTraversingPath() && _actionList.IsEmpty(); }
    
    // True if wheel speeds are non-zero in most recent RobotState message
    bool   IsMoving() const {return _isMoving;}
    
    // True if head/lift is on its way to a commanded angle/height
    bool   IsHeadMoving() const {return _isHeadMoving;}
    bool   IsLiftMoving() const {return _isLiftMoving;}
    
    // True if we are on the sloped part of a ramp
    bool   IsOnRamp() const { return _onRamp; }
    
    // Set whether or not the robot is on a ramp
    Result SetOnRamp(bool t);
    
    // Just sets the ramp to use and in which direction, not whether robot is on it yet
    void   SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction);
    
    //
    // Camera / Vision
    //
    void                              EnableVisionWhileMoving(bool enable);
    const Vision::Camera&             GetCamera() const;
    Vision::Camera&                   GetCamera();
    void                              SetCameraCalibration(const Vision::CameraCalibration& calib);
    const Vision::CameraCalibration&  GetCameraCalibration() const;
    Vision::Camera                    GetHistoricalCamera(RobotPoseStamp* p, TimeStamp_t t);
    
    // Queue an observed vision marker for processing with BlockWorld.
    // Note that this is a NON-const reference: the marker's camera will be updated
    // to use a pose from pose history.
    Result QueueObservedMarker(const Vision::ObservedMarker& marker);
    
    // Get a *copy* of the current image on this robot's vision processing thread
    // TODO: Remove this method? I don't think anyone is using it...
    bool GetCurrentImage(Vision::Image& img, TimeStamp_t newerThan);
    
    // Returns the average period of incoming robot images
    u32 GetAverageImagePeriodMS();
    
    // Specify whether this robot is a physical robot or not.
    // Currently, adjusts headCamPose by slop factor if it's physical.
    void SetPhysicalRobot(bool isPhysical);
    bool IsPhysical() {return _isPhysical;}
    
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
    
    // Get 3D bounding box of the robot at its current pose or a given pose
    void GetBoundingBox(std::array<Point3f, 8>& bbox3d, const Point3f& padding_mm) const;
    void GetBoundingBox(const Pose3d& atPose, std::array<Point3f, 8>& bbox3d, const Point3f& padding_mm) const;

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
    
    // Return pose of robot's drive center based on what it's currently carrying
    const Pose3d& GetDriveCenterPose() const;
    
    // Computes the drive center offset from origin based on current carrying state
    f32 GetDriveCenterOffset() const;
    
    // Computes pose of drive center for the given robot pose
    void ComputeDriveCenterPose(const Pose3d &robotPose, Pose3d &driveCenterPose) const;
    
    // Computes robot origin pose for the given drive center pose
    void ComputeOriginPose(const Pose3d &driveCenterPose, Pose3d &robotPose) const;
    
    //
    // Path Following
    //

    // Begin computation of a path to drive to the given pose (or poses). Once the path is computed, the robot
    // will immediately start following it, and will replan (e.g. to avoid new obstacles) automatically If
    // useManualSpeed is set to true, the robot will plan a path to the goal, but won't actually execute any
    // speed changes, so the user (or some other system) will have control of the speed along the "rails" of
    // the path. If specified, the maxReplanTime arguments specifies the maximum nyum
    Result StartDrivingToPose(const Pose3d& pose, bool useManualSpeed = false);

    // Just like above, but will plan to any of the given poses. It's up to the robot / planner to pick which
    // pose it wants to go to. The optional second argument is a pointer to a size_t, which, if not null, will
    // be set to the pose which is selected once planning is complete
    Result StartDrivingToPose(const std::vector<Pose3d>& poses,
                              size_t* selectedPoseIndex = nullptr,
                              bool useManualSpeed = false);
  
    // This function checks the planning / path following status of the robot. See the enum definition for
    // details
    ERobotDriveToPoseStatus CheckDriveToPoseStatus() const;
    
    bool IsTraversingPath()   const {return (_currPathSegment >= 0) || (_lastSentPathID > _lastRecvdPathID);}
    
    u16  GetCurrentPathSegment() const { return _currPathSegment; }
    u16  GetLastRecvdPathID()    const { return _lastRecvdPathID; }
    u16  GetLastSentPathID()     const { return _lastSentPathID;  }

    bool IsUsingManualPathSpeed() const {return _usingManualPathSpeed;}
    
    //
    // Object Docking / Carrying
    //

    const ObjectID&  GetDockObject()          const {return _dockObjectID;}
    const ObjectID&  GetCarryingObject()      const {return _carryingObjectID;}
    const ObjectID&  GetCarryingObjectOnTop() const {return _carryingObjectOnTopID;}
    const std::set<ObjectID> GetCarryingObjects() const;
    const Vision::KnownMarker*  GetCarryingMarker() const {return _carryingMarker; }

    bool IsCarryingObject()   const {return _carryingObjectID.IsSet(); }
    bool IsPickingOrPlacing() const {return _isPickingOrPlacing;}
    bool IsPickedUp()         const {return _isPickedUp;}
    
    void SetCarryingObject(ObjectID carryObjectID);
    void UnSetCarryingObjects();
    
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
                              const DockAction dockAction,
                              const u16 image_pixel_x,
                              const u16 image_pixel_y,
                              const u8 pixel_radius,
                              const f32 placementOffsetX_mm = 0,
                              const f32 placementOffsetY_mm = 0,
                              const f32 placementOffsetAngle_rad = 0,
                              const bool useManualSpeed = false);
    
    // Same as above but without specifying image location for marker
    Result DockWithObject(const ObjectID objectID,
                          const Vision::KnownMarker* marker,
                          const Vision::KnownMarker* marker2,
                          const DockAction dockAction,
                          const f32 placementOffsetX_mm = 0,
                          const f32 placementOffsetY_mm = 0,
                          const f32 placementOffsetAngle_rad = 0,
                          const bool useManualSpeed = false);
    
    // Transitions the object that robot was docking with to the one that it
    // is carrying, and puts it in the robot's pose chain, attached to the
    // lift. Returns RESULT_FAIL if the robot wasn't already docking with
    // an object.
    Result SetDockObjectAsAttachedToLift();
    
    // Same as above, but with specified object
    Result SetObjectAsAttachedToLift(const ObjectID& dockObjectID,
                                     const Vision::KnownMarker* dockMarker);
    
    void SetLastPickOrPlaceSucceeded(bool tf) { _lastPickOrPlaceSucceeded = tf; }
    bool GetLastPickOrPlaceSucceeded() const { return _lastPickOrPlaceSucceeded; }
    
    // Places the object that the robot was carrying in its current position
    // w.r.t. the world, and removes it from the lift pose chain so it is no
    // longer "attached" to the robot.
    Result SetCarriedObjectAsUnattached();
    
    Result StopDocking();
    
    /*
    //
    // Proximity Sensors
    //
    u8   GetProxSensorVal(ProxSensor_t sensor)    const {return _proxVals[sensor];}
    bool IsProxSensorBlocked(ProxSensor_t sensor) const {return _proxBlocked[sensor];}

    // Pose of where objects are assumed to be with respect to robot pose when
    // obstacles are detected by proximity sensors
    static const Pose3d ProxDetectTransform[NUM_PROX];
    */


    //
    // Vision
    //
    Result ProcessImage(const Vision::ImageRGB& image);
    Result StartLookingForMarkers();
    Result StopLookingForMarkers();
    void SetupVisionHandlers(IExternalInterface& externalInterface);

    // Set how to save incoming robot state messages
    void SetSaveStateMode(const SaveMode_t mode);
    
    // Set how to save incoming robot images to file
    void SetSaveImageMode(const SaveMode_t mode);
    
    // Return the timestamp of the last _processed_ image
    TimeStamp_t GetLastImageTimeStamp() { return _visionProcessor.GetLastProcessedImageTimeStamp(); }
    
    // =========== Actions Commands =============
    
    // Return a reference to the robot's action list for directly adding things
    // to do, either "now" or in queues.
    // TODO: This seems simpler than writing/maintaining wrappers, but maybe that would be better?
    ActionList& GetActionList() { return _actionList; }
    
    // Send a message to the robot to place whatever it is carrying on the
    // ground right where it is. Returns RESULT_FAIL if robot is not carrying
    // anything.
    Result PlaceObjectOnGround(const bool useManualSpeed = false);
    
    
    // =========== Animation Commands =============
    
    // Plays specified animation numLoops times.
    // If numLoops == 0, animation repeats forever.
    // Returns the streaming tag, so you can find out when it is done.
    u8 PlayAnimation(const std::string& animName, const u32 numLoops = 1);
    
    // Set the animation to be played when no other animation has been specified.
    // Use the empty string to disable idle animation.
    Result SetIdleAnimation(const std::string& animName);
    
    // Returns name of currently streaming animation. Does not include idle animation.
    // Returns "" if no non-idle animation is streaming.
    const std::string GetStreamingAnimationName() const;
    
    const ProceduralFace& GetProceduralFace() const { return _proceduralFace; }
    const ProceduralFace& GetLastProceduralFace() const { return _lastProceduralFace; }
    void SetProceduralFace(const ProceduralFace& face);
    void MarkProceduralFaceAsSent();
    
    // Returns the number of animation bytes played on the robot since
    // it was initialized with SyncTime.
    s32 GetNumAnimationBytesPlayed() const;
    
    // Returns a reference to a count of the total number of bytes streamed to the robot.
    s32 GetNumAnimationBytesStreamed();
    void IncrementNumAnimationBytesStreamed(s32 num);
    
    // Ask the UI to play a sound for us
    Result PlaySound(const std::string& soundName, u8 numLoops, u8 volume);
    void   StopSound();
    
    Result StopAnimation();

    // Read the animations in a dir
    void ReadAnimationFile(const char* filename, std::string& animationID);

    // Read the animations in a dir
    void ReadAnimationDir();

    // Returns true if the robot is currently playing an animation, according
    // to most recent state message. NOTE: Will also be true if the animation
    // is the "idle" animation!
    bool IsAnimating() const;
    
    // Returns true iff the robot is currently playing the idle animation.
    bool IsIdleAnimating() const;
    
    // Returns the "tag" of the 
    u8 GetCurrentAnimationTag() const;

    Result SyncTime();
    
    Result RequestIMU(const u32 length_ms) const;

    // For debugging robot parameters:
    void SetupGainsHandlers(IExternalInterface& externalInterface);

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
    
    // Color specified as RGBA, where A(lpha) will be ignored
    void SetDefaultLights(const u32 color);
    
    void SetBackpackLights(const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& onColor,
                           const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& offColor,
                           const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& onPeriod_ms,
                           const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& offPeriod_ms,
                           const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& transitionOnPeriod_ms,
                           const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& transitionOffPeriod_ms);
   
    
    // =========  Block messages  ============
  
    // Dynamically cast the given object ID into the templated active object type
    // Return nullptr on failure to find ActiveObject
    ObservableObject* GetActiveObject(const ObjectID objectID,
                                      const ObjectFamily inFamily = ObjectFamily::Unknown);
  
    // Same as above, but search by active ID instead of (BlockWorld-assigned) object ID.
    ObservableObject* GetActiveObjectByActiveID(const s32 activeID,
                                                const ObjectFamily inFamily = ObjectFamily::Unknown);
  
    // Set the LED colors/flashrates individually (ordered by BlockLEDPosition)
    Result SetObjectLights(const ObjectID& objectID,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& onColor,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& offColor,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& onPeriod_ms,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& offPeriod_ms,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& transitionOnPeriod_ms,
                           const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& transitionOffPeriod_ms,
                           const MakeRelativeMode makeRelative,
                           const Point2f& relativeToPoint);
    
    // Set all LEDs of the specified block to the same color/flashrate
    Result SetObjectLights(const ObjectID& objectID,
                           const WhichCubeLEDs whichLEDs,
                           const u32 onColor, const u32 offColor,
                           const u32 onPeriod_ms, const u32 offPeriod_ms,
                           const u32 transitionOnPeriod_ms, const u32 transitionOffPeriod_ms,
                           const bool turnOffUnspecifiedLEDs,
                           const MakeRelativeMode makeRelative,
                           const Point2f& relativeToPoint);
    
    // Shorthand for turning off all lights on an object
    Result TurnOffObjectLights(const ObjectID& objectID);
    
    // =========  Other State  ============
    f32 GetBatteryVoltage() const { return _battVoltage; }
    
    // Abort everything the robot is doing, including path following, actions,
    // animations, and docking. This is like the big red E-stop button.
    // TODO: Probably need a more elegant way of doing this.
    Result AbortAll();
    
    // Abort things individually
    Result AbortAnimation();
    Result AbortDocking(); // a.k.a. PickAndPlace
    Result AbortDrivingToPose(); // stops planning and path following
  
    // Helper template for sending Robot messages with clean syntax
    template<typename T, typename... Args>
    Result SendRobotMessage(Args&&... args) const
    {
      return SendMessage(RobotInterface::EngineToRobot(T(std::forward<Args>(args)...)));
    }
  
    // Send a message to the physical robot
    Result SendMessage(const RobotInterface::EngineToRobot& message,
                       bool reliable = true, bool hot = false) const;
    
    // =========  Events  ============
    using RobotWorldOriginChangedSignal = Signal::Signal<void (RobotID_t)>;
    RobotWorldOriginChangedSignal& OnRobotWorldOriginChanged() { return _robotWorldOriginChangedSignal; }
    inline bool HasExternalInterface() const { return _externalInterface != nullptr; }
    inline IExternalInterface* GetExternalInterface() {
      ASSERT_NAMED(_externalInterface != nullptr, "Robot.ExternalInterface.nullptr"); return _externalInterface; }
    inline void SetImageSendMode(ImageSendMode newMode) { _imageSendMode = newMode; }
    inline const ImageSendMode GetImageSendMode() const { return _imageSendMode; }
  
    inline MovementComponent& GetMoveComponent() { return _movementComponent; }
    inline const MovementComponent& GetMoveComponent() const { return _movementComponent; }

    inline const MoodManager& GetMoodManager() const { return _moodManager; }
    inline MoodManager& GetMoodManager() { return _moodManager; }
  
    // Handle various message types
    template<typename T>
    void HandleMessage(const T& msg);

    // Convenience wrapper for broadcasting an event if the robot has an ExternalInterface.
    // Does nothing if not. Returns true if event was broadcast, false if not (i.e.
    // if there was no external interface).
    bool Broadcast(ExternalInterface::MessageEngineToGame&& event);
  
  protected:
    IExternalInterface* _externalInterface;
    Util::Data::DataPlatform* _dataPlatform;
    RobotWorldOriginChangedSignal _robotWorldOriginChangedSignal;
    // The robot's identifier
    RobotID_t         _ID;
    bool              _isPhysical = false;
    
    // Flag indicating whether a robotStateMessage was ever received
    bool              _newStateMsgAvailable = false;
    
    // A reference to the MessageHandler that the robot uses for outgoing comms
    RobotInterface::MessageHandler* _msgHandler;
    
    // A reference to the BlockWorld the robot lives in
    BlockWorld        _blockWorld;
    
    // A container for faces/people the robot knows about
    FaceWorld         _faceWorld;
    
    VisionProcessingThread _visionProcessor;
    //Vision::PanTiltTracker _faceTracker;
  
    BehaviorManager  _behaviorMgr;
    bool             _isBehaviorMgrEnabled = false;
    
    //ActionQueue      _actionQueue;
    ActionList       _actionList;
    MovementComponent _movementComponent;

    // Path Following. There are two planners, only one of which can
    // be selected at a time
    IPathPlanner*            _selectedPathPlanner          = nullptr;
    IPathPlanner*            _longPathPlanner              = nullptr;
    IPathPlanner*            _shortPathPlanner             = nullptr;
    size_t*                  _plannerSelectedPoseIndexPtr  = nullptr;
    int                      _numPlansStarted              = 0;
    int                      _numPlansFinished             = 0;
    ERobotDriveToPoseStatus  _driveToPoseStatus            = ERobotDriveToPoseStatus::Waiting;
    s8                       _currPathSegment              = -1;
    u8                       _numFreeSegmentSlots          = 0;
    u16                      _lastSentPathID               = 0;
    u16                      _lastRecvdPathID              = 0;
    bool                     _usingManualPathSpeed         = false;
    PathDolerOuter*          _pdo                          = nullptr;
    
    // This functions sets _selectedPathPlanner to the appropriate
    // planner
    void SelectPlanner(const Pose3d& targetPose);

    // Sends a path to the robot to be immediately executed
    Result ExecutePath(const Planning::Path& path, const bool useManualSpeed = false);
    
    // Robot stores the calibration, camera just gets a reference to it
    // This is so we can share the same calibration data across multiple
    // cameras (e.g. those stored inside the pose history)
    Vision::CameraCalibration _cameraCalibration;
    Vision::Camera            _camera;
    bool                      _visionWhileMovingEnabled = false;

    /*
    // Proximity sensors
    std::array<u8,   NUM_PROX>  _proxVals;
    std::array<bool, NUM_PROX>  _proxBlocked;
    */

    // Geometry / Pose
    std::list<Pose3d> _poseOrigins; // placeholder origin poses while robot isn't localized
    Pose3d*           _worldOrigin;
    Pose3d            _pose;
    Pose3d            _driveCenterPose;
    PoseFrameID_t     _frameId = 0;
    ObjectID          _localizedToID;       // ID of mat object robot is localized to
    bool              _hasMovedSinceLocalization = false;
    bool              _isLocalized = true;  // May be true even if not localized to an object, if robot has not been picked up
    bool              _localizedToFixedObject; // false until robot sees a _fixed_ mat
    f32               _localizedMarkerDistToCameraSq; // Stores (squared) distance to the closest observed marker of the object we're localized to

    
    Result UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin);
    
    const Pose3d     _neckPose;     // joint around which head rotates
    Pose3d           _headCamPose;  // in canonical (untilted) position w.r.t. neck joint
    const Pose3d     _liftBasePose; // around which the base rotates/lifts
    Pose3d           _liftPose;     // current, w.r.t. liftBasePose

    f32              _currentHeadAngle;
    f32              _currentLiftAngle = 0;
    
    f32              _leftWheelSpeed_mmps;
    f32              _rightWheelSpeed_mmps;
    
    // Ramping
    bool             _onRamp = false;
    ObjectID         _rampID;
    Point2f          _rampStartPosition;
    f32              _rampStartHeight;
    Ramp::TraversalDirection _rampDirection;
    
    // State
    bool             _isPickingOrPlacing = false;
    bool             _isPickedUp         = false;
    bool             _isMoving           = false;
    bool             _isHeadMoving       = false;
    bool             _isLiftMoving       = false;
    f32              _battVoltage        = 5;
    ImageSendMode    _imageSendMode      = ImageSendMode::Off;
  
    // Pose history
    Result ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                           TimeStamp_t& t, RobotPoseStamp** p,
                                           HistPoseKey* key = nullptr,
                                           bool withInterpolation = false);
    
    Result GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p);
    Result GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key = nullptr);
    Result GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose);
    
    RobotPoseHistory* _poseHistory;
    
    // Takes startPose and moves it forward as if it were a robot pose by distance mm and
    // puts result in movedPose.
    static void MoveRobotPoseForward(const Pose3d &startPose, const f32 distance, Pose3d &movedPose);
    
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
    const Vision::KnownMarker*  _dockMarker               = nullptr;
    ObjectID                    _carryingObjectID;
    ObjectID                    _carryingObjectOnTopID;
    const Vision::KnownMarker*  _carryingMarker           = nullptr;
    bool                        _lastPickOrPlaceSucceeded = false;
  
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
    
    // Save mode for robot state
    SaveMode_t _stateSaveMode = SAVE_OFF;
    
    // Save mode for robot images
    SaveMode_t _imageSaveMode = SAVE_OFF;
    
    // Maintains an average period of incoming robot images
    u32         _imgFramePeriod        = 0;
    TimeStamp_t _lastImgTimeStamp      = 0;
    std::string _lastPlayedAnimationId;

    std::unordered_map<std::string, time_t> _loadedAnimationFiles;
    
    ///////// Modifiers ////////
    
    void SetCurrPathSegment(const s8 s)     {_currPathSegment = s;}
    void SetNumFreeSegmentSlots(const u8 n) {_numFreeSegmentSlots = n;}
    void SetLastRecvdPathID(u16 path_id)    {_lastRecvdPathID = path_id;}
    void SetPickingOrPlacing(bool t)        {_isPickingOrPlacing = t;}
    void SetPickedUp(bool t);
    /*
    void SetProxSensorData(const ProxSensor_t sensor, u8 value, bool blocked) {_proxVals[sensor] = value; _proxBlocked[sensor] = blocked;}
    */

    ///////// Animation /////////
    
    CannedAnimationContainer _cannedAnimations;
    AnimationStreamer        _animationStreamer;
    ProceduralFace           _proceduralFace, _lastProceduralFace;
    s32 _numFreeAnimationBytes;
    s32 _numAnimationBytesPlayed   = 0;
    s32 _numAnimationBytesStreamed = 0;
    u8  _animationTag              = 0;
    
    ///////// Mood/Emotions ////////
    MoodManager      _moodManager;
    
    ///////// Messaging ////////
    // These methods actually do the creation of messages and sending
    // (via MessageHandler) to the physical robot
    std::vector<Signal::SmartHandle> _signalHandles;
    ImageDeChunker* _imageDeChunker;
    uint8_t _imuSeqID = 0;
    uint32_t _imuDataSize = 0;
    int8_t _imuData[6][1024]{{0}};  // first ax, ay, az, gx, gy, gz


  void InitRobotMessageComponent(RobotInterface::MessageHandler* messageHandler, RobotID_t robotId);
    void HandleCameraCalibration(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandlePrint(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleBlockPickedUp(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleBlockPlaced(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleGoalPose(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleCliffEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    void HandleChargerEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    // For processing image chunks arriving from robot.
    // Sends complete images to VizManager for visualization (and possible saving).
    void HandleImageChunk(const AnkiEvent<RobotInterface::RobotToEngine>& message);
    // For processing imu data chunks arriving from robot.
    // Writes the entire log of 3-axis accelerometer and 3-axis
    // gyro readings to a .m file in kP_IMU_LOGS_DIR so they
    // can be read in from Matlab. (See robot/util/imuLogsTool.m)
    void HandleImuData(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  
    void SetupMiscHandlers(IExternalInterface& externalInterface);

    Result SendAbsLocalizationUpdate(const Pose3d&        pose,
                                     const TimeStamp_t&   t,
                                     const PoseFrameID_t& frameId) const;

    Result ClearPath();

    // Clears the path that the robot is executing which also stops the robot
    Result SendClearPath() const;

    // Removes the specified number of segments from the front and back of the path
    Result SendTrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments) const;
    
    // Sends a path to the robot to be immediately executed
    Result SendExecutePath(const Planning::Path& path, const bool useManualSpeed) const;
    
    // Sync time with physical robot and trigger it robot to send back camera
    // calibration
    Result SendSyncTime() const;
    
    // Send's robot's current pose
    Result SendAbsLocalizationUpdate() const;
    
    // Update the head angle on the robot
    Result SendHeadAngleUpdate() const;

    // Request imu log from robot
    Result SendIMURequest(const u32 length_ms) const;

    Result SendAbortDocking();
    Result SendAbortAnimation();
    
    Result SendSetCarryState(CarryState state);

    
    // =========  Active Object messages  ============
    Result SendFlashObjectIDs();
    Result SendSetObjectLights(const ActiveCube* activeCube);
    Result SendSetObjectLights(const ObjectID& objectID, const u32 onColor, const u32 offColor, const u32 onPeriod_ms, const u32 offPeriod_ms);
    void ActiveObjectLightTest(const ObjectID& objectID);  // For testing
    
    
}; // class Robot

  
//
// Inline accessors:
//
inline const RobotID_t Robot::GetID(void) const
{ return _ID; }

inline const Pose3d& Robot::GetPose(void) const
{ return _pose; }

inline const Pose3d& Robot::GetDriveCenterPose(void) const
{return _driveCenterPose; }

inline void Robot::EnableVisionWhileMoving(bool enable)
{ _visionWhileMovingEnabled = enable; }

inline const Vision::Camera& Robot::GetCamera(void) const
{ return _camera; }

inline Vision::Camera& Robot::GetCamera(void)
{ return _camera; }

inline void Robot::SetCameraCalibration(const Vision::CameraCalibration& calib)
{
  if (_cameraCalibration != calib) {

    _cameraCalibration = calib;
    _camera.SetSharedCalibration(&_cameraCalibration);
    _visionProcessor.SetCameraCalibration(_cameraCalibration);

  } else {
    PRINT_NAMED_INFO("Robot.SetCameraCalibration.IgnoringDuplicateCalib","");
  }
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

inline Result Robot::SetDockObjectAsAttachedToLift(){
  return SetObjectAsAttachedToLift(_dockObjectID, _dockMarker);
}

inline u8 Robot::GetCurrentAnimationTag() const {
  return _animationTag;
}

inline bool Robot::IsAnimating() const {
  return _animationTag != 0;
}

inline bool Robot::IsIdleAnimating() const {
  return _animationTag == 255;
}

inline Result Robot::TurnOffObjectLights(const ObjectID& objectID) {
  return SetObjectLights(objectID, WhichCubeLEDs::ALL, 0, 0, 10000, 10000, 0, 0,
                         false, MakeRelativeMode::RELATIVE_LED_MODE_OFF, {0.f,0.f});
}

inline s32 Robot::GetNumAnimationBytesPlayed() const {
  return _numAnimationBytesPlayed;
}

inline s32 Robot::GetNumAnimationBytesStreamed() {
  return _numAnimationBytesStreamed;
}

inline void Robot::IncrementNumAnimationBytesStreamed(s32 num) {
  _numAnimationBytesStreamed += num;
}

inline f32 Robot::GetLocalizedToDistanceSq() const {
  return _localizedMarkerDistToCameraSq;
}
  
inline bool Robot::HasMovedSinceBeingLocalized() const {
  return _hasMovedSinceLocalization;
}
  
inline bool Robot::IsLocalized() const {
  
  ASSERT_NAMED(_isLocalized || (!_isLocalized && !_localizedToID.IsSet()),
               "Robot can't think it is localized and have localizedToID set!");
  
  return _isLocalized;
}

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOT_H
