
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
        
        const CameraInfo* headCamInfo_ = NULL;
        const CameraInfo* matCamInfo_  = NULL;

        // Whether or not we're in the process of waiting for an image to be acquired
        // TODO: need one of these for both mat and head cameras?
        bool continuousCaptureStarted_ = false;
        
        u16  dockingBlock_ = 0;
        bool isDockingBlockFound_ = false;
        
        bool isTemplateInitialized_ = false;
        
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
      
      
      ReturnCode Init(const VisionSystem::CameraInfo*  headCamInfo,
                      const VisionSystem::CameraInfo*  matCamInfo,
                      BlockMarkerMailbox*              blockMarkerMailbox,
                      MatMarkerMailbox*                matMarkerMailbox,
                      DockingMailbox*                  dockingMailbox)
      {
        isInitialized_ = false;
        
        if(headCamInfo == NULL) {
          PRINT("VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        headCamInfo_ = headCamInfo;
        
        if(matCamInfo == NULL) {
          PRINT("VisionSystem::Init() - MatCam Info pointer is NULL!\n");
          return EXIT_FAILURE;
        }
        matCamInfo_  = matCamInfo;
        
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
      
      ReturnCode Update(u8* memoryBuffer)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        switch(mode_)
        {
          case LOOKING_FOR_BLOCKS:
          {
            FrameBuffer frame = {
              .data   = memoryBuffer,
              .width  = DETECTION_RESOLUTION.width,
              .height = DETECTION_RESOLUTION.height
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
                .data   = memoryBuffer,
                .width  = TRACKING_RESOLUTION.width,
                .height = TRACKING_RESOLUTION.height
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
              .data   = memoryBuffer,
              .width  = MAT_LOCALIZATION_RESOLUTION.width,
              .height = MAT_LOCALIZATION_RESOLUTION.height
            };
            
            CaptureMatFrame(frame);
            LocalizeWithMat(frame);
            
            break;
          }
            
          case VISUAL_ODOMETRY:
          {
            VisionSystem::FrameBuffer frame = {
              .data   = memoryBuffer,
              .width  = MAT_ODOMETRY_RESOLUTION.width,
              .height = MAT_ODOMETRY_RESOLUTION.height
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
        // NOTE: we are currently always capturing at 640x480.  The camera mode
        //       is just affecting how much we downsample before sending the
        //       frame out over UART.
        if (HAL::GetHeadCamMode() != HAL::CAMERA_MODE_QQQVGA) {
          
          if (!continuousCaptureStarted_) {
            CameraStartFrame(HAL::CAMERA_FRONT, frame.data, HAL::CAMERA_MODE_VGA,
                             HAL::CAMERA_UPDATE_CONTINUOUS, 0, false);
            continuousCaptureStarted_ = true;
          }
          
        }
        else {
          CameraStartFrame(HAL::CAMERA_FRONT, frame.data, HAL::CAMERA_MODE_VGA,
                           HAL::CAMERA_UPDATE_SINGLE, 0, false);
          continuousCaptureStarted_ = false;
        }
        
        
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

      // Waits until a message with specified msgID is received or until
      // timeOut time has passed. Processes all messages in the mean time.
      ReturnCode WaitForProcessingResult(const u8 msgID, const u32 timeOut)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
        bool resultReceived = false;
        u32 startTime = HAL::GetMicroCounter();
        while(not resultReceived &&
              HAL::GetMicroCounter() - startTime < timeOut)
        {
          if(HAL::USBGetNextPacket(msgBuffer_) == EXIT_SUCCESS)
          {
            if(ProcessMessage(msgBuffer_) == msgID) {
              resultReceived = true;
            }
          }
        }
        
        if(not resultReceived) {
          retVal = EXIT_FAILURE;
        }
        
        return retVal;
      }
      
      
      ReturnCode LookForBlocks(const FrameBuffer &frame)
      {
        ReturnCode retVal = EXIT_SUCCESS;
        
#if USE_OFFBOARD_VISION
        
        // TODO: move this elsewhere?
        const u32 LOOKING_FOR_BLOCKS_TIMEOUT = 100000; // in microseconds
        
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it
        HAL::USBSendFrame(frame, HAL::USB_VISION_COMMAND_DETECTBLOCKS);
        
        // Wait until we get the result or timeout in the process.
        // (This will also process any other commands we get in the meant time.)
        retVal = WaitForProcessingResult(MSG_OFFBOARD_VISION_TOTAL_BLOCKS_FOUND,
                                         LOOKING_FOR_BLOCKS_TIMEOUT);
        
        
/*
        mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(frame.data, nrows, ncols, 4);
        
        if(mxImg != NULL) {
          // Send the image to matlab
          engPutVariable(matlabEngine_, "headCamImage", mxImg);
          
          // Convert from GBRA format to RGB:
          engEvalString(matlabEngine_, "headCamImage = headCamImage(:,:,[3 2 1]);");
          
#if DISPLAY_MATLAB_IMAGES
          // Display (optional)
          engEvalString(matlabEngine_, "set(h_headImg, 'CData', headCamImage);");
#endif
          
          // Detect BlockMarkers
          engEvalString(matlabEngine_, "blockMarkers = simpleDetector(headCamImage); "
                        "numMarkers = length(blockMarkers);");
          
          int numMarkers = static_cast<int>(mxGetScalar(engGetVariable(matlabEngine_, "numMarkers")));
          
          PRINT("Found %d block markers.\n", numMarkers);
          
          // Can't get the blockMarkers directly because they are Matlab objects
          // which are not happy with engGetVariable.
          //mxArray *mxBlockMarkers = engGetVariable(matlabEngine_, "blockMarkers");
          for(int i_marker=0; i_marker<numMarkers; ++i_marker) {
            
            // Get the pieces of each block marker we need individually
            char cmd[256];
            snprintf(cmd, 255,
                     "currentMarker = blockMarkers{%d}; "
                     "blockType = currentMarker.blockType; "
                     "faceType  = currentMarker.faceType; "
                     "corners   = currentMarker.corners; "
                     "upDir     = currentMarker.upDirection;", i_marker+1);
            
            engEvalString(matlabEngine_, cmd);
            
            
            // Create a message from those pieces
            
            CozmoMsg_ObservedBlockMarker msg;
            
            // TODO: Can these be filled in automatically by a constructor??
            msg.size  = sizeof(CozmoMsg_ObservedBlockMarker) - 1; // -1 for the size byte
            msg.msgID = MSG_V2B_CORE_BLOCK_MARKER_OBSERVED;
            
            mxArray *mxBlockType = engGetVariable(matlabEngine_, "blockType");
            msg.blockType = static_cast<u16>(mxGetScalar(mxBlockType));
            
            mxArray *mxFaceType = engGetVariable(matlabEngine_, "faceType");
            msg.faceType = static_cast<u8>(mxGetScalar(mxFaceType));
            
            mxArray *mxCorners = engGetVariable(matlabEngine_, "corners");
            
            mxAssert(mxGetM(mxCorners)==4 && mxGetN(mxCorners)==2,
                     "BlockMarker's corners should be 4x2 in size.");
            
            double *corners_x = mxGetPr(mxCorners);
            double *corners_y = corners_x + 4;
            
            msg.x_imgUpperLeft  = static_cast<f32>(corners_x[0]);
            msg.y_imgUpperLeft  = static_cast<f32>(corners_y[0]);
            
            msg.x_imgLowerLeft  = static_cast<f32>(corners_x[1]);
            msg.y_imgLowerLeft  = static_cast<f32>(corners_y[1]);
            
            msg.x_imgUpperRight = static_cast<f32>(corners_x[2]);
            msg.y_imgUpperRight = static_cast<f32>(corners_y[2]);
            
            msg.x_imgLowerRight = static_cast<f32>(corners_x[3]);
            msg.y_imgLowerRight = static_cast<f32>(corners_y[3]);
            
            mxArray *mxUpDir = engGetVariable(matlabEngine_, "upDir");
            msg.upDirection = static_cast<u8>(mxGetScalar(mxUpDir)) - 1; // Note the -1 for C vs. Matlab indexing
            
            msg.headAngle = HAL::MotorGetPosition(HAL::MOTOR_HEAD);
            //       // NOTE the negation here!
            //       msg.headAngle = -HAL::GetHeadAngle();
            
            PRINT("Sending ObservedBlockMarker message: Block %d, Face %d "
                  "at [(%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)] with "
                  "upDirection=%d, headAngle=%.1fdeg\n",
                  msg.blockType, msg.faceType,
                  msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
                  msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
                  msg.x_imgUpperRight, msg.y_imgUpperRight,
                  msg.x_imgLowerRight, msg.y_imgLowerRight,
                  msg.upDirection, msg.headAngle * 180.f/PI);
            
            blockMarkerMailbox_->putMessage(msg);
            
            // If the docker is looking for blocks, let it see this one and check
            // to see if it's the one it wants (otherwise, this will just be a
            // no-op)
            Docker::CheckBlockMarker(msg);
            
            ++numBlocks;
            
          } // FOR each block Marker
          
        } // IF any blockMarkers found
*/
        
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        // TODO: Call embedded vision block detector
        // For each block that's found, create a CozmoMsg_ObservedBlockMarkerMsg
        // and process it.
        
        // for( each marker) {
        //   CozmoMsg_ObservedBlockMarkerMsg msg;
        //   ProcessMessage(msg);
        // }
        
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
        
        return EXIT_SUCCESS;
        
      } // lookForBlocks()
      
      
      ReturnCode LocalizeWithMat(const FrameBuffer &frame)
      {
        ReturnCode retVal = -1;
        
#if USE_OFFBOARD_VISION
        
        // TODO: move this elsewhere?
        const u32 MAT_LOCALIZATION_TIMEOUT = 100000; // in microseconds
        
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it
        HAL::USBSendFrame(frame, HAL::USB_VISION_COMMAND_MATLOCALIZATION);
        
        // Wait until we get the result or timeout in the process.
        // (This will also process any other commands we get in the meant time.)
        retVal = WaitForProcessingResult(MSG_V2B_CORE_MAT_MARKER_OBSERVED,
                                         MAT_LOCALIZATION_TIMEOUT);
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
        
        // TODO: move this elsewhere?
        const u32 INIT_TEMPLATE_TIMEOUT = 100000; // in microseconds
        
        // Send the offboard vision processor the frame, with the command
        // to look for blocks in it
        HAL::USBSendFrame(frame, HAL::USB_VISION_COMMAND_INITTRACK);
        
        // Wait until we get the result or timeout in the process.
        // (This will also process the result and any other commands we get in
        //  the meantime.)
        retVal = WaitForProcessingResult(MSG_OFFBOARD_VISION_TEMPLATE_INITIALIZED,
                                         INIT_TEMPLATE_TIMEOUT);
        
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
        
        const u32 TRACKING_TIMEOUT = 100000;
        
        // Send the message out for tracking
        HAL::USBSendFrame(frame, HAL::USB_VISION_COMMAND_TRACK);
        
        retVal = WaitForProcessingResult(MSG_V2B_CORE_DOCKING_ERROR_SIGNAL,
                                         TRACKING_TIMEOUT);
        
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