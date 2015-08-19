/**
 * File: visionSystem.h [Basestation]
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: High-level module that controls the basestation vision system
 *              Runs on its own thread inside VisionProcessingThread.
 *
 *  NOTE: Current implementation is basically a copy of the Embedded vision system
 *    on the robot, so we can first see if vision-over-WiFi is feasible before a
 *    native Basestation implementation of everything.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_BASESTATION_VISIONSYSTEM_H
#define ANKI_COZMO_BASESTATION_VISIONSYSTEM_H

#if ANKICORETECH_USE_MATLAB
   // You can manually adjust this one
#  define ANKI_COZMO_USE_MATLAB_VISION 0
#else
   // Leave this one always set to 0
#  define ANKI_COZMO_USE_MATLAB_VISION 0
#endif


#include "anki/common/types.h"

#include "anki/common/basestation/mailbox.h"

// Robot includes should eventually go away once Basestation vision is natively
// implemented
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/geometry_declarations.h"

#include "anki/common/basestation/matlabInterface.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/cozmo/basestation/comms/robot/robotMessages.h"

#include "anki/vision/basestation/cameraCalibration.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/faceTracker.h"

#include "visionParameters.h"


namespace Anki {
namespace Cozmo {
    
  // Forward declaration:
  class Robot;
  namespace Data {
    class DataPlatform;
  }

  class VisionSystem
  {
  public:

    VisionSystem(Data::DataPlatform* dataPlatform);
    ~VisionSystem();
    
    enum Mode {
      IDLE                 = 0x00,
      LOOKING_FOR_MARKERS  = 0x01,
      TRACKING             = 0x02,
      DETECTING_FACES      = 0x04,
      TAKING_SNAPSHOT      = 0x08,
      LOOKING_FOR_SALIENCY = 0x10
    };
    
    //
    // Methods:
    //
    
    Result Init(const Vision::CameraCalibration& camCalib);
    bool IsInitialized() const;
    
    void StartMarkerDetection();
    void StopMarkerDetection();
    
    // Accessors
    //const HAL::CameraInfo* GetCameraCalibration();
    f32 GetTrackingMarkerWidth();
    
    // This is main Update() call to be called in a loop from above.
    //
    // NOTE: It is important the passed-in robot state message be passed by
    //   value and NOT by reference, since the vision system can be interrupted
    //   by main execution (which updates the state).
    Result Update(const MessageRobotState robotState,
                  const Vision::Image&    inputImg);
    
    void StopTracking();

    // Select a block type to look for to dock with.
    // Use MARKER_UNKNOWN to disable.
    // Next time the vision system sees a block of this type while looking
    // for blocks, it will initialize a template tracker and switch to
    // docking mode.
    // If checkAngleX is true, then tracking will be considered as a failure if
    // the X angle is greater than TrackerParameters::MAX_BLOCK_DOCKING_ANGLE.
    Result SetMarkerToTrack(const Vision::MarkerType&  markerToTrack,
                            const f32                  markerWidth_mm,
                            const bool                 checkAngleX);
    
    // Same as above, except the robot will only start tracking the marker
    // if its observed centroid is within the specified radius (in pixels)
    // from the given image point.
    Result SetMarkerToTrack(const Vision::MarkerType&  markerToTrack,
                            const f32                  markerWidth_mm,
                            const Embedded::Point2f&   imageCenter,
                            const f32                  radius,
                            const bool                 checkAngleX);
    
    u32 DownsampleHelper(const Embedded::Array<u8>& imageIn,
                         Embedded::Array<u8>&       imageOut,
                         Embedded::MemoryStack      scratch);
    
    
    // Returns a const reference to the list of the most recently observed
    // vision markers.
    const Embedded::FixedLengthList<Embedded::VisionMarker>& GetObservedMarkerList();
    
    // Return a const pointer to the largest (in terms of image size)
    // VisionMarker of the specified type.  Pointer is NULL if there is none.
    const Embedded::VisionMarker* GetLargestVisionMarker(const Vision::MarkerType withType);
    
    // Compute the 3D pose of a VisionMarker w.r.t. the camera.
    // - If the pose w.r.t. the robot is required, follow this with a call to
    //    the GetWithRespectToRobot() method below.
    // - If ignoreOrientation=true, the orientation of the marker within the
    //    image plane will be ignored (by sorting the marker's corners such
    //    that they always represent an upright marker).
    // NOTE: rotation should already be allocated as a 3x3 array.
    Result GetVisionMarkerPose(const Embedded::VisionMarker& marker,
                               const bool                    ignoreOrientation,
                               Embedded::Array<f32>&         rotationWrtCamera,
                               Embedded::Point3<f32>&        translationWrtCamera);
    
    // Find the VisionMarker with the specified type whose 3D pose is closest
    // to the given 3D position (with respect to *robot*) and also within the
    // specified maxDistance (in mm).  If such a marker is found, the pose is
    // returned and the "markerFound" flag will be true.
    // NOTE: rotation should already be allocated as a 3x3 array.
    Result GetVisionMarkerPoseNearestTo(const Embedded::Point3<f32>&  atPosition,
                                        const Vision::MarkerType&     withType,
                                        const f32                     maxDistance_mm,
                                        Embedded::Array<f32>&         rotationWrtRobot,
                                        Embedded::Point3<f32>&        translationWrtRobot,
                                        bool&                         markerFound);
    
    // Convert a point or pose in camera coordinates to robot coordinates,
    // using the kinematic chain of the neck and head geometry.
    // NOTE: the rotation matrices should already be allocated as 3x3 arrays.
    Result GetWithRespectToRobot(const Embedded::Point3<f32>& pointWrtCamera,
                                 Embedded::Point3<f32>&       pointWrtRobot);
    
    Result GetWithRespectToRobot(const Embedded::Array<f32>&  rotationWrtCamera,
                                 const Embedded::Point3<f32>& translationWrtCamera,
                                 Embedded::Array<f32>&        rotationWrtRobot,
                                 Embedded::Point3<f32>&       translationWrtRobot);
    
    // Tell the vision system to grab a snapshot on the next call to Update(),
    // within the specified Region of Interest (roi), subsampled by the
    // given amount (e.g. every subsample=4 pixels) and put it in the given
    // snapshot array.  readyFlag will be set to true once this is done.
    // Calling this multiple times before the snapshot is ready does nothing
    // extra (internally, the system already knows it is waiting on a snapshot).
    Result TakeSnapshot(const Embedded::Rectangle<s32> roi, const s32 subsample,
                        Embedded::Array<u8>& snapshot, bool& readyFlag);

    
    // Tell the vision system to switch to face-detection mode
    Result StartDetectingFaces();
    Result StopDetectingFaces();
    
    // Returns field of view (radians) of camera
    f32 GetVerticalFOV();
    f32 GetHorizontalFOV();
    
    const FaceDetectionParameters& GetFaceDetectionParams();
    
    std::string GetModeName(Mode mode) const;
    std::string GetCurrentModeName() const;
    
    void SetParams(const bool autoExposureOn,
                   const f32 exposureTime,
                   const s32 integerCountsIncrement,
                   const f32 minExposureTime,
                   const f32 maxExposureTime,
                   const u8 highValue,
                   const f32 percentileToMakeHigh);

    void SetFaceDetectParams(const f32 scaleFactor,
                             const s32 minNeighbors,
                             const s32 minObjectHeight,
                             const s32 minObjectWidth,
                             const s32 maxObjectHeight,
                             const s32 maxObjectWidth);
  
    // These return true if a mailbox messages was available, and they copy
    // that message into the passed-in message struct.
    //bool CheckMailbox(ImageChunk&          msg);
    bool CheckMailbox(MessageDockingErrorSignal&  msg);
    //bool CheckMailbox(MessageFaceDetection&       msg);
    bool CheckMailbox(MessageVisionMarker&        msg);
    bool CheckMailbox(MessageTrackerQuad&         msg);
    bool CheckMailbox(MessagePanAndTiltHead&      msg);
    bool CheckMailbox(Vision::TrackedFace&        msg);
    
  protected:
    
#   if ANKI_COZMO_USE_MATLAB_VISION
    // For prototyping with Matlab
    Matlab _matlab;
#   endif
    
    // Previous image for doing background subtraction, e.g. for saliency
    Vision::Image _prevImage;
    
    //
    // Formerly in Embedded VisionSystem "private" namespace:
    //
    
    bool _isInitialized;
  Data::DataPlatform* _dataPlatform;
    
    // Just duplicating this from HAL for vision functions to work with less re-writing
    struct CameraInfo {
      f32 focalLength_x, focalLength_y;
      f32 center_x, center_y;
      f32 skew;
      u16 nrows, ncols;
      f32 distortionCoeffs[NUM_RADIAL_DISTORTION_COEFFS];
      
      CameraInfo(const Vision::CameraCalibration& camCalib);
    } *_headCamInfo;
    
    enum VignettingCorrection
    {
      VignettingCorrection_Off,
      VignettingCorrection_CameraHardware,
      VignettingCorrection_Software
    };
    
    // The tracker can fail to converge this many times before we give up
    // and reset the docker
    // TODO: Move this to visionParameters
    const s32 MAX_TRACKING_FAILURES = 1;
    
    //const Anki::Cozmo::HAL::CameraInfo* _headCamInfo;
    f32 _headCamFOV_ver;
    f32 _headCamFOV_hor;
    Embedded::Array<f32> _RcamWrtRobot;
    
    u32 _mode;
    u32 _modeBeforeTracking;
    
    // Camera parameters
    // TODO: Should these be moved to (their own struct in) visionParameters.h/cpp?
    f32 _exposureTime;
    
    VignettingCorrection _vignettingCorrection = VignettingCorrection_Off;

    // For OV7725 (cozmo 2.0)
    //static const f32 _vignettingCorrectionParameters[5] = {1.56852140958887f, -0.00619880766167132f, -0.00364222219719291f, 2.75640497906470e-05f, 1.75476361058157e-05f}; //< for _vignettingCorrection == VignettingCorrection_Software, computed by fit2dCurve.m
    
    // For OV7739 (cozmo 2.1)
    // TODO: figure these out
    const f32 _vignettingCorrectionParameters[5] = {0,0,0,0,0};
    
    s32 _frameNumber;
    bool _autoExposure_enabled = true;
    s32 _trackingIteration; // Simply for display at this point
    
    // TEMP: Un-const-ing these so that we can adjust them from basestation for dev purposes.
    /*
     const s32 autoExposure_integerCountsIncrement = 3;
     const f32 autoExposure_minExposureTime = 0.02f;
     const f32 autoExposure_maxExposureTime = 0.98f;
     const u8 autoExposure_highValue = 250;
     const f32 autoExposure_percentileToMakeHigh = 0.97f;
     const s32 autoExposure_adjustEveryNFrames = 1;
     */
    s32 _autoExposure_integerCountsIncrement = 3;
    f32 _autoExposure_minExposureTime = 0.02f;
    f32 _autoExposure_maxExposureTime = 0.50f;
    u8  _autoExposure_highValue = 250;
    f32 _autoExposure_percentileToMakeHigh = 0.95f;
    f32 _autoExposure_tooHighPercentMultiplier = 0.7f;
    s32 _autoExposure_adjustEveryNFrames = 2;
    
    // Tracking marker related members
    struct MarkerToTrack {
      Anki::Vision::MarkerType  type;
      f32                       width_mm;
      Embedded::Point2f         imageCenter;
      f32                       imageSearchRadius;
      bool                      checkAngleX;
      
      MarkerToTrack();
      bool IsSpecified() const {
        return type != Anki::Vision::MARKER_UNKNOWN;
      }
      void Clear();
      bool Matches(const Embedded::VisionMarker& marker) const;
    };
    
    MarkerToTrack _markerToTrack;
    MarkerToTrack _newMarkerToTrack;
    bool          _newMarkerToTrackWasProvided = false;
    
    Embedded::Quadrilateral<f32>    _trackingQuad;
    s32                             _numTrackFailures ;
    Tracker                         _tracker;
    
    Embedded::Point3<P3P_PRECISION> _canonicalMarker3d[4];
    
    // Snapshots of robot state
    bool _wasCalledOnce, _havePreviousRobotState;
    MessageRobotState _robotState, _prevRobotState;
    
    // Parameters defined in visionParameters.h
    DetectFiducialMarkersParameters _detectionParameters;
    TrackerParameters               _trackerParameters;
    FaceDetectionParameters         _faceDetectionParameters;
    Vision::CameraResolution        _captureResolution;
    
    // For sending images to basestation
    ImageSendMode_t                 _imageSendMode = ISM_OFF;
    Vision::CameraResolution        _nextSendImageResolution = Vision::CAMERA_RES_NONE;
    
    // For taking snapshots
    bool                            _isWaitingOnSnapShot;
    bool*                           _isSnapshotReady;
    Embedded::Rectangle<s32>        _snapshotROI;
    s32                             _snapshotSubsample;
    Embedded::Array<u8>*            _snapshot;

    // FaceTracking
    Vision::FaceTracker*            _faceTracker;

    struct VisionMemory {
      /* 10X the memory for debugging on a PC
       static const s32 OFFCHIP_BUFFER_SIZE = 20000000;
       static const s32 ONCHIP_BUFFER_SIZE = 1700000; // The max here is somewhere between 175000 and 180000 bytes
       static const s32 CCM_BUFFER_SIZE = 500000; // The max here is probably 65536 (0x10000) bytes
       */
      static const s32 OFFCHIP_BUFFER_SIZE = 4000000;
      static const s32 ONCHIP_BUFFER_SIZE  = 600000;
      static const s32 CCM_BUFFER_SIZE     = 200000; 

      static const s32 MAX_MARKERS = 100; // TODO: this should probably be in visionParameters
      
      OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
      ONCHIP  char onchipBuffer[ONCHIP_BUFFER_SIZE];
      CCM     char ccmBuffer[CCM_BUFFER_SIZE];
      
      Embedded::MemoryStack _offchipScratch;
      Embedded::MemoryStack _onchipScratch;
      Embedded::MemoryStack _ccmScratch;
      
      // Markers is the one things that can move between functions, so it is always allocated in memory
      Embedded::FixedLengthList<Embedded::VisionMarker> _markers;
      
      // WARNING: ResetBuffers should be used with caution
      Result ResetBuffers();
      
      Result Initialize();
    }; // VisionMemory
    
    VisionMemory _memory;
    
    void EnableModeHelper(Mode mode);
    void DisableModeHelper(Mode mode);
    
    Embedded::Quadrilateral<f32> GetTrackerQuad(Embedded::MemoryStack scratch);
    Result UpdateRobotState(const MessageRobotState newRobotState);
    void GetPoseChange(f32& xChange, f32& yChange, Radians& angleChange);
    Result UpdateMarkerToTrack();
    Radians GetCurrentHeadAngle();
    Radians GetPreviousHeadAngle();
    
    //void DownsampleAndSendImage(const Embedded::Array<u8> &img);
    
    Result LookForMarkers(const Embedded::Array<u8> &grayscaleImage,
                          const DetectFiducialMarkersParameters &parameters,
                          Embedded::FixedLengthList<Embedded::VisionMarker> &markers,
                          Embedded::MemoryStack ccmScratch,
                          Embedded::MemoryStack onchipScratch,
                          Embedded::MemoryStack offchipScratch);
    
    Result InitTemplate(const Embedded::Array<u8> &grayscaleImage,
                        const Embedded::Quadrilateral<f32> &trackingQuad,
                        const TrackerParameters &parameters,
                        Tracker &tracker,
                        Embedded::MemoryStack ccmScratch,
                        Embedded::MemoryStack &onchipMemory, //< NOTE: onchip is a reference
                        Embedded::MemoryStack &offchipMemory);
    
    Result TrackTemplate(const Embedded::Array<u8> &grayscaleImage,
                         const Embedded::Quadrilateral<f32> &trackingQuad,
                         const TrackerParameters &parameters,
                         Tracker &tracker,
                         bool &trackingSucceeded,
                         Embedded::MemoryStack ccmScratch,
                         Embedded::MemoryStack onchipScratch,
                         Embedded::MemoryStack offchipScratch);
    
    Result TrackerPredictionUpdate(const Embedded::Array<u8>& grayscaleImage,
                                   Embedded::MemoryStack scratch);
    
    void FillDockErrMsg(const Embedded::Quadrilateral<f32>& currentQuad,
                        MessageDockingErrorSignal& dockErrMsg,
                        Embedded::MemoryStack scratch);
    
    Result TakeSnapshotHelper(const Embedded::Array<u8>& grayscaleImage);
    
    
    // Mailboxes for different types of messages that the vision
    // system communicates to main execution:
    //MultiMailbox<Messages::BlockMarkerObserved, MAX_BLOCK_MARKER_MESSAGES> blockMarkerMailbox_;
    //Mailbox<Messages::MatMarkerObserved>    matMarkerMailbox_;
    Mailbox<MessageDockingErrorSignal>   _dockingMailbox;
    Mailbox<MessageTrackerQuad>          _trackerMailbox;
    Mailbox<MessagePanAndTiltHead>       _panTiltMailbox;
    MultiMailbox<MessageVisionMarker,  DetectFiducialMarkersParameters::MAX_MARKERS>   _visionMarkerMailbox;
    //MultiMailbox<MessageFaceDetection, FaceDetectionParameters::MAX_FACE_DETECTIONS>   _faceDetectMailbox;
    
    MultiMailbox<Vision::TrackedFace, FaceDetectionParameters::MAX_FACE_DETECTIONS> _faceMailbox;
    
  }; // class VisionSystem
  
      
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISIONSYSTEM_H
