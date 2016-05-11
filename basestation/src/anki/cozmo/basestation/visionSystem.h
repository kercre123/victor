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

#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/groundPlaneROI.h"
#include "anki/cozmo/basestation/overheadEdge.h"
#include "anki/cozmo/basestation/rollingShutterCorrector.h"

#include "anki/common/basestation/matlabInterface.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/cameraCalibration.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/faceTracker.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/profiler.h"

#include "visionParameters.h"
#include "clad/vizInterface/messageViz.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/imageTypes.h"
#include "clad/types/visionModes.h"
#include "clad/types/toolCodes.h"
#include "clad/externalInterface/messageEngineToGame.h"


namespace Anki {
namespace Embedded {
  typedef Point<float> Point2f;
}
namespace Cozmo {
    
  // Forward declaration:
  class Robot;
  class VizManager;
  
  struct VisionPoseData {
    TimeStamp_t           timeStamp;
    RobotPoseStamp        poseStamp;  // contains historical head/lift/pose info
    Pose3d                cameraPose; // w.r.t. pose in poseStamp
    bool                  groundPlaneVisible;
    Matrix_3x3f           groundPlaneHomography;
    GroundPlaneROI        groundPlaneROI;
    bool                  isMoving;
    ImuDataHistory        imuDataHistory;
  };

  class VisionSystem : public Vision::Profiler
  {
  public:

    VisionSystem(const std::string& dataPath, VizManager* vizMan);
    ~VisionSystem();
    
    //
    // Methods:
    //
    
    Result Init(const Json::Value& config);
    bool   IsInitialized() const;
    
    Result UpdateCameraCalibration(Vision::CameraCalibration& camCalib);
    
    Result EnableMode(VisionMode whichMode, bool enabled);
    bool   IsModeEnabled(VisionMode whichMode) const { return _mode & static_cast<u32>(whichMode); }
    u32    GetEnabledModes() const { return _mode; }
    void   SetModes(u32 modes) { _mode = modes; }
    
    Result EnableToolCodeCalibration(bool enable);
    
    // Accessors
    const Point2f& GetTrackingMarkerSize();
    
    // This is main Update() call to be called in a loop from above.

    Result Update(const VisionPoseData&            robotState,
                  const Vision::ImageRGB&    inputImg);
    
    Result AddCalibrationImage(const Vision::Image& calibImg);
    Result ClearCalibrationImages();
    size_t GetNumStoredCalibrationImages() const { return _calibImages.size(); }
    const std::list<Vision::Image>& GetCalibrationImages() const {return _calibImages;}
    
    void StopTracking();

    // Select a block type to look for to dock with.
    // Use MARKER_UNKNOWN to disable.
    // Next time the vision system sees a block of this type while looking
    // for blocks, it will initialize a template tracker and switch to
    // docking mode.
    // If checkAngleX is true, then tracking will be considered as a failure if
    // the X angle is greater than TrackerParameters::MAX_BLOCK_DOCKING_ANGLE.
    Result SetMarkerToTrack(const Vision::MarkerType&  markerToTrack,
                            const Point2f&             markerSize_mm,
                            const bool                 checkAngleX);
    
    // Same as above, except the robot will only start tracking the marker
    // if its observed centroid is within the specified radius (in pixels)
    // from the given image point.
    Result SetMarkerToTrack(const Vision::MarkerType&  markerToTrack,
                            const Point2f&             markerSize_mm,
                            const Embedded::Point2f&   imageCenter,
                            const f32                  radius,
                            const bool                 checkAngleX,
                            const f32                  postOffsetX_mm = 0,
                            const f32                  postOffsetY_mm = 0,
                            const f32                  posttOffsetAngle_rad = 0);
    
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
    
    // VisionMode <-> String Lookups
    std::string GetModeName(VisionMode mode) const;
    std::string GetCurrentModeName() const;
    VisionMode  GetModeFromString(const std::string& str) const;
    
    Result AssignNameToFace(Vision::FaceID_t faceID, const std::string& name);
    void SetFaceEnrollmentMode(Vision::FaceEnrollmentMode mode);
    
    Result LoadFaceAlbum(const std::string& albumName, std::list<Vision::FaceNameAndID>& namesAndIDs);
    Result SaveFaceAlbum(const std::string& albumName);
    
    void GetSerializedFaceData(std::vector<u8>& albumData,
                               std::vector<u8>& enrollData) const;
    
    Result SetSerializedFaceData(const std::vector<u8>& albumData,
                                 const std::vector<u8>& enrollData,
                                 std::list<Vision::FaceNameAndID>& namesAndIDs);

    // Returns the ID of the erased face, or UnknownFaceID if the name isn't found
    Vision::FaceID_t EraseFace(const std::string& name);
    Result           EraseFace(Vision::FaceID_t faceID);
    
    void SetParams(const bool autoExposureOn,
                   const f32 exposureTime,
                   const s32 integerCountsIncrement,
                   const f32 minExposureTime,
                   const f32 maxExposureTime,
                   const u8 highValue,
                   const f32 percentileToMakeHigh);

    const std::string& GetDataPath() const { return _dataPath; }
  
    // These return true if a mailbox messages was available, and they copy
    // that message into the passed-in message struct.
    //bool CheckMailbox(ImageChunk&          msg);
    //bool CheckMailbox(MessageFaceDetection&       msg);
    bool CheckMailbox(std::pair<Pose3d, TimeStamp_t>& markerPoseWrtCamera);
    bool CheckMailbox(Vision::ObservedMarker&     msg);
    bool CheckMailbox(VizInterface::TrackerQuad&  msg);
    //bool CheckMailbox(RobotInterface::PanAndTilt& msg);
    bool CheckMailbox(ExternalInterface::RobotObservedMotion& msg);
    bool CheckMailbox(Vision::TrackedFace&        msg);
    bool CheckMailbox(Vision::UpdatedFaceID&  msg);
    bool CheckMailbox(OverheadEdgeFrame& msg);
    bool CheckMailbox(ToolCodeInfo& msg);
    bool CheckMailbox(Vision::CameraCalibration& msg);
    
    bool CheckDebugMailbox(std::pair<std::string, Vision::Image>& msg);
    bool CheckDebugMailbox(std::pair<std::string, Vision::ImageRGB>& msg);
    
    const RollingShutterCorrector& GetRollingShutterCorrector() { return _rollingShutterCorrector; }
    void ShouldDoRollingShutterCorrection(bool b) { _doRollingShutterCorrection = b; }
    bool IsDoingRollingShutterCorrection() const { return _doRollingShutterCorrection; }
    
  protected:
  
    RollingShutterCorrector _rollingShutterCorrector;

    bool _doRollingShutterCorrection = false;
    
#   if ANKI_COZMO_USE_MATLAB_VISION
    // For prototyping with Matlab
    Matlab _matlab;
#   endif
    
    // Previous image for doing background subtraction, e.g. for saliency
    // NOTE: previous images stored at resolution of motion detection processing.
    Vision::ImageRGB _prevImage;
    Vision::ImageRGB _prevPrevImage;
    TimeStamp_t      _lastMotionTime = 0;
    //Vision::Image    _prevRatioImg;
    Anki::Point2f    _prevMotionCentroid;
    Anki::Point2f    _prevGroundMotionCentroid;
    f32              _prevCentroidFilterWeight = 0.f;
    f32              _prevGroundCentroidFilterWeight = 0.f;
    //
    // Formerly in Embedded VisionSystem "private" namespace:
    //
    
    bool _isInitialized = false;
    const std::string _dataPath;
    
    Vision::Camera _camera;
    
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
    
    u32 _mode = static_cast<u32>(VisionMode::Idle);
    u32 _modeBeforeTracking = static_cast<u32>(VisionMode::Idle);
    
    bool _calibrateFromToolCode = false;
    
    // Camera parameters
    // TODO: Should these be moved to (their own struct in) visionParameters.h/cpp?
    f32 _exposureTime = 0.2f;
    
    VignettingCorrection _vignettingCorrection = VignettingCorrection_Off;

    const f32 _vignettingCorrectionParameters[5] = {0,0,0,0,0};
    
    s32 _frameNumber = 0;
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
      Point2f                   size_mm;
      Embedded::Point2f         imageCenter;
      f32                       imageSearchRadius;
      bool                      checkAngleX;
      f32                       postOffsetX_mm;
      f32                       postOffsetY_mm;
      f32                       postOffsetAngle_rad;
      
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
    s32                             _numTrackFailures = 0;
    Tracker                         _tracker;
    bool                            _trackerJustInitialized = false;
    bool                            _isTrackingMarkerFound = false;
    
    Embedded::Point3<P3P_PRECISION> _canonicalMarker3d[4];
    
    // Snapshots of robot state
    bool _wasCalledOnce    = false;
    bool _havePrevPoseData = false;
    VisionPoseData _poseData, _prevPoseData;
    
    // Parameters defined in visionParameters.h
    DetectFiducialMarkersParameters _detectionParameters;
    TrackerParameters               _trackerParameters;
    ImageResolution                 _captureResolution;
    
    // For sending images to basestation
    ImageSendMode                 _imageSendMode = ImageSendMode::Off;
    ImageResolution               _nextSendImageResolution = ImageResolution::ImageResolutionNone;
    
    // Face detection, tracking, and recognition
    Vision::FaceTracker*          _faceTracker = nullptr;
    
    // We hold a reference to the VizManager since we often want to draw to it
    VizManager*                   _vizManager = nullptr;

    // Tool code stuff
    TimeStamp_t                   _lastToolCodeReadTime_ms = 0;
    
    // Calibration stuff
    static const u32              _kMinNumCalibImagesRequired = 4;
    std::list<Vision::Image>      _calibImages;
    bool                          _isCalibrating = false;
    
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
      
      u8 _offchipBuffer[OFFCHIP_BUFFER_SIZE];
      u8 _onchipBuffer[ONCHIP_BUFFER_SIZE];
      u8 _ccmBuffer[CCM_BUFFER_SIZE];
      
      Embedded::MemoryStack _offchipScratch;
      Embedded::MemoryStack _onchipScratch;
      Embedded::MemoryStack _ccmScratch;
      
      // Markers is the one thing that can move between functions, so it is always allocated in memory
      Embedded::FixedLengthList<Embedded::VisionMarker> _markers;
      
      // WARNING: ResetBuffers should be used with caution
      Result ResetBuffers();
      
      Result Initialize();
    }; // VisionMemory
    
    VisionMemory _memory;
    
    Embedded::Quadrilateral<f32> GetTrackerQuad(Embedded::MemoryStack scratch);
    Result UpdatePoseData(const VisionPoseData& newPoseData);
    void GetPoseChange(f32& xChange, f32& yChange, Radians& angleChange);
    Result UpdateMarkerToTrack();
    Radians GetCurrentHeadAngle();
    Radians GetPreviousHeadAngle();
    
    static Result GetImageHelper(const Vision::Image& srcImage,
                                 Embedded::Array<u8>& destArray);

    //void DownsampleAndSendImage(const Embedded::Array<u8> &img);
    Result PreprocessImage(Embedded::Array<u8>& grayscaleImage);
    
    static Result BrightnessNormalizeImage(Embedded::Array<u8>& image,
                                           const Embedded::Quadrilateral<f32>& quad);

    static Result BrightnessNormalizeImage(Embedded::Array<u8>& image,
                                           const Embedded::Quadrilateral<f32>& quad,
                                           const f32 filterWidthFraction,
                                           Embedded::MemoryStack scratch);
    
    
    Result DetectMarkers(const Vision::Image& inputImage,
                          std::vector<Quad2f>& markerQuads);

    Result InitTemplate(Embedded::Array<u8> &grayscaleImage,
                        const Embedded::Quadrilateral<f32> &trackingQuad);
    
    Result TrackTemplate(const Vision::Image& inputImage);
    
    Result TrackerPredictionUpdate(const Embedded::Array<u8>& grayscaleImage,
                                   Embedded::MemoryStack scratch);
    
    Result DetectFaces(const Vision::Image& grayImage,
                       const std::vector<Quad2f>& markerQuads);
    
    Result DetectMotion(const Vision::ImageRGB& image);
    
    Result DetectOverheadEdges(const Vision::ImageRGB& image);
    
    Result ReadToolCode(const Vision::Image& image);
    
    Result ComputeCalibration();
    
    void FillDockErrMsg(const Embedded::Quadrilateral<f32>& currentQuad,
                        DockingErrorSignal& dockErrMsg,
                        Embedded::MemoryStack scratch);
    
    
    // Contrast-limited adaptive histogram equalization (CLAHE)
    cv::Ptr<cv::CLAHE> _clahe;
    
    
    // Mailboxes for different types of messages that the vision
    // system communicates back to the vision processing thread
    Mailbox<VizInterface::TrackerQuad>        _trackerMailbox;
    Mailbox<ExternalInterface::RobotObservedMotion>  _motionMailbox;
    MultiMailbox<std::pair<Pose3d, TimeStamp_t>, 4>  _dockingMailbox; // holds timestamped marker pose w.r.t. camera
    MultiMailbox<Vision::ObservedMarker, DetectFiducialMarkersParameters::MAX_MARKERS>   _visionMarkerMailbox;

    static const s32 MAX_FACE_DETECTIONS = 16;
    MultiMailbox<Vision::TrackedFace, MAX_FACE_DETECTIONS>   _faceMailbox;
    MultiMailbox<Vision::UpdatedFaceID, MAX_FACE_DETECTIONS> _updatedFaceIdMailbox;
    
    MultiMailbox<OverheadEdgeFrame, 8> _overheadEdgeFrameMailbox;
    
    Mailbox<ToolCodeInfo> _toolCodeMailbox;
    Mailbox<Vision::CameraCalibration> _calibrationMailbox;
    
    MultiMailbox<std::pair<std::string, Vision::Image>, 10>     _debugImageMailbox;
    MultiMailbox<std::pair<std::string, Vision::ImageRGB>, 10>  _debugImageRGBMailbox;
    
    void RestoreNonTrackingMode();
    
  }; // class VisionSystem
  
      
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_VISIONSYSTEM_H
