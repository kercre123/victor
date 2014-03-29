#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"

#include "anki/vision/robot/docking_vision.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/imageProcessing.h"

#include "anki/common/shared/radians.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/benchmarking_c.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/messages.h"
#include "anki/cozmo/robot/visionSystem.h"

// TODO: make it so we don't have to include this entire file to get just the enums
#include "anki/vision/robot/visionMarkerDecisionTrees.h"

#include "headController.h"
#include "matlabVisualization.h"
#include "visionDebugStream.h"

//#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
#include "matlabVisionProcessor.h"
//#endif

#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE && !USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
#error Affine tracker requires that USE_APPROXIMATE_DOCKING_ERROR_SIGNAL = 1.
#endif

static bool isInitialized_ = false;

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
      
      using namespace Embedded;
      
      typedef enum {
        VISION_MODE_IDLE,
        VISION_MODE_LOOKING_FOR_MARKERS,
        VISION_MODE_TRACKING
      } VisionSystemMode;
      
      
#if 0
#pragma mark --- VisionMemory ---
#endif
      
      namespace VisionMemory
      {
        static const s32 OFFCHIP_BUFFER_SIZE = 2000000;
        static const s32 ONCHIP_BUFFER_SIZE = 170000; // The max here is somewhere between 175000 and 180000 bytes
        static const s32 CCM_BUFFER_SIZE = 50000; // The max here is probably 65536 (0x10000) bytes
        static const s32 MAX_MARKERS = 100;
        
        static OFFCHIP char offchipBuffer[OFFCHIP_BUFFER_SIZE];
        static ONCHIP char onchipBuffer[ONCHIP_BUFFER_SIZE];
        static CCM char ccmBuffer[CCM_BUFFER_SIZE];
        
        static MemoryStack offchipScratch_;
        static MemoryStack onchipScratch_;
        static MemoryStack ccmScratch_;
        
        // Markers is the one things that can move between functions, so it is always allocated in memory
        static FixedLengthList<VisionMarker> markers_;
        
        // WARNING: ResetBuffers should be used with caution
        static ReturnCode ResetBuffers()
        {
          offchipScratch_ = MemoryStack(offchipBuffer, OFFCHIP_BUFFER_SIZE);
          onchipScratch_ = MemoryStack(onchipBuffer, ONCHIP_BUFFER_SIZE);
          ccmScratch_ = MemoryStack(ccmBuffer, CCM_BUFFER_SIZE);
          
          if(!offchipScratch_.IsValid() || !onchipScratch_.IsValid() || !ccmScratch_.IsValid()) {
            PRINT("Error: InitializeScratchBuffers\n");
            return EXIT_FAILURE;
          }
          
          markers_ = FixedLengthList<VisionMarker>(VisionMemory::MAX_MARKERS, offchipScratch_);
          
          return EXIT_SUCCESS;
        }
        
        static ReturnCode Initialize()
        {
          return ResetBuffers();
        }
      } // namespace VisionMemory
      
#if 0
#pragma mark --- DebugStream ---
#endif
      
      
      

      
      
      
#if 0
#pragma mark --- VisionState ---
#endif
      
      namespace VisionState {
        static const s32 MAX_TRACKING_FAILURES = 1;
        
        static const Anki::Cozmo::HAL::CameraInfo* headCamInfo_;
        
        static VisionSystemMode mode_;
        
        static Anki::Vision::MarkerType markerTypeToTrack_;
        static Quadrilateral<f32> trackingQuad_;
        static s32 numTrackFailures_ ; //< The tracker can fail to converge this many times before we give up and reset the docker
        static bool isTrackingMarkerSpecified_;
        static Tracker tracker_;
        static f32 trackingMarkerWidth_mm;
        
        static bool wasCalledOnce_, havePreviousRobotState_;
        static Messages::RobotState robotState_, prevRobotState_;
        
        //// Capture an entire frame using HAL commands and put it in the given frame buffer
        //typedef struct {
        //  u8* data;
        //  HAL::CameraMode resolution;
        //  TimeStamp_t  timestamp;
        //} FrameBuffer;
        
        static ReturnCode Initialize()
        {
          mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
          headCamInfo_ = NULL;
          markerTypeToTrack_ = Anki::Vision::MARKER_UNKNOWN;
          numTrackFailures_ = 0;
          isTrackingMarkerSpecified_ = false;
          trackingMarkerWidth_mm = 0;
          
          wasCalledOnce_ = false;
          havePreviousRobotState_ = false;
          
#ifdef RUN_SIMPLE_TRACKING_TEST
          Anki::Cozmo::VisionSystem::SetMarkerToTrack(Vision::MARKER_BATTERIES,
                                                      DEFAULT_BLOCK_MARKER_WIDTH_MM);
#endif
          
          headCamInfo_ = HAL::GetHeadCamInfo();
          if(headCamInfo_ == NULL) {
            PRINT("VisionState::Initialize() - HeadCam Info pointer is NULL!\n");
            return EXIT_FAILURE;
          }
          
          //// Compute the resolution of the mat camera from its FOV and height off the mat:
          //f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
          //  tanf(matCamInfo_->fov_ver * .5f));
          //matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
          
          return EXIT_SUCCESS;
        } // VisionState::Initialize()
        
        static Quadrilateral<f32> GetTrackerQuad(MemoryStack scratch)
        {
#if USE_MATLAB_TRACKER
          return MatlabVisionProcessor::GetTrackerQuad();
#else
          return tracker_.get_transformation().get_transformedCorners(scratch);
#endif
        } // GetTrackerQuad()
        
        static ReturnCode UpdateRobotState(const Messages::RobotState newRobotState)
        {
          prevRobotState_ = robotState_;
          robotState_     = newRobotState;
          
          if(wasCalledOnce_) {
            havePreviousRobotState_ = true;
          } else {
            wasCalledOnce_ = true;
          }
          
          return EXIT_SUCCESS;
        } // VisionState::UpdateRobotState()
        
        static void GetPoseChange(f32& xChange, f32& yChange, Radians& angleChange)
        {
          AnkiAssert(havePreviousRobotState_);
          
          angleChange = Radians(robotState_.pose_angle) - Radians(prevRobotState_.pose_angle);
          
          // Position change in world (mat) coordinates
          const f32 dx = robotState_.pose_x - prevRobotState_.pose_x;
          const f32 dy = robotState_.pose_y - prevRobotState_.pose_y;
          
          // Get change in robot coordinates
          const f32 cosAngle = cosf(-prevRobotState_.pose_angle);
          const f32 sinAngle = sinf(-prevRobotState_.pose_angle);
          xChange = dx*cosAngle - dy*sinAngle;
          yChange = dx*sinAngle + dy*cosAngle;
        } // GetPoseChange()
        
        static Radians GetCurrentHeadAngle()
        {
          return robotState_.headAngle;
        }
        
        // This will return the camera's pose adjustment, in camera coordinates.
        // So x/y are in the image plane, with x pointing right and y pointing down.
        // z points out of the camera along the optical axis.
        // Rmat will be allocated as a 3x3 rotation matrix, using given memory.
        static void GetCameraPoseChange(f32& xChange, f32& yChange, f32& zChange,
                                        Array<f32>& Rmat, MemoryStack memory)
        {
          // Horizontal robot movement (robot_y) corresponds to -x_camera
          // (positive is left for robot and right for camera)
          xChange = prevRobotState_.pose_y - robotState_.pose_y;
          
          /*
           //"robotPose = Pose([0 0 0], [0 0 0]); "
           //"neckPose = Pose([0 0 0], [-13 0 33.5+14.2]); "
           //"neckPose.parent = robotPose; "
           //"HEAD_ANGLE = -10; " // degrees
           //"Rpitch = rodrigues(-HEAD_ANGLE*pi/180*[0 1 0]); "
           //"headPose = Pose(Rpitch * [0 0 1; -1 0 0; 0 -1 0], "
           //"                Rpitch * [11.45 0 -6]'); "
           //"headPose.parent = neckPose; "
           "camera = Camera('calibration', struct("
           "   'fc', calib(1:2), 'cc', calib(3:4), "
           "   'kc', zeros(5,1), 'alpha_c', 0
           */
          
        } // GetCameraPoseChange()
        
        
        
      } // namespace VisionState
      
      
#if 0
#pragma mark --- Function Implementations ---
#endif
      
      u32 DownsampleHelper(const Array<u8>& in,
                           Array<u8>& out,
                           MemoryStack scratch)
      {
        const s32 inWidth  = in.get_size(1);
        const s32 inHeight = in.get_size(0);
        
        const s32 outWidth  = out.get_size(1);
        const s32 outHeight = out.get_size(0);
        
        const u32 downsampleFactor = inWidth / outWidth;
        
        const u32 downsamplePower = Log2u32(downsampleFactor);
        
        if(downsamplePower > 0) {
          //PRINT("Downsampling [%d x %d] frame by %d.\n", inWidth, inHeight, (1 << downsamplePower));
          
          ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(in,
                                                             downsamplePower,
                                                             out,
                                                             scratch);
        } else {
          // No need to downsample, just copy the buffer
          out.Set(in);
        }
        
        return downsampleFactor;
      }
      
      const HAL::CameraInfo* GetCameraCalibration() {
        // TODO: is just returning the pointer to HAL's camera info struct kosher?
        return VisionState::headCamInfo_;
      }
      
      f32 GetTrackingMarkerWidth() {
        return VisionState::trackingMarkerWidth_mm;
      }
      
      static ReturnCode LookForMarkers(
                                       const Array<u8> &grayscaleImage,
                                       const DetectFiducialMarkersParameters &parameters,
                                       FixedLengthList<VisionMarker> &markers,
                                       MemoryStack ccmScratch,
                                       MemoryStack onchipScratch,
                                       MemoryStack offchipScratch)
      {
        AnkiAssert(parameters.isInitialized);
        
        const s32 maxMarkers = markers.get_maximumSize();
        
        FixedLengthList<Array<f32> > homographies(maxMarkers, ccmScratch);
        
        markers.set_size(maxMarkers);
        homographies.set_size(maxMarkers);
        
        for(s32 i=0; i<maxMarkers; i++) {
          Array<f32> newArray(3, 3, ccmScratch);
          homographies[i] = newArray;
        }
        
        MatlabVisualization::ResetFiducialDetection(grayscaleImage);
        
        InitBenchmarking();
        
        const Result result = DetectFiducialMarkers(
                                                    grayscaleImage,
                                                    markers,
                                                    homographies,
                                                    parameters.scaleImage_numPyramidLevels, parameters.scaleImage_thresholdMultiplier,
                                                    parameters.component1d_minComponentWidth, parameters.component1d_maxSkipDistance,
                                                    parameters.component_minimumNumPixels, parameters.component_maximumNumPixels,
                                                    parameters.component_sparseMultiplyThreshold, parameters.component_solidMultiplyThreshold,
                                                    parameters.component_minHollowRatio,
                                                    parameters.quads_minQuadArea, parameters.quads_quadSymmetryThreshold, parameters.quads_minDistanceFromImageEdge,
                                                    parameters.decode_minContrastRatio,
                                                    parameters.maxConnectedComponentSegments,
                                                    parameters.maxExtractedQuads,
                                                    false,
                                                    ccmScratch, onchipScratch, offchipScratch);
        
        if(result == RESULT_OK) {
          DebugStream::SendFiducialDetection(grayscaleImage, markers, ccmScratch, onchipScratch, offchipScratch);
          
          for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker) {
            const VisionMarker crntMarker = markers[i_marker];
            
            MatlabVisualization::SendFiducialDetection(crntMarker.corners, crntMarker.markerType);
          }
          
          MatlabVisualization::SendDrawNow();
        } // if(result == RESULT_OK)
        
        return EXIT_SUCCESS;
      } // LookForMarkers()
      
      static ReturnCode InitTemplate(
                                     const Array<u8> &grayscaleImage,
                                     const Quadrilateral<f32> &trackingQuad,
                                     const TrackerParameters &parameters,
                                     Tracker &tracker,
                                     MemoryStack ccmScratch,
                                     MemoryStack &onchipScratch, //< NOTE: onchip is a reference
                                     MemoryStack offchipScratch)
      {
        AnkiAssert(parameters.isInitialized);
        
#if USE_MATLAB_TRACKER
        return MatlabVisionProcessor::InitTemplate(grayscaleImage, trackingQuad, ccmScratch);
#endif
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
        // TODO: At some point template initialization should happen at full detection resolution but for
        //       now, we have to downsample to tracking resolution
        
        Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
        u32 downsampleFactor = DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);
        
        AnkiAssert(downsampleFactor > 0);
        // Note that the templateRegion and the trackingQuad are both at DETECTION_RESOLUTION, not
        // necessarily the resolution of the frame.
        //const u32 downsampleFactor = parameters.detectionWidth / parameters.trackingImageWidth;
        //const u32 downsamplePower = Log2u32(downsampleFactor);
        
        /*Quadrilateral<f32> trackingQuadSmall;
         
         for(s32 i=0; i<4; ++i) {
         trackingQuadSmall[i].x = trackingQuad[i].x / downsampleFactor;
         trackingQuadSmall[i].y = trackingQuad[i].y / downsampleFactor;
         }*/
        
#endif // #if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
        tracker = TemplateTracker::LucasKanadeTracker_Slow(
                                                           grayscaleImageSmall,
                                                           trackingQuad,
                                                           parameters.scaleTemplateRegionPercent,
                                                           parameters.numPyramidLevels,
                                                           Transformations::TRANSFORM_TRANSLATION,
                                                           0.0,
                                                           onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
        tracker = TemplateTracker::LucasKanadeTracker_Affine(
                                                             grayscaleImageSmall,
                                                             trackingQuad,
                                                             parameters.scaleTemplateRegionPercent,
                                                             parameters.numPyramidLevels,
                                                             Transformations::TRANSFORM_AFFINE,
                                                             onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
        tracker = TemplateTracker::LucasKanadeTracker_Projective(
                                                                 grayscaleImageSmall,
                                                                 trackingQuad,
                                                                 parameters.scaleTemplateRegionPercent,
                                                                 parameters.numPyramidLevels,
                                                                 Transformations::TRANSFORM_PROJECTIVE,
                                                                 onchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
        tracker = TemplateTracker::LucasKanadeTracker_SampledProjective(
                                                                        grayscaleImage,
                                                                        trackingQuad,
                                                                        parameters.scaleTemplateRegionPercent,
                                                                        parameters.numPyramidLevels,
                                                                        Transformations::TRANSFORM_PROJECTIVE,
                                                                        parameters.maxSamplesAtBaseLevel,
                                                                        ccmScratch,
                                                                        onchipScratch,
                                                                        offchipScratch);
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        tracker = TemplateTracker::BinaryTracker(
                                                 grayscaleImage,
                                                 trackingQuad,
                                                 parameters.scaleTemplateRegionPercent,
                                                 //parameters.edgeDetection_grayvalueThreshold,
                                                 parameters.edgeDetection_threshold_yIncrement,
                                                 parameters.edgeDetection_threshold_xIncrement,
                                                 parameters.edgeDetection_threshold_blackPercentile,
                                                 parameters.edgeDetection_threshold_whitePercentile,
                                                 parameters.edgeDetection_threshold_scaleRegionPercent,
                                                 parameters.edgeDetection_minComponentWidth,
                                                 parameters.edgeDetection_maxDetectionsPerType,
                                                 parameters.edgeDetection_everyNLines,
                                                 onchipScratch);
#endif
        
        if(!tracker.IsValid()) {
          return EXIT_FAILURE;
        }
        
        MatlabVisualization::SendTrackInit(grayscaleImage, tracker, onchipScratch);
        
#if DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        DebugStream::SendBinaryTracker(tracker, ccmScratch, onchipScratch, offchipScratch);
#endif
        
        return EXIT_SUCCESS;
      } // InitTemplate()
      
      static ReturnCode TrackTemplate(
                                      const Array<u8> &grayscaleImage,
                                      const Quadrilateral<f32> &trackingQuad,
                                      const TrackerParameters &parameters,
                                      Tracker &tracker,
                                      bool &converged,
                                      MemoryStack ccmScratch,
                                      MemoryStack onchipScratch,
                                      MemoryStack offchipScratch)
      {
        AnkiAssert(parameters.isInitialized);
        
#if USE_MATLAB_TRACKER
        return MatlabVisionProcessor::TrackTemplate(grayscaleImage, converged, ccmScratch);
#endif
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
        // TODO: At some point template initialization should happen at full detection resolution
        //       but for now, we have to downsample to tracking resolution
        Array<u8> grayscaleImageSmall(parameters.trackingImageHeight, parameters.trackingImageWidth, ccmScratch);
        DownsampleHelper(grayscaleImage, grayscaleImageSmall, ccmScratch);
        
        //DebugStream::SendArray(grayscaleImageSmall);
#endif
        
        converged = false;
        s32 verify_meanAbsoluteDifference;
        s32 verify_numInBounds;
        s32 verify_numSimilarPixels;
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SLOW
        const Result trackerResult = tracker.UpdateTrack(
                                                         grayscaleImage,
                                                         parameters.maxIterations,
                                                         parameters.convergenceTolerance,
                                                         parameters.useWeights,
                                                         converged,
                                                         onchipScratch);
        
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
        const Result trackerResult = tracker.UpdateTrack(
                                                         grayscaleImageSmall,
                                                         parameters.maxIterations,
                                                         parameters.convergenceTolerance,
                                                         parameters.verify_maxPixelDifference,
                                                         converged,
                                                         verify_meanAbsoluteDifference,
                                                         verify_numInBounds,
                                                         verify_numSimilarPixels,
                                                         onchipScratch);
        
        //tracker.get_transformation().Print("track");
        
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
        const Result trackerResult = tracker.UpdateTrack(
                                                         grayscaleImageSmall,
                                                         parameters.maxIterations,
                                                         parameters.convergenceTolerance,
                                                         parameters.verify_maxPixelDifference,
                                                         converged,
                                                         verify_meanAbsoluteDifference,
                                                         verify_numInBounds,
                                                         verify_numSimilarPixels,
                                                         onchipScratch);
        
        //tracker.get_transformation().Print("track");
        
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
        const Result trackerResult = tracker.UpdateTrack(
                                                         grayscaleImage,
                                                         parameters.maxIterations,
                                                         parameters.convergenceTolerance,
                                                         parameters.verify_maxPixelDifference,
                                                         converged,
                                                         verify_meanAbsoluteDifference,
                                                         verify_numInBounds,
                                                         verify_numSimilarPixels,
                                                         onchipScratch);
        
        //tracker.get_transformation().Print("track");
        
#elif DOCKING_ALGORITHM == DOCKING_BINARY_TRACKER
        s32 numMatches = -1;
        
        const Result trackerResult = tracker.UpdateTrack(
                                                         grayscaleImage,
                                                         //parameters.edgeDetection_grayvalueThreshold,
                                                         parameters.edgeDetection_threshold_yIncrement,
                                                         parameters.edgeDetection_threshold_xIncrement,
                                                         parameters.edgeDetection_threshold_blackPercentile,
                                                         parameters.edgeDetection_threshold_whitePercentile,
                                                         parameters.edgeDetection_threshold_scaleRegionPercent,
                                                         parameters.edgeDetection_minComponentWidth,
                                                         parameters.edgeDetection_maxDetectionsPerType,
                                                         parameters.edgeDetection_everyNLines,
                                                         parameters.matching_maxTranslationDistance,
                                                         parameters.matching_maxProjectiveDistance,
                                                         parameters.verification_maxTranslationDistance,
                                                         false,
                                                         numMatches,
                                                         ccmScratch, offchipScratch);
        
        //tracker.get_transformation().Print("track");
        
        const s32 numTemplatePixels = tracker.get_numTemplatePixels();
        
        const f32 percentMatchedPixels = static_cast<f32>(numMatches) / static_cast<f32>(numTemplatePixels);
        
        if(percentMatchedPixels >= parameters.percentMatchedPixelsThreshold) {
          converged = true;
        } else {
          converged = false;
        }
        
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PLANAR6DOF
        
        // Nothing to do until this is implemented in embedded code!
        const Result trackerResult = RESULT_OK;
        
#else
#error Unknown DOCKING_ALGORITHM!
#endif
        
        if(trackerResult != RESULT_OK) {
          return EXIT_FAILURE;
        }
        
        // Check for a super shrunk or super large template
        {
          // TODO: make not hacky
          const Array<f32> &homography = tracker.get_transformation().get_homography();
          
          const s32 numValues = 4;
          const s32 numMaxValues = 2;
          f32 values[numValues] = {ABS(homography[0][0]), ABS(homography[0][1]), ABS(homography[1][0]), ABS(homography[1][1])};
          s32 maxInds[numMaxValues] = {0, 1};
          for(s32 i=1; i<numValues; i++) {
            if(values[i] > values[maxInds[0]]) {
              maxInds[0] = i;
            }
          }
          
          for(s32 i=0; i<numValues; i++) {
            if(i == maxInds[0])
              continue;
            
            if(values[i] > values[maxInds[1]]) {
              maxInds[1] = i;
            }
          }
          
          const f32 secondValue = values[maxInds[1]];
          
          if(secondValue < 0.1f || secondValue > 40.0f) {
            converged = false;
          }
        }
        
        MatlabVisualization::SendTrack(grayscaleImage, tracker, converged, offchipScratch);
        
        //MatlabVisualization::SendTrackerPrediction_Compare(tracker, offchipScratch);
        
        DebugStream::SendTrackingUpdate(grayscaleImage, tracker, parameters, ccmScratch, onchipScratch, offchipScratch);
        
        return EXIT_SUCCESS;
      } // TrackTemplate()
      
      
      
      
      static DetectFiducialMarkersParameters detectionParameters_;
      static TrackerParameters               trackerParameters_;
      static HAL::CameraMode                 captureResolution_;
      
#ifdef SIMULATOR
      static SimulatorParameters             simulatorParameters_;
#endif
      
      ReturnCode Init()
      {
        ReturnCode result = EXIT_SUCCESS;
        
        if(!isInitialized_) {
          
          captureResolution_ = HAL::CAMERA_MODE_QVGA;
          
          // WARNING: the order of these initializations matter!
          
          // TODO: Figure out the intertwinedness
          
          // Do this one FIRST! this initializes headCamInfo_
          result = VisionState::Initialize();
          if(result != EXIT_SUCCESS) { return result; }
          
          result = VisionMemory::Initialize();
          if(result != EXIT_SUCCESS) { return result; }
          
          result = DebugStream::Initialize();
          if(result != EXIT_SUCCESS) { return result; }
          
          AnkiAssert(VisionState::headCamInfo_ != NULL);
          
          detectionParameters_.Initialize();
          trackerParameters_.Initialize();
          
          //result = DetectFiducialMarkersParameters::Initialize();
          //if(result != EXIT_SUCCESS) { return result; }
          
          //result = TrackerParameters::Initialize(detectionParameters_.detectionResolution,
          //                                       VisionState::headCamInfo_->focalLength_x);
          //if(result != EXIT_SUCCESS) { return result; }
          
#ifdef SIMULATOR
          result = simulatorParameters_.Initialize();
          if(result != EXIT_SUCCESS) { return result; }
#endif
          
          result = MatlabVisualization::Initialize();
          if(result != EXIT_SUCCESS) { return result; }
          
#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
          result = MatlabVisionProcessor::Initialize();
          if(result != EXIT_SUCCESS) { return result; }
#endif
          
          //result = Offboard::Initialize();
          //if(result != EXIT_SUCCESS) { return result; }
          
          AnkiAssert(detectionParameters_.isInitialized);
          AnkiAssert(trackerParameters_.isInitialized);
          
          isInitialized_ = true;
        }
        
        return result;
      }
      
      void Destroy()
      {
      }
      
      ReturnCode SetMarkerToTrack(const Vision::MarkerType& markerToTrack,
                                  const f32 markerWidth_mm)
      {
        VisionState::markerTypeToTrack_ = markerToTrack;
        VisionState::trackingMarkerWidth_mm = markerWidth_mm;
        VisionState::mode_ = VISION_MODE_LOOKING_FOR_MARKERS;
        VisionState::numTrackFailures_ = 0;
        
        // If the marker type is valid, start looking for it
        if(markerToTrack < Vision::NUM_MARKER_TYPES &&
           markerWidth_mm > 0.f)
        {
          VisionState::isTrackingMarkerSpecified_ = true;
        } else {
          VisionState::isTrackingMarkerSpecified_ = false;
        }
        
        return EXIT_SUCCESS;
      }
      
      void StopTracking()
      {
        VisionState::mode_ = VISION_MODE_IDLE;
      }
      
      //
      // Tracker Prediction
      //
      // Adjust the tracker transformation by approximately how much we
      // think we've moved since the last tracking call.
      //
      ReturnCode TrackerPredictionUpdate(const Array<u8>& grayscaleImage, MemoryStack scratch)
      {
        const Quadrilateral<f32> currentQuad = VisionState::GetTrackerQuad(scratch);
        
        MatlabVisualization::SendTrackerPrediction_Before(grayscaleImage, currentQuad);
        
        // Ask VisionState how much we've moved since last call (in robot coordinates)
        Radians theta_robot;
        f32 T_fwd_robot, T_hor_robot;
        
        VisionState::GetPoseChange(T_fwd_robot, T_hor_robot, theta_robot);
        Radians theta_head = VisionState::GetCurrentHeadAngle();
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PLANAR6DOF
        
        MatlabVisionProcessor::UpdateTracker(T_fwd_robot, T_hor_robot,
                                             theta_robot, theta_head);
        
#else
        const Quadrilateral<f32> sortedQuad  = currentQuad.ComputeClockwiseCorners();
        
        f32 dx = sortedQuad[3].x - sortedQuad[0].x;
        f32 dy = sortedQuad[3].y - sortedQuad[0].y;
        const f32 observedVerticalSize_pix = sqrtf( dx*dx + dy*dy );
        
        // Compare observed vertical size to actual block marker size (projected
        // to be orthogonal to optical axis, using head angle) to approximate the
        // distance to the marker along the camera's optical axis
        const f32 cosHeadAngle = cosf(theta_head.ToFloat());
        const f32 sinHeadAngle = sinf(theta_head.ToFloat());
        const f32 d = (VisionState::trackingMarkerWidth_mm* cosHeadAngle *
                       VisionState::headCamInfo_->focalLength_y /
                       observedVerticalSize_pix);
        
        // Convert to how much we've moved along (and orthogonal to) the camera's optical axis
        const f32 T_fwd_cam =  T_fwd_robot*cosHeadAngle;
        const f32 T_ver_cam = -T_fwd_robot*sinHeadAngle;
        
        // Predict approximate horizontal shift from two things:
        // 1. The rotation fo the robot
        //    Compute pixel-per-degree of the camera and multiply by degrees rotated
        // 2. Convert horizontal shift of the robot to pixel shift, using
        //    focal length
        f32 horizontalShift_pix = (static_cast<f32>(VisionState::headCamInfo_->nrows/2) * theta_robot.ToFloat() /
                                   VisionState::headCamInfo_->fov_ver) + (T_hor_robot*VisionState::headCamInfo_->focalLength_x/d);
        
        // Predict approximate scale change by comparing the distance to the
        // object before and after forward motion
        const f32 scaleChange = d / (d - T_fwd_cam);
        
        // Predict approximate vertical shift in the camera plane by comparing
        // vertical motion (orthogonal to camera's optical axis) to the focal
        // length
        const f32 verticalShift_pix = T_ver_cam * VisionState::headCamInfo_->focalLength_y/d;
        
#ifndef THIS_IS_PETES_BOARD
        PRINT("Adjusting transformation: %.3fpix H shift for %.3fdeg rotation, "
              "%.3f scaling and %.3f V shift for %.3f translation forward (%.3f cam)\n",
              horizontalShift_pix, theta_robot.getDegrees(), scaleChange,
              verticalShift_pix, T_fwd_robot, T_fwd_cam);
#endif
        
        // Adjust the Transformation
        // Note: UpdateTransformation is doing *inverse* composition (thus using the negatives)
        if(VisionState::tracker_.get_transformation().get_transformType() == Transformations::TRANSFORM_TRANSLATION) {
          Array<f32> update(1,2,scratch);
          update[0][0] = -horizontalShift_pix;
          update[0][1] = -verticalShift_pix;
          
#if USE_MATLAB_TRACKER
          MatlabVisionProcessor::UpdateTracker(update);
#else
          VisionState::tracker_.UpdateTransformation(update, 1.f, scratch,
                                                     Transformations::TRANSFORM_TRANSLATION);
#endif
        }
        else {
          // Inverse update we are composing is:
          //
          //                  [s 0 0]^(-1)     [0 0 h_shift]^(-1)
          //   updateMatrix = [0 s 0]       *  [0 0 v_shift]
          //                  [0 0 1]          [0 0    1   ]
          //
          //      [1/s  0  -h_shift/s]   [ update_0  update_1  update_2 ]
          //   =  [ 0  1/2 -v_shift/s] = [ update_3  update_4  update_5 ]
          //      [ 0   0      1     ]   [    0         0         1     ]
          //
          // Note: UpdateTransformation adds 1.0 to the diagonal scale terms
          Array<f32> update(1,6,scratch);
          update.Set(0.f);
          update[0][0] = 1.f/scaleChange - 1.f;               // first row, first col
          update[0][2] = -horizontalShift_pix/scaleChange;    // first row, last col
          update[0][4] = 1.f/scaleChange - 1.f;               // second row, second col
          update[0][5] = -verticalShift_pix/scaleChange;      // second row, last col
          
#if USE_MATLAB_TRACKER
          MatlabVisionProcessor::UpdateTracker(update);
#else
          VisionState::tracker_.UpdateTransformation(update, 1.f, scratch,
                                                     Transformations::TRANSFORM_AFFINE);
#endif
        } // if(tracker transformation type == TRANSLATION...)
        
#endif // if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PLANAR6DOF
        
        MatlabVisualization::SendTrackerPrediction_After(VisionState::GetTrackerQuad(scratch));
        
        return EXIT_SUCCESS;
      } // TrackerPredictionUpdate()
      
      
      //      ReturnCode Update_Offboard()
      //      {
      //#if USE_OFFBOARD_VISION
      //        //PRINT("VisionSystem::Update(): waiting for processing result.\n");
      //        Messages::ProcessUARTMessages();
      //        if(Messages::StillLookingForID()) {
      //          // Still waiting, skip further vision processing below.
      //          return EXIT_SUCCESS;
      //        }
      //
      //        if(VisionState::mode_ == VISION_MODE_IDLE) {
      //        } else if(VisionState::mode_ == LOOKING_FOR_MARKERS) {
      //          // Send the offboard vision processor the frame, with the command to look for blocks in
      //          // it. Note that if we previsouly sent the offboard processor a message to set the docking
      //          // block type, it will also initialize a template tracker once that block type is seen
      //          HAL::USBSendFrame(frame.data, frame.timestamp,
      //            frame.resolution, DETECTION_RESOLUTION,
      //            HAL::USB_VISION_COMMAND_DETECTBLOCKS);
      //
      //          Messages::LookForID( GET_MESSAGE_ID(Messages::TotalVisionMarkersSeen) );
      //#ifdef SIMULATOR
      //          frameRdyTimeUS_ = HAL::GetMicroCounter() + LOOK_FOR_BLOCK_PERIOD_US;
      //#endif
      //        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
      //#ifdef SIMULATOR
      //          frameRdyTimeUS_ = HAL::GetMicroCounter() + TRACK_BLOCK_PERIOD_US;
      //#endif
      //
      //          if(TrackTemplate(frame) != EXIT_SUCCESS) {
      //            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
      //            return EXIT_FAILURE;
      //          }
      //        } else {
      //          PRINT("VisionSystem::Update(): reached default case in switch statement.");
      //          return EXIT_FAILURE;
      //        }
      //#endif // USE_OFFBOARD_VISION
      //        return EXIT_SUCCESS;
      //      }// ReturnCode Update_Offboard()
      
      static void FillDockErrMsg(const Quadrilateral<f32>& currentQuad,
                                 Messages::DockingErrorSignal& dockErrMsg)
      {
        
#if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
        const bool useTopBar = false; // TODO: pass in? make a docker parameter?
        const f32 focalLength_x = VisionState::headCamInfo_->focalLength_x;
        const f32 imageResolutionWidth_pix = detectionParameters_.detectionWidth;
        
        Quadrilateral<f32> sortedQuad = currentQuad.ComputeClockwiseCorners();
        const Point<f32>& lineLeft  = (useTopBar ? sortedQuad[0] : sortedQuad[3]); // topLeft  or bottomLeft
        const Point<f32>& lineRight = (useTopBar ? sortedQuad[1] : sortedQuad[2]); // topRight or bottomRight
        
        AnkiAssert(lineRight.x > lineLeft.x);
        
        //L = sqrt(sum( (upperRight-upperLeft).^2) );
        const f32 lineDx = lineRight.x - lineLeft.x;
        const f32 lineDy = lineRight.y - lineLeft.y;
        const f32 lineLength = sqrtf(lineDx*lineDx + lineDy*lineDy);
        
        // Get the angle from vertical of the top or bottom bar of the marker
        //we're tracking
        
        //angleError = -asin( (upperRight(2)-upperLeft(2)) / L);
        //const f32 angleError = -asinf( (upperRight.y-upperLeft.y) / lineLength);
        const f32 angleError = -asinf( (lineRight.y-lineLeft.y) / lineLength) * 4;  // Multiply by scalar which makes angleError a little more accurate.  TODO: Something smarter than this.
        
        //currentDistance = BlockMarker3D.ReferenceWidth * this.calibration.fc(1) / L;
        const f32 distanceError = VisionState::trackingMarkerWidth_mm * focalLength_x / lineLength;
        
        //ANS: now returning error in terms of camera. mainExecution converts to robot coords
        // //distError = currentDistance - CozmoDocker.LIFT_DISTANCE;
        // const f32 distanceError = currentDistance - cozmoLiftDistanceInMM;
        
        // TODO: should I be comparing to ncols/2 or calibration center?
        
        //midPointErr = -( (upperRight(1)+upperLeft(1))/2 - this.trackingResolution(1)/2 );
        f32 midpointError = -( (lineRight.x+lineLeft.x)/2 - imageResolutionWidth_pix/2 );
        
        //midPointErr = midPointErr * currentDistance / this.calibration.fc(1);
        midpointError *= distanceError / focalLength_x;
        
        dockErrMsg.x_distErr = distanceError;
        dockErrMsg.y_horErr  = midpointError;
        dockErrMsg.angleErr  = angleError;
        
#else // Use projective-pose error signal
        
#if USE_MATLAB_TRACKER
        MatlabVisionProcessor::ComputeProjectiveDockingSignal(currentQuad,
                                                              dockErrMsg.x_distErr,
                                                              dockErrMsg.y_horErr,
                                                              dockErrMsg.angleErr);
#else
#error Projective-pose docking error signal not yet implemented outside of Matlab.
        // TODO: Implement projective-pose docking error signal.
#endif // if USE_MATLAB_TRACKER
        
#endif // if USE_APPROXIMATE_DOCKING_ERROR_SIGNAL
        
      }
      
      ReturnCode Update(const Messages::RobotState robotState)
      {
        const f32 exposure = 0.1f;
        
        // This should be called from elsewhere first, but calling it again won't hurt
        Init();
        
#ifdef SIMULATOR
        if (HAL::GetMicroCounter() < simulatorParameters_.frameRdyTimeUS_) {
          return EXIT_SUCCESS;
        }
#endif
        
#ifdef SEND_IMAGE_ONLY
        VisionMemory::ResetBuffers();
        
        const s32 captureHeight = CameraModeInfo[captureResolution_].height;
        const s32 captureWidth  = CameraModeInfo[captureResolution_].width;
        
        Array<u8> grayscaleImage(captureHeight, captureWidth,
                                 VisionMemory::onchipScratch_, Flags::Buffer(false,false,false));
        
        HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_rawDataPointer()),
                            captureResolution_, exposure, false);
        
#ifdef SEND_BINARY_IMAGE_ONLY
        DebugStream::SendBinaryImage(grayscaleImage, VisionState::tracker_, trackerParameters_, VisionMemory::ccmScratch_, VisionMemory::onchipScratch_, VisionMemory::offchipScratch_);
        HAL::MicroWait(250000);
#else
        DebugStream::SendArray(grayscaleImage);
        HAL::MicroWait(1000000);
#endif
        
        return EXIT_SUCCESS;
#endif
        
        VisionState::UpdateRobotState(robotState);
        
        //#if USE_OFFBOARD_VISION
        //        return Update_Offboard();
        //#endif // USE_OFFBOARD_VISION
        
        const TimeStamp_t imageTimeStamp = HAL::GetTimeStamp();
        
        if(VisionState::mode_ == VISION_MODE_IDLE) {
          // Nothing to do!
        }
        else if(VisionState::mode_ == VISION_MODE_LOOKING_FOR_MARKERS) {
#ifdef SIMULATOR
          simulatorParameters_.frameRdyTimeUS_ = HAL::GetMicroCounter() + SimulatorParameters::LOOK_FOR_BLOCK_PERIOD_US;
#endif
          
          VisionMemory::ResetBuffers();
          
          MemoryStack offchipScratch_local(VisionMemory::offchipScratch_);
          
          const s32 captureHeight = CameraModeInfo[captureResolution_].height;
          const s32 captureWidth  = CameraModeInfo[captureResolution_].width;
          
          Array<u8> grayscaleImage(captureHeight, captureWidth,
                                   offchipScratch_local, Flags::Buffer(false,false,false));
          
          HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_rawDataPointer()),
                              captureResolution_, exposure, false);
          
          const ReturnCode result = LookForMarkers(
                                                   grayscaleImage,
                                                   detectionParameters_,
                                                   VisionMemory::markers_,
                                                   VisionMemory::ccmScratch_,
                                                   VisionMemory::onchipScratch_,
                                                   offchipScratch_local);
          
          if(result != EXIT_SUCCESS) {
            return EXIT_FAILURE;
          }
          
          const s32 numMarkers = VisionMemory::markers_.get_size();
          bool isTrackingMarkerFound = false;
          for(s32 i_marker = 0; i_marker < numMarkers; ++i_marker)
          {
            const VisionMarker& crntMarker = VisionMemory::markers_[i_marker];
            
            // Create a vision marker message and process it (which just queues it
            // in the mailbox to be picked up and sent out by main execution)
            {
              Messages::VisionMarker msg;
              msg.timestamp  = imageTimeStamp;
              msg.markerType = crntMarker.markerType;
              
              msg.x_imgLowerLeft = crntMarker.corners[Quadrilateral<f32>::BottomLeft].x;
              msg.y_imgLowerLeft = crntMarker.corners[Quadrilateral<f32>::BottomLeft].y;
              
              msg.x_imgUpperLeft = crntMarker.corners[Quadrilateral<f32>::TopLeft].x;
              msg.y_imgUpperLeft = crntMarker.corners[Quadrilateral<f32>::TopLeft].y;
              
              msg.x_imgUpperRight = crntMarker.corners[Quadrilateral<f32>::TopRight].x;
              msg.y_imgUpperRight = crntMarker.corners[Quadrilateral<f32>::TopRight].y;
              
              msg.x_imgLowerRight = crntMarker.corners[Quadrilateral<f32>::BottomRight].x;
              msg.y_imgLowerRight = crntMarker.corners[Quadrilateral<f32>::BottomRight].y;
              
              Messages::ProcessVisionMarkerMessage(msg);
            }
            
            // Was the desired marker found? If so, start tracking it.
            if(VisionState::isTrackingMarkerSpecified_ && !isTrackingMarkerFound &&
               crntMarker.markerType == VisionState::markerTypeToTrack_)
            {
              // We will start tracking the _first_ marker of the right type that
              // we see.
              // TODO: Something smarter to track the one closest to the image center or to the expected location provided by the basestation?
              isTrackingMarkerFound = true;
              
              // I'd rather only initialize trackingQuad_ if InitTemplate() succeeds, but
              // InitTemplate downsamples it for the time being, since we're still doing template
              // initialization at tracking resolution instead of the eventual goal of doing it at
              // full detection resolution.
              VisionState::trackingQuad_ = Quadrilateral<f32>(
                                                              Point<f32>(crntMarker.corners[0].x, crntMarker.corners[0].y),
                                                              Point<f32>(crntMarker.corners[1].x, crntMarker.corners[1].y),
                                                              Point<f32>(crntMarker.corners[2].x, crntMarker.corners[2].y),
                                                              Point<f32>(crntMarker.corners[3].x, crntMarker.corners[3].y));
              
              const ReturnCode result = InitTemplate(grayscaleImage,
                                                     VisionState::trackingQuad_,
                                                     trackerParameters_,
                                                     VisionState::tracker_,
                                                     VisionMemory::ccmScratch_,
                                                     VisionMemory::onchipScratch_, //< NOTE: onchip is a reference
                                                     offchipScratch_local);
              
              if(result != EXIT_SUCCESS) {
                return EXIT_FAILURE;
              }
              
              // Template initialization succeeded, switch to tracking mode:
              // TODO: Log or issue message?
              VisionState::mode_ = VISION_MODE_TRACKING;
            } // if(isTrackingMarkerSpecified && !isTrackingMarkerFound && markerType == markerToTrack)
          } // for(each marker)
        } else if(VisionState::mode_ == VISION_MODE_TRACKING) {
#ifdef SIMULATOR
          simulatorParameters_.frameRdyTimeUS_ = HAL::GetMicroCounter() + SimulatorParameters::TRACK_BLOCK_PERIOD_US;
#endif
          
          //
          // Capture image for tracking
          //
          
          MemoryStack offchipScratch_local(VisionMemory::offchipScratch_);
          MemoryStack onchipScratch_local(VisionMemory::onchipScratch_);
          
          const s32 captureHeight = CameraModeInfo[captureResolution_].height;
          const s32 captureWidth  = CameraModeInfo[captureResolution_].width;
          
          Array<u8> grayscaleImage(captureHeight, captureWidth,
                                   onchipScratch_local, Flags::Buffer(false,false,false));
          
          HAL::CameraGetFrame(reinterpret_cast<u8*>(grayscaleImage.get_rawDataPointer()),
                              captureResolution_, exposure, false);
          
          //
          // Tracker Prediction
          //
          // Adjust the tracker transformation by approximately how much we
          // think we've moved since the last tracking call.
          //
          
          ReturnCode predictionResult = TrackerPredictionUpdate(grayscaleImage, onchipScratch_local);
          
          if(predictionResult != EXIT_SUCCESS) {
            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
            return EXIT_FAILURE;
          }
          
          //
          // Update the tracker transformation using this image
          //
          
          // Set by TrackTemplate() call
          bool converged = false;
          
          const ReturnCode trackResult = TrackTemplate(
                                                       grayscaleImage,
                                                       VisionState::trackingQuad_,
                                                       trackerParameters_,
                                                       VisionState::tracker_,
                                                       converged,
                                                       VisionMemory::ccmScratch_,
                                                       onchipScratch_local,
                                                       offchipScratch_local);
          
          if(trackResult != EXIT_SUCCESS) {
            PRINT("VisionSystem::Update(): TrackTemplate() failed.\n");
            return EXIT_FAILURE;
          }
          
          //
          // Create docking error signal from tracker
          //
          
          Messages::DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = imageTimeStamp;
          dockErrMsg.didTrackingSucceed = static_cast<u8>(converged);
          
          if(converged)
          {
            Quadrilateral<f32> currentQuad = VisionState::GetTrackerQuad(VisionMemory::onchipScratch_);
            FillDockErrMsg(currentQuad, dockErrMsg);
            
            // Reset the failure counter
            VisionState::numTrackFailures_ = 0;
          }
          else {
            VisionState::numTrackFailures_ += 1;
            
            if(VisionState::numTrackFailures_ == VisionState::MAX_TRACKING_FAILURES) {
              // This resets docking, puttings us back in VISION_MODE_LOOKING_FOR_MARKERS mode
              SetMarkerToTrack(VisionState::markerTypeToTrack_,
                               VisionState::trackingMarkerWidth_mm);
            }
          }
          
          Messages::ProcessDockingErrorSignalMessage(dockErrMsg);
        } else {
          PRINT("VisionSystem::Update(): reached default case in switch statement.");
          return EXIT_FAILURE;
        } // if(converged)
        
        return EXIT_SUCCESS;
      } // Update()
    } // namespace VisionSystem
  } // namespace Cozmo
} // namespace Anki
