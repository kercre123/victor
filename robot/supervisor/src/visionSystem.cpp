
#include "anki/common/robot/config.h"
#include "anki/common/shared/radians.h"

#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/visionSystem.h"

#include "anki/cozmo/messageProtocol.h"

#define USING_MATLAB_VISION (defined(USE_MATLAB_FOR_HEAD_CAMERA) || \
defined(USE_MATLAB_FOR_MAT_CAMERA))

#if USING_MATLAB_VISION
// If using Matlab for any vision processing, enable the Matlab engine
#include "engine.h"
#include "anki/common/robot/matlabInterface.h"
#define DISPLAY_MATLAB_IMAGES 0
extern Engine *matlabEngine_;
#endif

namespace Anki {
  namespace Cozmo {
    namespace VisionSystem {
#pragma mark --- VisionSystem "Private Member Variables" ---
      
      typedef enum {
        LOOKING_FOR_BLOCKS,
        DOCKING,
        MAT_LOCALIZATION,
        VISUAL_ODOMETRY
      } Mode;
      
      // Private "Members" of the VisionSystem:
      namespace {
        
        bool isInitialized_ = false;
        
        Mode mode_ = LOOKING_FOR_BLOCKS;
        
        const HAL::CameraInfo* headCamInfo_ = NULL;
        const HAL::CameraInfo* matCamInfo_  = NULL;

        // Whether or not we're in the process of waiting for an image to be acquired
        // TODO: need one of these for both mat and head cameras?
        bool continuousCaptureStarted_ = false;
        
        u16  dockingBlock_ = 0;
        bool isDockingBlockFound_ = false;
        
        bool isTemplateInitialized_ = false;
        
#if USE_OFFBOARD_VISION
        u8 waitingForProcessingResult_ = 0;
#endif
        
        // Pointers to "Mailboxes" for communicating information back to
        // mainExecution()
        BlockMarkerMailbox* blockMarkerMailbox_ = NULL;
        MatMarkerMailbox*   matMarkerMailbox_   = NULL;
        DockingMailbox*     dockingMailbox_     = NULL;
        
        u8 msgBuffer_[256];
        
        f32 matCamPixPerMM_ = 1.f;
        
      } // private namespace VisionSystem
      
      //
      // Forward declarations:
      //
      
      // Capture an entire frame using HAL commands and put it in the given
      // frame buffer
      typedef struct {
        u8* data;
        HAL::CameraMode resolution;
      } FrameBuffer;
      
      ReturnCode CaptureHeadFrame(FrameBuffer &frame);
      ReturnCode CaptureMatFrame(FrameBuffer &frame);
      
      ReturnCode LookForBlocks(const FrameBuffer &frame);
      ReturnCode LocalizeWithMat(const FrameBuffer &frame);
      
      ReturnCode InitTemplate(const FrameBuffer &frame);
      ReturnCode TrackTemplate(const FrameBuffer &frame);
      
      ReturnCode GetRelativeOdometry(const FrameBuffer &frame);
      
      
#pragma mark --- VisionSystem::Mailbox Implementations ---
      
      /*
       //TODO: Was having trouble getting this to compile on robot.
       //      Default logic should still work when commenting this out.
       template<>
       void VisionSystem::MatMarkerMailbox::advanceIndex(u8 &index)
       {
       return;
       }
       */
      
      
#pragma mark --- VisionSystem Method Implementations ---
      
      
      ReturnCode Init(BlockMarkerMailbox*     blockMarkerMailbox,
                      MatMarkerMailbox*       matMarkerMailbox,
                      DockingMailbox*         dockingMailbox)
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
        
        if(blockMarkerMailbox == NULL) {
          PRINT("VisionSystem::Init() - BlockMarkerMailbox pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        blockMarkerMailbox_ = blockMarkerMailbox;
        
        if(matMarkerMailbox == NULL) {
          PRINT("VisionSystem::Init() - MatMarkerMailbox pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        matMarkerMailbox_ = matMarkerMailbox;
        
        if(dockingMailbox == NULL) {
          PRINT("VisionSystem::Init() - DockingMailbox pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        dockingMailbox_ = dockingMailbox;
        
        // Compute the resolution of the mat camera from its FOV and height
        // off the mat:
        f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
                                 tanf(matCamInfo_->fov_ver * .5f));
        matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
        
#if USE_OFFBOARD_VISION
        PRINT("VisionSystem::Init(): Registering message IDs for offboard processing.\n");
        
        // Register all the message IDs we need with Matlab:
        HAL::SendMessageID("CozmoMsg_ObservedBlockMarker",
                           MSG_V2B_CORE_BLOCK_MARKER_OBSERVED);
        
        HAL::SendMessageID("CozmoMsg_TemplateInitialized",
                           MSG_OFFBOARD_VISION_TEMPLATE_INITIALIZED);
        
        HAL::SendMessageID("CozmoMsg_TotalBlocksDetected",
                           MSG_OFFBOARD_VISION_TOTAL_BLOCKS_FOUND);
        
        HAL::SendMessageID("CozmoMsg_DockingErrorSignal",
                           MSG_V2B_CORE_DOCKING_ERROR_SIGNAL);
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
        dockingBlock_ = blockTypeToDockWith;
        mode_ = LOOKING_FOR_BLOCKS;
        return EXIT_SUCCESS;
      }
      
      inline void ProcessMessage(const CozmoMsg_ObservedBlockMarker& msg)
      {
        VisionSystem::blockMarkerMailbox_->putMessage(msg);
        
        // If we have a block to dock with set, see if this was it
        if(dockingBlock_ > 0 && dockingBlock_ == msg.blockType)
        {
          // This will just start docking with the first block we see
          // that matches the block type.
          // TODO: Something smarter about seeing the block in the expected place?
          isDockingBlockFound_ = true;
          
        }
      }
      
      inline void ProcessMessage(const CozmoMsg_TotalBlocksDetected& msg)
      {
        PRINT("Saw %d block markers.\n", msg.numBlocks);
        mode_ = LOOKING_FOR_BLOCKS;
      }
      
      inline void ProcessMessage(const CozmoMsg_TemplateInitialized& msg)
      {
        if(msg.success) {
          isTemplateInitialized_ = true;
        }
        else {
          isTemplateInitialized_ = false;
        }
      }
      
      inline void ProcessMessage(const CozmoMsg_DockingErrorSignal& msg)
      {
        // Just pass the docking error signal along to the mainExecution to
        // deal with. Note that if the message indicates tracking failed,
        // the mainExecution thread should handle it, and put the vision
        // system back in LOOKING_FOR_BLOCKS mode.
        VisionSystem::dockingMailbox_->putMessage(msg);
      }
      
      
      
      u8 ProcessMessage(const u8* msgBuffer)
      {
        if(msgBuffer[0] < 2) {
          PRINT("ProcessMessage: message size < 2!\n");
          return 0;
        }
        
        const u8 msgID = msgBuffer[1];
        
        // Put the packet in the corresponding Mailbox to be picked up
        // and processed by mainExecution()
        switch(msgID)
        {
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          {
            ProcessMessage(*reinterpret_cast<const CozmoMsg_ObservedBlockMarker*>(msgBuffer));
            break;
          }
            
          case MSG_OFFBOARD_VISION_TOTAL_BLOCKS_FOUND:
          {
            ProcessMessage(*reinterpret_cast<const CozmoMsg_TotalBlocksDetected*>(msgBuffer));
                           break;
          }
            
          case MSG_OFFBOARD_VISION_TEMPLATE_INITIALIZED:
          {
            ProcessMessage(*reinterpret_cast<const CozmoMsg_TemplateInitialized*>(msgBuffer));
            break;
          }
            
          case MSG_V2B_CORE_DOCKING_ERROR_SIGNAL:
          {
            ProcessMessage(*reinterpret_cast<const CozmoMsg_DockingErrorSignal*>(msgBuffer));
            break;
          }
            
          default:
            PRINT("USBProcessNextPacket(): unexpected msgID received, "
                  "reached default case!\n");
        } // switch(msgID)
        
        return msgID;
        
      } // ProcessMessage()

      
      ReturnCode Update(u8* memoryBuffer)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        // NOTE: for now, we are always capturing at full resolution and
        //       then downsampling as we send the frame out for offboard
        //       processing.  Once the hardware camera supports it, we should
        //       capture at the correct resolution directly and pass that in
        //       (and remove the downsampling from USBSendFrame()
        
        if(waitingForProcessingResult_ > 0)
        {
          PRINT("VisionSystem::Update(): waiting for processing result.\n");

          if(HAL::USBGetNextPacket(msgBuffer_) == EXIT_SUCCESS &&
             ProcessMessage(msgBuffer_) == waitingForProcessingResult_)
          {
              // Got the result message we were looking for, go back to
              // processing
              waitingForProcessingResult_ = 0;
          }
          else {
            // Still waiting, skip further vision processing below.
            return EXIT_SUCCESS;
          }
        }

        switch(mode_)
        {
          case LOOKING_FOR_BLOCKS:
          {
            FrameBuffer frame = {
              .data       = memoryBuffer,
              .resolution = HAL::CAMERA_MODE_VGA
            };
            
            CaptureHeadFrame(frame);
            
            retVal = LookForBlocks(frame);
            
            if(isDockingBlockFound_) {
              if(InitTemplate(frame) == EXIT_SUCCESS) {
                // If we successfully initialize a tracking template,
                // switch to docking mode.  Otherwise, we'll keep looking
                // for the block and try again
                mode_ = DOCKING;
              }
            }
            break;
          }
            
          case DOCKING:
          {
            if(not isTemplateInitialized_) {
              PRINT("VisionSystem::Update(): Reached DOCKING mode without template initialized.\n");
              retVal = EXIT_FAILURE;
            }
            else {
              VisionSystem::FrameBuffer frame = {
                .data       = memoryBuffer,
                .resolution = HAL::CAMERA_MODE_VGA
              };
              
              CaptureHeadFrame(frame);
              
              if(TrackTemplate(frame) == EXIT_FAILURE) {
                // Lost track.  Reset and go back to trying to find the
                // block we want to dock with.
                isDockingBlockFound_ = false;
                isTemplateInitialized_ = false;
                mode_ = LOOKING_FOR_BLOCKS;
              }
            }
            break;
          }
/*
          case MAT_LOCALIZATION:
          {
            VisionSystem::FrameBuffer frame = {
              .data       = memoryBuffer,
              .resolution = MAT_LOCALIZATION_RESOLUTION
            };
            
            CaptureMatFrame(frame);
            LocalizeWithMat(frame);
            
            break;
          }
            
          case VISUAL_ODOMETRY:
          {
            VisionSystem::FrameBuffer frame = {
              .data       = memoryBuffer,
              .resolution = MAT_ODOMETRY_RESOLUTION
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
        if (HAL::GetHeadCamMode() != HAL::CAMERA_MODE_QQQVGA)
        {
          CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                           HAL::CAMERA_UPDATE_CONTINUOUS, 0, false);
        }
        else {
          CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                           HAL::CAMERA_UPDATE_SINGLE, 0, false);
        }

        
        /*
        // Only QQQVGA can be captured in SINGLE mode.
        // Other modes must be captured in CONTINUOUS mode otherwise you get weird
        // rolling sync effects.
        // NB: CONTINUOUS mode contains tears that could affect vision algorithms
        // if moving too fast.
        // NOTE: we are currently always capturing at 640x480.  The camera mode
        //       is just affecting how much we downsample before sending the
        //       frame out over UART.
        if (HAL::GetHeadCamMode() != HAL::CAMERA_MODE_QQQVGA) {
          
          if (!continuousCaptureStarted_) {
            CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                             HAL::CAMERA_UPDATE_CONTINUOUS, 0, false);
            continuousCaptureStarted_ = true;
          }
          
        }
        else {
          CameraStartFrame(HAL::CAMERA_FRONT, frame.data, frame.resolution,
                           HAL::CAMERA_UPDATE_SINGLE, 0, false);
          continuousCaptureStarted_ = false;
        }
        */
        
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
        // to look for blocks in it
        HAL::USBSendFrame(frame.data, frame.resolution, DETECTION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_DETECTBLOCKS);
        
        waitingForProcessingResult_ = MSG_OFFBOARD_VISION_TOTAL_BLOCKS_FOUND;
        
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.
        
        // for( each marker) {
        //   CozmoMsg_ObservedBlockMarkerMsg msg;
        //   ProcessMessage(msg);
        // }
        
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        return retVal;
        
      } // lookForBlocks()
      
      
      ReturnCode LocalizeWithMat(const FrameBuffer &frame)
      {
        ReturnCode retVal = -1;
        
#if USE_OFFBOARD_VISION
        
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it
        HAL::USBSendFrame(frame.data, frame.resolution, TRACKING_RESOLUTION,
                          HAL::USB_VISION_COMMAND_MATLOCALIZATION);
        
        waitingForProcessingResult_ = MSG_V2B_CORE_MAT_MARKER_OBSERVED;
        
/*
        const s32 nrows = static_cast<s32>(frame.height);
        const s32 ncols = static_cast<s32>(frame.width);
        
#if defined(USE_MATLAB_FOR_MAT_CAMERA)
        mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(frame.data, nrows, ncols, 4);
        
        if(mxImg != NULL) {
          // Display Mat Image in Matlab
          engPutVariable(matlabEngine_, "matCamImage", mxImg);
          engEvalString(matlabEngine_, "matCamImage = matCamImage(:,:,[3 2 1]);");
#if DISPLAY_MATLAB_IMAGES
          engEvalString(matlabEngine_, "set(h_matImg, 'CData', matCamImage);");
#endif
          
          // Detect MatMarker
 
          // [xMat, yMat, orient] = matLocalization(this.matImage, ...
          //   'pixPerMM', pixPerMM, 'camera', this.robot.matCamera, ...
          //   'matSize', world.matSize, 'zDirection', world.zDirection, ...
          //   'embeddedConversions', this.robot.embeddedConversions);
           
          // % Set the pose based on the result of the matLocalization
          // this.pose = Pose(orient*[0 0 -1], ...
          // [xMat yMat this.robot.appearance.WheelRadius]);
          //    this.pose.name = 'ObservationPose';
 
          engEvalString(matlabEngine_,
                        "matMarker = matLocalization(matCamImage, "
                        "   'pixPerMM', pixPerMM, 'returnMarkerOnly', true); "
                        "matOrient = matMarker.upAngle; "
                        "isMatMarkerValid = matMarker.isValid; "
                        "xMatSquare = matMarker.X; "
                        "yMatSquare = matMarker.Y; "
                        "centroid = matMarker.centroid; "
                        "xImgCen = centroid(1); yImgCen = centroid(2); "
                        "matUpDir = matMarker.upDirection;");
          
          mxArray *mx_isValid = engGetVariable(matlabEngine_, "isMatMarkerValid");
          const bool matMarkerIsValid = mxIsLogicalScalarTrue(mx_isValid);
          
          if(matMarkerIsValid)
          {
            CozmoMsg_ObservedMatMarker msg;
            msg.size = sizeof(CozmoMsg_ObservedMatMarker);
            msg.msgID = MSG_V2B_CORE_MAT_MARKER_OBSERVED;
            
            mxArray *mx_xMatSquare = engGetVariable(matlabEngine_, "xMatSquare");
            mxArray *mx_yMatSquare = engGetVariable(matlabEngine_, "yMatSquare");
            
            msg.x_MatSquare = static_cast<u16>(mxGetScalar(mx_xMatSquare));
            msg.y_MatSquare = static_cast<u16>(mxGetScalar(mx_yMatSquare));
            
            mxArray *mx_xImgCen = engGetVariable(matlabEngine_, "xImgCen");
            mxArray *mx_yImgCen = engGetVariable(matlabEngine_, "yImgCen");
            
            msg.x_imgCenter = static_cast<f32>(mxGetScalar(mx_xImgCen));
            msg.y_imgCenter = static_cast<f32>(mxGetScalar(mx_yImgCen));
            
            mxArray *mx_upDir = engGetVariable(matlabEngine_, "matUpDir");
            msg.upDirection = static_cast<u8>(mxGetScalar(mx_upDir)) - 1; // Note the -1 for C vs. Matlab indexing
            
            mxArray *mx_matAngle = engGetVariable(matlabEngine_, "matOrient");
            msg.angle = static_cast<f32>(mxGetScalar(mx_matAngle));
            
            PRINT("Sending ObservedMatMarker message: Square (%d,%d) "
                  "at (%.1f,%.1f) with orientation %.1f degrees and upDirection=%d\n",
                  msg.x_MatSquare, msg.y_MatSquare,
                  msg.x_imgCenter, msg.y_imgCenter,
                  msg.angle * (180.f/M_PI), msg.upDirection);
            
            matMarkerMailbox_->putMessage(msg);
            
          } else {
            PRINT("No valid MatMarker found!\n");
            
          } // if marker is valid
          
          retVal = EXIT_SUCCESS;
          
        } else {
          PRINT("Robot::processHeadImage(): could not convert image to mxArray.");
        }
*/
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
        
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it
        HAL::USBSendFrame(frame.data, frame.resolution, DETECTION_RESOLUTION,
                          HAL::USB_VISION_COMMAND_INITTRACK);
        
        waitingForProcessingResult_ = MSG_OFFBOARD_VISION_TEMPLATE_INITIALIZED;
        
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
        
        waitingForProcessingResult_ = MSG_V2B_CORE_DOCKING_ERROR_SIGNAL;
        
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