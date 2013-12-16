
#include "anki/common/robot/config.h"
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
      
      bool isInitialized_ = false;
      
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
        HAL::TimeStamp  timeStamp;
      } FrameBuffer;
      
      ReturnCode CaptureHeadFrame(FrameBuffer &frame);
      ReturnCode CaptureMatFrame(FrameBuffer &frame);
      
      ReturnCode LookForBlocks(const FrameBuffer &frame);
      ReturnCode LocalizeWithMat(const FrameBuffer &frame);
      
      ReturnCode InitTemplate(const FrameBuffer &frame);
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
        Messages::HeadCameraCalibration msg = {
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
        HAL::USBSendPacket(HAL::USB_VISION_COMMAND_CALIBRATION,
                           &msg, sizeof(Messages::HeadCameraCalibration));
        
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
          }
        }
      } // UpdateTrackingStatus()
      
      
      ReturnCode Update(u8* memoryBuffer)
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
              memoryBuffer,
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
                memoryBuffer,
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
              memoryBuffer,
              MAT_LOCALIZATION_RESOLUTION
            };
            
            CaptureMatFrame(frame);
            LocalizeWithMat(frame);
            
            break;
          }
            
          case VISUAL_ODOMETRY:
          {
            VisionSystem::FrameBuffer frame = {
              memoryBuffer,
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
        const HAL::CameraUpdateMode updateMode = (HAL::GetHeadCamMode() == HAL::CAMERA_MODE_QQQVGA ?
                                                  HAL::CAMERA_UPDATE_SINGLE :
                                                  HAL::CAMERA_UPDATE_CONTINUOUS);
        
        CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                         updateMode, 0, false);
        
        while (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT))
        {
        }
        
        return EXIT_SUCCESS;
        
      } // CaptureHeadFrame()
      
      ReturnCode CaptureMatFrame(FrameBuffer &frame)
      {
        PRINT("CaptureMatFrame(): mat camera available yet.\n");
        return EXIT_FAILURE;
      }
      
      
      ReturnCode LookForBlocks(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
#if USE_OFFBOARD_VISION
       
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it. Note that if we previsouly sent the
        // offboard processor a message to set the docking block type, it will
        // also initialize a template tracker once that block type is seen
        HAL::USBSendFrame(frame.data, frame.resolution, DETECTION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_DETECTBLOCKS);
        
        Messages::LookForID( GET_MESSAGE_ID(Messages::TotalBlocksDetected) );
        
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.
        
        // for( each marker)
        {
          CozmoMsg_BlockMarkerObserved msg;
          ProcessBlockMarkerObservedMessage(msg);
          
          // Processing the message could have set isDockingBlockFound to true
          if(isDockingBlockFound_) {
            if(InitTemplate(frame) == EXIT_SUCCESS) {
              SetDockingMode(static_cast<bool>(msg->success));
            }
          }
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
        HAL::USBSendFrame(frame.data, frame.resolution, MAT_LOCALIZATION_RESOLUTION,
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
      
      
      ReturnCode InitTemplate(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
#if USE_OFFBOARD_VISION
        
        // When using offboard vision processing, template initialization is
        // rolled into looking for blocks, to ensure that the same frame in
        // which a desired docking block was detected is also used to
        // initialize the tracking template (without re-sending the frame
        // over USB).
        
#else
        // TODO: Call embedded vision template initalization
        //       If successful, mark isTemplateInitialized to true
        
        isTemplateInitialized_ = false;
        
#endif // USE_OFFBOARD_VISION
        
        return retVal;
        
      } // InitTemplate()
      
      
      ReturnCode TrackTemplate(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
#if USE_OFFBOARD_VISION
        
        // Send the message out for tracking
        HAL::USBSendFrame(frame.data, frame.resolution, TRACKING_RESOLUTION,
                          HAL::USB_VISION_COMMAND_TRACK);
        
        Messages::LookForID( GET_MESSAGE_ID(Messages::DockingErrorSignal) );
        
#else // ONBOARD VISION
        
        // TODO: Hook this up to Pete's vision code
        PRINT("VisionSystem::TrackTemplate(): embedded vision "
              "processing not hooked up yet.\n");
        retVal = EXIT_FAILURE;
        
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        return retVal;
        
      } // TrackTemplate()
      
    } // namespace VisionSystem
    
  } // namespace Cozmo
} // namespace Anki