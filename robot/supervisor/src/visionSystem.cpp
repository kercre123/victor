
#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"

#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/miscVisionKernels.h"

#include "anki/common/shared/radians.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/visionSystem.h"

#include "anki/cozmo/messages.h"

namespace Anki {
  namespace Cozmo {
    
    typedef enum {
      IDLE,
      LOOKING_FOR_BLOCKS,
      DOCKING,
      MAT_LOCALIZATION,
      VISUAL_ODOMETRY
    } Mode;
    
    // Private "Members" of the VisionSystem:
    namespace {
      
      // Constants / parameters:
      const s32 NUM_TRACKING_PYRAMID_LEVELS = 2;
      const f32 TRACKING_RIDGE_WEIGHT = 0.f;
      
      const s32 TRACKING_MAX_ITERATIONS = 25;
      const f32 TRACKING_CONVERGENCE_TOLERANCE = .05f;
      
      bool isInitialized_ = false;
      
      //
      // Memory
      //

      const u32 CMX_BUFFER_SIZE = 600000;
#ifdef SIMULATOR
      char cmxBuffer_[CMX_BUFFER_SIZE];
#else
      __attribute__((section(".smallBss1"))) char cmxBuffer_[CMX_BUFFER_SIZE];
#endif
      
      // DDR buffer (for captured frames at the moment)
      const u32 FRAMEBUFFER_WIDTH  = 640;
      const u32 FRAMEBUFFER_HEIGHT = 480;
      const u32 DDR_BUFFER_SIZE   = FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT;
      
#ifdef SIMULATOR
      u8 ddrBuffer_[DDR_BUFFER_SIZE];
#else
      __attribute__((section(".bigBss"))) u8 ddrBuffer_[DDR_BUFFER_SIZE];
#endif
      
      const u32 TRACKER_SCRATCH_SIZE   = 600000;
      const u32 DETECTOR_SCRATCH1_SIZE = 300000;
      const u32 DETECTOR_SCRATCH2_SIZE = 300000;
      
      Embedded::MemoryStack trackerScratch_;
      Embedded::MemoryStack detectorScratch1_;
      Embedded::MemoryStack detectorScratch2_;
      bool detectorScratchInitialized_ = false;
      
      Mode mode_ = IDLE;
      
      const HAL::CameraInfo* headCamInfo_ = NULL;
      const HAL::CameraInfo* matCamInfo_  = NULL;
      
      // Whether or not we're in the process of waiting for an image to be acquired
      // TODO: need one of these for both mat and head cameras?
      //bool continuousCaptureStarted_ = false;
      
      u16  dockingBlock_ = 0;
      bool isDockingBlockFound_ = false;
      
      bool isTemplateInitialized_ = false;
      
      // The tracker can fail to converge this many times before we give up
      // and reset the docker
      const u8 MAX_TRACKING_FAILURES = 5;
      u8 numTrackFailures_ = 0;
      
      f32 matCamPixPerMM_ = 1.f;
      
      Embedded::TemplateTracker::LucasKanadeTracker_f32 tracker_;
      
    } // private namespace
    
    namespace VisionSystem {
#pragma mark --- VisionSystem "Private Member Variables" ---
      
      //
      // Forward declarations:
      //
      
      // Capture an entire frame using HAL commands and put it in the given
      // frame buffer
      typedef struct {
        u8* data;
        HAL::CameraMode resolution;
        TimeStamp  timestamp;
      } FrameBuffer;
      
      ReturnCode CaptureHeadFrame(FrameBuffer &frame);
      ReturnCode CaptureMatFrame(FrameBuffer &frame);
      
      ReturnCode LookForBlocks(const FrameBuffer &frame);
      
      ReturnCode LocalizeWithMat(const FrameBuffer &frame);
      
      ReturnCode InitTemplate(const FrameBuffer &frame,
                              const Embedded::Rectangle<f32>& templateRegion);
      
      ReturnCode TrackTemplate(const FrameBuffer &frame);
      
      ReturnCode GetRelativeOdometry(const FrameBuffer &frame);
      
      
#pragma mark --- VisionSystem Method Implementations ---
      
      
      ReturnCode Init(void)
      {
        isInitialized_ = false;
        
        headCamInfo_ = HAL::GetHeadCamInfo();
        if(headCamInfo_ == NULL) {
          PRINT("VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        
        matCamInfo_  = HAL::GetMatCamInfo();
        if(matCamInfo_ == NULL) {
          PRINT("VisionSystem::Init() - MatCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        
        // Compute the resolution of the mat camera from its FOV and height
        // off the mat:
        f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
                                 tanf(matCamInfo_->fov_ver * .5f));
        matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
        
#if USE_OFFBOARD_VISION
        PRINT("VisionSystem::Init(): Registering message IDs for offboard processing.\n");
        
        // Register all the message IDs we need with Matlab:
        HAL::SendMessageID("CozmoMsg_BlockMarkerObserved",
                           GET_MESSAGE_ID(Messages::BlockMarkerObserved));
        
        HAL::SendMessageID("CozmoMsg_TemplateInitialized",
                           GET_MESSAGE_ID(Messages::TemplateInitialized));
        
        HAL::SendMessageID("CozmoMsg_TotalBlocksDetected",
                           GET_MESSAGE_ID(Messages::TotalBlocksDetected));
               
        HAL::SendMessageID("CozmoMsg_DockingErrorSignal",
                           GET_MESSAGE_ID(Messages::DockingErrorSignal));
        
        //HAL::SendMessageID("CozmoMsg_HeadCameraCalibration",
        //                   GET_MESSAGE_ID(Messages::HeadCameraCalibration));
        
        // TODO: Update this to send mat and head cam calibration separately
        PRINT("VisionSystem::Init(): Sending head camera calibration to "
              "offoard vision processor.\n");
        
        // Create a camera calibration message and send it to the offboard
        // vision processor
        Messages::HeadCameraCalibration headCalibMsg = {
          headCamInfo_->focalLength_x,
          headCamInfo_->focalLength_y,
          headCamInfo_->fov_ver,
          headCamInfo_->center_x,
          headCamInfo_->center_y,
          headCamInfo_->skew,
          headCamInfo_->nrows,
          headCamInfo_->ncols
        };
        
        //HAL::USBSendMessage(&msg, GET_MESSAGE_ID(HeadCameraCalibration));
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_HEAD_CALIBRATION,
                           &headCalibMsg, sizeof(Messages::HeadCameraCalibration));

        /* Don't need entire calibration, just pixPerMM
         
        Messages::MatCameraCalibration matCalibMsg = {
          matCamInfo_->focalLength_x,
          matCamInfo_->focalLength_y,
          matCamInfo_->fov_ver,
          matCamInfo_->center_x,
          matCamInfo_->center_y,
          matCamInfo_->skew,
          matCamInfo_->nrows,
          matCamInfo_->ncols
        };
        
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
                           &matCalibMsg, sizeof(Messages::MatCameraCalibration));
         */
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_MAT_CALIBRATION,
                           &matCamPixPerMM_, sizeof(matCamPixPerMM_));

#endif
        
        isInitialized_ = true;
        return EXIT_SUCCESS;
      }
      
      
      bool IsInitialized()
      {
        return isInitialized_;
      }
      
      void Destroy()
      {

      }
      
      
      ReturnCode SetDockingBlock(const u16 blockTypeToDockWith)
      {
        dockingBlock_          = blockTypeToDockWith;
        isDockingBlockFound_   = false;
        isTemplateInitialized_ = false;
        numTrackFailures_      = 0;
        
        mode_ = LOOKING_FOR_BLOCKS;
        
#if USE_OFFBOARD_VISION
        // Let the offboard vision processor know that it should be looking for
        // this block type as well, so it can do template initialization on the
        // same frame where it sees the block, instead of needing a second
        // USBSendFrame call.
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_SETDOCKBLOCK,
                           &dockingBlock_, sizeof(dockingBlock_));
#endif
        return EXIT_SUCCESS;
      }
      
      void CheckForDockingBlock(const u16 blockType)
      {
        // If we have a block to dock with set, see if this was it
        if(dockingBlock_ > 0 && dockingBlock_ == blockType)
        {
          isDockingBlockFound_ = true;
        }
      }
      
      void SetDockingMode(const bool isTemplateInitalized)
      {
        if(isTemplateInitalized)
        {
          PRINT("Tracking template initialized, switching to DOCKING mode.\n");
          isTemplateInitialized_ = true;
          isDockingBlockFound_   = true;
          
          // If we successfully initialized a tracking template,
          // switch to docking mode.  Otherwise, we'll keep looking
          // for the block and try again
          mode_ = DOCKING;
        }
        else {
          isTemplateInitialized_ = false;
          isDockingBlockFound_   = false;
        }
      }
      
      void UpdateTrackingStatus(const bool didTrackingSucceed)
      {
        if(didTrackingSucceed) {
          // Reset the failure counter
          numTrackFailures_ = 0;
        }
        else {
          ++numTrackFailures_;
          if(numTrackFailures_ == MAX_TRACKING_FAILURES) {
            
            // This resets docking, puttings us back in LOOKING_FOR_BLOCKS mode
            SetDockingBlock(dockingBlock_);
            numTrackFailures_ = 0;
          }
        }
      } // UpdateTrackingStatus()
      
      
      ReturnCode Update(void)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        // NOTE: for now, we are always capturing at full resolution and
        //       then downsampling as we send the frame out for offboard
        //       processing.  Once the hardware camera supports it, we should
        //       capture at the correct resolution directly and pass that in
        //       (and remove the downsampling from USBSendFrame()
        
#if USE_OFFBOARD_VISION
        
        //PRINT("VisionSystem::Update(): waiting for processing result.\n");
        
        Messages::ProcessUARTMessages();
        
        if(Messages::StillLookingForID())
        {
          // Still waiting, skip further vision processing below.
          return EXIT_SUCCESS;
        }
#endif

        switch(mode_)
        {
          case IDLE:
            // Nothing to do!
            break;
            
          case LOOKING_FOR_BLOCKS:
          {
            FrameBuffer frame = {
              ddrBuffer_,
              HAL::CAMERA_MODE_VGA
            };
            
            CaptureHeadFrame(frame);
            
            // Note that if a docking block was specified and we see it while
            // looking for blocks, a tracking template will be initialized and,
            // if that's successful, we will switch to DOCKING mode.
            retVal = LookForBlocks(frame);
            
            break;
          }
            
          case DOCKING:
          {
            if(not isTemplateInitialized_) {
              PRINT("VisionSystem::Update(): Reached DOCKING mode without "
                    "template initialized.\n");
              retVal = EXIT_FAILURE;
            }
            else {
              VisionSystem::FrameBuffer frame = {
                ddrBuffer_,
                HAL::CAMERA_MODE_VGA
              };
              
              CaptureHeadFrame(frame);
              
              // If tracking fails [enough times in a row], we will go back to
              // looking for the block we wanted to dock with.
              // This failure can be indicated either by an EXIT_FAILURE return
              // code, or by a flag in a message returned over USB by the
              // offboard vision processor.  If the latter, the
              // UpdateTrackingStatus() call will be made by the message
              // processing system.  (In offboard vision mode, TrackTemplate
              // generally always return EXIT_SUCCESS.)
              if(TrackTemplate(frame) == EXIT_FAILURE) {
                UpdateTrackingStatus(false);
              }
            }
            break;
          }
/*
          case MAT_LOCALIZATION:
          {
            VisionSystem::FrameBuffer frame = {
              ddrBuffer_,
              MAT_LOCALIZATION_RESOLUTION
            };
            
            CaptureMatFrame(frame);
            LocalizeWithMat(frame);
            
            break;
          }
            
          case VISUAL_ODOMETRY:
          {
            VisionSystem::FrameBuffer frame = {
              ddrBuffer_,
              MAT_ODOMETRY_RESOLUTION
            };
            
            CaptureMatFrame(frame);
            GetRelativeOdometry(frame);
            
            break;
          }
 */
          default:
            PRINT("VisionSystem::Update(): reached default case in switch statement.");
            retVal = EXIT_FAILURE;
            break;
        } // SWITCH(mode_)
        
        return retVal;
        
      } // Update()
      
          
      ReturnCode CaptureHeadFrame(FrameBuffer &frame)
      {
        // Only QQQVGA can be captured in SINGLE mode.
        // Other modes must be captured in CONTINUOUS mode otherwise you get weird
        // rolling sync effects.
        // NB: CONTINUOUS mode contains tears that could affect vision algorithms
        // if moving too fast.
        const HAL::CameraUpdateMode updateMode = (frame.resolution == HAL::CAMERA_MODE_QQQVGA ?
                                                  HAL::CAMERA_UPDATE_SINGLE :
                                                  HAL::CAMERA_UPDATE_CONTINUOUS);
        
        CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                         updateMode, 0, false);
        
        while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
        {
        }
        
        frame.timestamp = HAL::GetTimeStamp();
        
        return EXIT_SUCCESS;
        
      } // CaptureHeadFrame()
      
      ReturnCode CaptureMatFrame(FrameBuffer &frame)
      {
        PRINT("CaptureMatFrame(): mat camera available yet.\n");
        return EXIT_FAILURE;
      }
      
      
      ReturnCode LookForBlocks(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_FAILURE;
        
#if USE_OFFBOARD_VISION
       
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it. Note that if we previsouly sent the
        // offboard processor a message to set the docking block type, it will
        // also initialize a template tracker once that block type is seen
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, DETECTION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_DETECTBLOCKS);
        
        Messages::LookForID( GET_MESSAGE_ID(Messages::TotalBlocksDetected) );
        
        retVal = EXIT_SUCCESS;
        
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.
        
        if(not detectorScratchInitialized_)
        {
          detectorScratch1_ = Embedded::MemoryStack(cmxBuffer_, DETECTOR_SCRATCH1_SIZE);
          detectorScratch2_ = Embedded::MemoryStack(cmxBuffer_ + DETECTOR_SCRATCH1_SIZE,
                                                    DETECTOR_SCRATCH2_SIZE);
          detectorScratchInitialized_ = true;
        }
        
        const u16 detectWidth  = HAL::CameraModeInfo[DETECTION_RESOLUTION].width;
        const u16 detectHeight = HAL::CameraModeInfo[DETECTION_RESOLUTION].height;
        
        
        // TOOD: move these parameters up above?
        const s32 scaleImage_thresholdMultiplier = 49152; // .75 * (2^16) = 49152
        const s32 scaleImage_numPyramidLevels = 3;
        
        const s32 component1d_minComponentWidth = 0;
        const s32 component1d_maxSkipDistance = 0;
        
        const f32 minSideLength = 0.03f*static_cast<f32>(MAX(detectWidth,detectHeight));
        const f32 maxSideLength = 0.97f*static_cast<f32>(MIN(detectWidth,detectHeight));
        
        const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
        const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
        const s32 component_sparseMultiplyThreshold = 1000 << 5;
        const s32 component_solidMultiplyThreshold = 2 << 5;
        
        const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
        const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8
        
        // Unused? const s32 maxExtractedQuads = 1000/2;
        const s32 quads_minQuadArea = 100/4;
        const s32 quads_quadSymmetryThreshold = 384;
        const s32 quads_minDistanceFromImageEdge = 2;
        
        const f32 decode_minContrastRatio = 1.25;
        
        const s32 maxMarkers = 100;
        // Unused? const s32 maxConnectedComponentSegments = 25000/2;
        
        // TODO: Add downsampling here
        
        // Questin for Pete: is detectorScratch1_ the right memory stack to provide here?
        Embedded::Array<u8> image(detectHeight, detectWidth, detectorScratch2_);
        Embedded::FixedLengthList<Embedded::BlockMarker> markers(maxMarkers, detectorScratch2_);
        Embedded::FixedLengthList<Embedded::Array<f64> > homographies(maxMarkers, detectorScratch2_);
                
        for(s32 i=0; i<maxMarkers; i++) {
          Embedded::Array<f64> newArray(3, 3, detectorScratch2_);
          homographies[i] = newArray;
        } // for(s32 i=0; i<maximumSize; i++)
        
        const Embedded::Result result = SimpleDetector_Steps12345_lowMemory(image,
                                                                            markers,
                                                                            homographies,
                                                                            scaleImage_numPyramidLevels,
                                                                            scaleImage_thresholdMultiplier,
                                                                            component1d_minComponentWidth,
                                                                            component1d_maxSkipDistance,
                                                                            component_minimumNumPixels,
                                                                            component_maximumNumPixels,
                                                                            component_sparseMultiplyThreshold,
                                                                            component_solidMultiplyThreshold,
                                                                            component_percentHorizontal,
                                                                            component_percentVertical,
                                                                            quads_minQuadArea,
                                                                            quads_quadSymmetryThreshold,
                                                                            quads_minDistanceFromImageEdge,
                                                                            decode_minContrastRatio,
                                                                            detectorScratch1_,
                                                                            detectorScratch2_);
        
        if(result == Embedded::RESULT_OK)
        {
          for(s32 i_marker = 0; i_marker < markers.get_size(); ++i_marker)
          {
            Messages::BlockMarkerObserved msg;
            
            // TODO: create the message
            
            Messages::ProcessBlockMarkerObservedMessage(msg);
            
            // Processing the message could have set isDockingBlockFound to true
            // If it did, and we haven't already initialized the template tracker
            // (thanks to a previous marker setting isDockingBlockFound to true),
            // then initialize the template tracker now
            const Embedded::BlockMarker& crntMarker = markers[i_marker];
            if(isDockingBlockFound_ && not isTemplateInitialized_)
            {
              // TODO: Convert from internal fixed point to floating point here?
              f32 left   = crntMarker.corners[0].x;
              f32 right  = crntMarker.corners[0].x;
              f32 top    = crntMarker.corners[0].y;
              f32 bottom = crntMarker.corners[0].y;
              
              for(u8 i_corner=1; i_corner<4; ++i_corner) {
                left   = MIN(left,   crntMarker.corners[i_corner].x);
                right  = MAX(right,  crntMarker.corners[i_corner].x);
                top    = MIN(top,    crntMarker.corners[i_corner].y);
                bottom = MAX(bottom, crntMarker.corners[i_corner].y);
              }
              
              Embedded::Rectangle<f32> templateRegion(left, right, top, bottom);
            
              if(InitTemplate(frame, templateRegion) == EXIT_SUCCESS) {
                SetDockingMode(static_cast<bool>(true));
              }
            }
          } // for( each marker)
          
          retVal = EXIT_SUCCESS;
        }
        
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        return retVal;
        
      } // lookForBlocks()
      
      
      ReturnCode LocalizeWithMat(const FrameBuffer &frame)
      {
        ReturnCode retVal = -1;
        
#if USE_OFFBOARD_VISION
        
        // Send the offboard vision processor the frame, with the command
        // to do mat localization 
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, MAT_LOCALIZATION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_MATLOCALIZATION);
        
        Messages::LookForID( GET_MESSAGE_ID(Messages::MatMarkerObserved) );
        
#else  // if USE_OFFBOARD_VISION
        /*
         // TODO: Hook this up to Pete's vision code
         PRINT("Robot::processMatImage(): embedded vision "
         "processing not hooked up yet.\n");
         retVal = -1;
         */
#endif // defined(USE_MATLAB_FOR_MAT_CAMERA)
        
        return retVal;
        
      } // localizeWithMat()
      
      
      ReturnCode InitTemplate(const FrameBuffer              &frame,
                              const Embedded::Rectangle<f32> &templateRegion)
      {
        ReturnCode retVal = EXIT_FAILURE;
        isTemplateInitialized_ = false;
        
#if USE_OFFBOARD_VISION
        
        // When using offboard vision processing, template initialization is
        // rolled into looking for blocks, to ensure that the same frame in
        // which a desired docking block was detected is also used to
        // initialize the tracking template (without re-sending the frame
        // over USB).
        
        retVal = EXIT_SUCCESS;
        
#else
        trackerScratch_ = Embedded::MemoryStack(cmxBuffer_, TRACKER_SCRATCH_SIZE);
        detectorScratchInitialized_ = false;
        
        if(trackerScratch_.IsValid()) {
          
          using namespace Embedded::TemplateTracker;
          
          const u16 frameWidth  = HAL::CameraModeInfo[frame.resolution].width;
          const u16 frameHeight = HAL::CameraModeInfo[frame.resolution].height;
          
          // TODO: add downsampling
          // TODO: later, add stuff to init at higher res and downsample only to track
          
          Embedded::Array<u8> image(frameHeight, frameWidth, trackerScratch_);
          image.Set(frame.data, frameWidth*frameHeight);
          
          tracker_ = LucasKanadeTracker_f32(image, templateRegion,
                                            NUM_TRACKING_PYRAMID_LEVELS,
                                            TRANSFORM_TRANSLATION,
                                            TRACKING_RIDGE_WEIGHT, trackerScratch_);
          
          if(tracker_.IsValid()) {
            retVal = EXIT_SUCCESS;
          }
          
        } // if trackerScratch is valid
        
#endif // USE_OFFBOARD_VISION
        
        if(retVal == EXIT_SUCCESS) {
          isTemplateInitialized_ = true;
        }
        
        return retVal;
        
      } // InitTemplate()
      
      
      ReturnCode TrackTemplate(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
#if USE_OFFBOARD_VISION
        
        // Send the message out for tracking
        HAL::USBSendFrame(frame.data, frame.timestamp,
                          frame.resolution, TRACKING_RESOLUTION,
                          HAL::USB_VISION_COMMAND_TRACK);
        
        Messages::LookForID( GET_MESSAGE_ID(Messages::DockingErrorSignal) );
        
#else // ONBOARD VISION
        if(trackerScratch_.IsValid())
        {
          using namespace Embedded::TemplateTracker;
          
          // Create an embeddeed image array in CMX memory and copy the data
          // from the DDR buffer into it
          const u16 frameWidth  = HAL::CameraModeInfo[frame.resolution].width;
          const u16 frameHeight = HAL::CameraModeInfo[frame.resolution].height;
          
          // TODO: insert downsampling here!
          Embedded::Array<u8> image(frameHeight, frameWidth, trackerScratch_);
          image.Set(frame.data, frameHeight*frameWidth);
          
          if(tracker_.UpdateTrack(image, TRACKING_MAX_ITERATIONS,
                                  TRACKING_CONVERGENCE_TOLERANCE,
                                  trackerScratch_) == Embedded::RESULT_OK)
          {
            retVal = EXIT_SUCCESS;
          }
          
        } // if trackerScratch is valid
        
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        return retVal;
        
      } // TrackTemplate()
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki