#include "anki/cozmo/robot/visionSystem.h"
#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/messageProtocol.h"

#include <iostream>

#define USING_MATLAB_VISION (defined(USE_MATLAB_FOR_HEAD_CAMERA) || \
defined(USE_MATLAB_FOR_MAT_CAMERA))

#if USING_MATLAB_VISION
// If using Matlab for any vision processing, enable the Matlab engine
#include "engine.h"
#include "anki/common/robot/matlabInterface.h"
#define DISPLAY_MATLAB_IMAGES 0
#endif

namespace Anki {
  namespace Cozmo {
    
#pragma mark --- VisionSystem "Private Member Variables" ---
    
    // Private "Members" of the VisionSystem:
    namespace {
      
      bool isInitialized_ = false;
      
      HAL::FrameGrabber headCamFrameGrabber_ = NULL;
      HAL::FrameGrabber matCamFrameGrabber_  = NULL;
      
      const HAL::CameraInfo* headCamInfo_ = NULL;
      const HAL::CameraInfo* matCamInfo_  = NULL;
      
      VisionSystem::BlockMarkerMailbox* blockMarkerMailbox_ = NULL;
      VisionSystem::MatMarkerMailbox*   matMarkerMailbox_   = NULL;
      
      f32 matCamPixPerMM_ = 1.f;
      
#if USING_MATLAB_VISION
      Engine *matlabEngine_;
#endif
      
      inline const u8* getHeadCamImage()
      {
        assert(headCamFrameGrabber_ != NULL);
        assert(isInitialized_);
        return headCamFrameGrabber_();
      }
      
      inline const u8* getMatCamImage()
      {
        assert(matCamFrameGrabber_ != NULL);
        assert(isInitialized_);
        return matCamFrameGrabber_();
      }
      
    } // private namespace VisionSystem
    
    
#pragma mark --- VisionSystem::Mailbox Implementations ---
    
    
    template<>
    void VisionSystem::MatMarkerMailbox::advanceIndex(u8 &index)
    {
      return;
    }
    
#pragma mark --- VisionSystem Method Implementations ---
    
    
    ReturnCode VisionSystem::Init(HAL::FrameGrabber      headCamFrameGrabber,
                                  HAL::FrameGrabber      matCamFrameGrabber,
                                  const HAL::CameraInfo* headCamInfo,
                                  const HAL::CameraInfo* matCamInfo,
                                  BlockMarkerMailbox*    blockMarkerMailbox,
                                  MatMarkerMailbox*      matMarkerMailbox)
    {
      isInitialized_ = false;
      
      if(headCamFrameGrabber == NULL) {
        fprintf(stdout, "VisionSystem::Init() - HeadCam FrameGrabber is NULL!\n");
        return EXIT_FAILURE;
      }
      headCamFrameGrabber_ = headCamFrameGrabber;
      
      if(matCamFrameGrabber == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatCam FrameGrabber is NULL!\n");
        return EXIT_FAILURE;
      }
      matCamFrameGrabber_  = matCamFrameGrabber;
      
      if(headCamInfo == NULL) {
        fprintf(stdout, "VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      headCamInfo_ = headCamInfo;
      
      if(matCamInfo == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatCam Info pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      matCamInfo_  = matCamInfo;
      
      if(blockMarkerMailbox == NULL) {
        fprintf(stdout, "VisionSystem::Init() - BlockMarkerMailbox pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      blockMarkerMailbox_ = blockMarkerMailbox;
      
      if(matMarkerMailbox == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatMarkerMailbox pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      matMarkerMailbox_ = matMarkerMailbox;
      
      // Compute the resolution of the mat camera from its FOV and height
      // off the mat:
      f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
                               tanf(matCamInfo_->fov_ver * .5f));
      matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
      
      
#if USING_MATLAB_VISION
      
      matlabEngine_ = NULL;
      if (!(matlabEngine_ = engOpen(""))) {
        fprintf(stdout, "\nCan't start MATLAB engine!\n");
        return EXIT_FAILURE;
      }
      
      // Initialize Matlab
      engEvalString(matlabEngine_, "run('../../../../matlab/initCozmoPath.m');");
      
      // Store computed pixPerMM in Matlab for use by MatLocalization()
      engPutVariable(matlabEngine_, "pixPerMM",
                     mxCreateDoubleScalar(matCamPixPerMM_));
      
#if DISPLAY_MATLAB_IMAGES
      char cmd[256];
      snprintf(cmd, 255, "h_imgFig = figure; "
               "subplot(121); "
               "h_headImg = imshow(zeros(%d,%d)); "
               "title('Head Camera'); "
               "subplot(122); "
               "h_matImg = imshow(zeros(%d,%d)); "
               "title(sprintf('Mat Camera (pixPerMM=%%.1f)', pixPerMM));",
               headCamInfo->nrows,
               headCamInfo->ncols,
               matCamInfo->nrows,
               matCamInfo->ncols);
      
      engEvalString(matlabEngine_, cmd);
#endif // DISPLAY_MATLAB_IMAGES
      
#endif // USING_MATLAB_VISION
      
      isInitialized_ = true;
      return EXIT_SUCCESS;
    }
    
    
    bool VisionSystem::IsInitialized()
    {
      return isInitialized_;
    }
    
    void VisionSystem::Destroy()
    {
#if USING_MATLAB_VISION
      if(matlabEngine_ != NULL) {
        engClose(matlabEngine_);
      }
#endif
    }
    
    
    ReturnCode VisionSystem::lookForBlocks()
    {
      u32 numBlocks = 0;
      
      const u8 *image = getHeadCamImage();
      const s32 nrows = headCamInfo_->nrows;
      const s32 ncols = headCamInfo_->ncols;
      
#if defined(USE_MATLAB_FOR_HEAD_CAMERA)
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);
      
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
        
        fprintf(stdout, "Found %d block markers.\n", numMarkers);
        
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
          
          fprintf(stdout, "Sending ObservedBlockMarker message: Block %d, Face %d "
                  "at [(%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)] with "
                  "upDirection=%d\n",
                  msg.blockType, msg.faceType,
                  msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
                  msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
                  msg.x_imgUpperRight, msg.y_imgUpperRight,
                  msg.x_imgLowerRight, msg.y_imgLowerRight,
                  msg.upDirection);
          
          blockMarkerMailbox_->putMessage(msg);
          
          ++numBlocks;
          
        } // FOR each block Marker
        
      } // IF any blockMarkers found
      
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      // TODO: Hook this up to Pete's vision code
      fprintf(stderr, "Robot::processHeadImage(): embedded vision "
              "processing not hooked up yet.\n");
      return EXIT_FAILURE;
      
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      return EXIT_SUCCESS;
      
    } // lookForBlocks()
    
    
    ReturnCode VisionSystem::localizeWithMat()
    {
      ReturnCode retVal = -1;
      
      const u8 *image = getMatCamImage();
      const int nrows = matCamInfo_->nrows;
      const int ncols = matCamInfo_->ncols;
      
#if defined(USE_MATLAB_FOR_MAT_CAMERA)
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);
      
      if(mxImg != NULL) {
        // Display Mat Image in Matlab
        engPutVariable(matlabEngine_, "matCamImage", mxImg);
        engEvalString(matlabEngine_, "matCamImage = matCamImage(:,:,[3 2 1]);");
#if DISPLAY_MATLAB_IMAGES
        engEvalString(matlabEngine_, "set(h_matImg, 'CData', matCamImage);");
#endif
        
        // Detect MatMarker
        /*
         [xMat, yMat, orient] = matLocalization(this.matImage, ...
         'pixPerMM', pixPerMM, 'camera', this.robot.matCamera, ...
         'matSize', world.matSize, 'zDirection', world.zDirection, ...
         'embeddedConversions', this.robot.embeddedConversions);
         
         % Set the pose based on the result of the matLocalization
         this.pose = Pose(orient*[0 0 -1], ...
         [xMat yMat this.robot.appearance.WheelRadius]);
         this.pose.name = 'ObservationPose';
         */
        
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
          
          fprintf(stdout, "Sending ObservedMatMarker message: Square (%d,%d) "
                  "at (%.1f,%.1f) with orientation %.1f degrees and upDirection=%d\n",
                  msg.x_MatSquare, msg.y_MatSquare,
                  msg.x_imgCenter, msg.y_imgCenter,
                  msg.angle * (180.f/M_PI), msg.upDirection);
          
          matMarkerMailbox_->putMessage(msg);
          
        } else {
          fprintf(stdout, "No valid MatMarker found!\n");
          
        } // if marker is valid
        
        retVal = EXIT_SUCCESS;
        
      } else {
        fprintf(stderr, "Robot::processHeadImage(): could not convert image to mxArray.");
      }
      
#else  // NOT defined(USE_MATLAB_FOR_MAT_CAMERA)
      
      // TODO: Hook this up to Pete's vision code
      fprintf(stderr, "Robot::processMatImage(): embedded vision "
              "processing not hooked up yet.\n");
      retVal = -1;
      
#endif // defined(USE_MATLAB_FOR_MAT_CAMERA)
      
      return retVal;
      
    } // localizeWithMat()
    
    
    
  } // namespace Cozmo
} // namespace Anki