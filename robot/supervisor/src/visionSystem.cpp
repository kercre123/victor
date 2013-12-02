#include "anki/common/robot/config.h"
#include "anki/cozmo/robot/visionSystem.h"
#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/cozmo/robot/cozmoBot.h"

#include "anki/cozmo/messageProtocol.h"
#include "anki/common/shared/radians.h"


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
    
#pragma mark --- VisionSystem "Private Member Variables" ---
    
    // Private "Members" of the VisionSystem:
    namespace {
      
      bool isInitialized_ = false;
      
      const HAL::CameraInfo* headCamInfo_ = NULL;
      const HAL::CameraInfo* matCamInfo_  = NULL;
      
      // Image buffers (Only used when using MATLAB)
#ifdef USE_MATLAB_FOR_HEAD_CAMERA
      u8 headCamImage_[640*480*4];
#endif
#ifdef USE_MATLAB_FOR_MAT_CAMERA
      u8 matCamImage_[640*480*4];
#endif
      
      // Whether or not we're in the process of waiting for an image to be acquired
      bool acquiringHeadCamImage_ = false;
      bool acquiringMatCamImage_ = false;
    
      
      VisionSystem::BlockMarkerMailbox* blockMarkerMailbox_ = NULL;
      VisionSystem::MatMarkerMailbox*   matMarkerMailbox_   = NULL;
      
      f32 matCamPixPerMM_ = 1.f;
      
      // Window within which to search for docking target, specified by
      // upper left corner + width and height
      s16 dockTargetWinX_ = -1;
      s16 dockTargetWinY_ = -1;
      s16 dockTargetWinW_ = -1;
      s16 dockTargetWinH_ = -1;
      
    } // private namespace VisionSystem
    
    
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
    
    
    ReturnCode VisionSystem::Init(const HAL::CameraInfo* headCamInfo,
                                  const HAL::CameraInfo* matCamInfo,
                                  BlockMarkerMailbox*    blockMarkerMailbox,
                                  MatMarkerMailbox*      matMarkerMailbox)
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
      
      // Compute the resolution of the mat camera from its FOV and height
      // off the mat:
      f32 matCamHeightInPix = ((static_cast<f32>(matCamInfo_->nrows)*.5f) /
                               tanf(matCamInfo_->fov_ver * .5f));
      matCamPixPerMM_ = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
      
      
#if USING_MATLAB_VISION
      
      if(matlabEngine_ == NULL) {
        PRINT("Expecting Matlab engine to already be open.\n");
        return EXIT_FAILURE;
      }
      
      // Initialize Matlab
      engEvalString(matlabEngine_, "run('../../../../matlab/initCozmoPath.m');");
      
      engEvalString(matlabEngine_, "usingOutsideSquare = BlockMarker2D.UseOutsideOfSquare;");
      mxArray *mxUseOutsideOfSquare = engGetVariable(matlabEngine_, "usingOutsideSquare");
      if(mxIsLogicalScalarTrue(mxUseOutsideOfSquare) != BLOCKMARKER3D_USE_OUTSIDE_SQUARE) {
        PRINT("UseOutsideOfSquare settings between Matlab and C++ don't match!\n");
        return EXIT_FAILURE;
      }
      
      // Store computed pixPerMM in Matlab for use by MatLocalization()
      engPutVariable(matlabEngine_, "pixPerMM",
                     mxCreateDoubleScalar(matCamPixPerMM_));
      
#if DISPLAY_MATLAB_IMAGES
      char cmd[256];
      snprintf(cmd, 255, "h_imgFig = figure; "
               "subplot(121); "
               "h_headImg = imshow(zeros(%d,%d)); "
               "hold('on'); "
               "h_dockTargetWin = plot(nan, nan, 'r'); "
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
#if defined(USE_MATLAB_FOR_HEAD_CAMERA)
      u32 numBlocks = 0;
      
      // Start image capture
      if (!acquiringHeadCamImage_) {
        HAL::CameraStartFrame(HAL::CAMERA_FRONT, headCamImage_, HAL::CAMERA_MODE_VGA, HAL::CAMERA_UPDATE_SINGLE, 100, true);
        acquiringHeadCamImage_ = true;
      }
      
      // Wait until frame is ready
      if (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT)) {
        return EXIT_SUCCESS;
      }
      acquiringHeadCamImage_ = false;
      
      
      const s32 nrows = headCamInfo_->nrows;
      const s32 ncols = headCamInfo_->ncols;
      
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(headCamImage_, nrows, ncols, 4);
      
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
          
          ++numBlocks;
          
        } // FOR each block Marker
        
      } // IF any blockMarkers found
      
#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      // TODO: Hook this up to Pete's vision code
      PRINT("Robot::processHeadImage(): embedded vision "
              "processing not hooked up yet.\n");
      return EXIT_FAILURE;
      
#endif // defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      return EXIT_SUCCESS;
      
    } // lookForBlocks()
    
    
    ReturnCode VisionSystem::localizeWithMat()
    {
      ReturnCode retVal = -1;
      
#if defined(USE_MATLAB_FOR_MAT_CAMERA)
      
      // Start image capture
      if (!acquiringMatCamImage_) {
        HAL::CameraStartFrame(HAL::CAMERA_MAT, matCamImage_, HAL::CAMERA_MODE_VGA, HAL::CAMERA_UPDATE_SINGLE, 100, true);
        acquiringMatCamImage_ = true;
      }
      
      // Wait until frame is ready
      if (!HAL::CameraIsEndOfFrame(HAL::CAMERA_MAT)) {
        return EXIT_SUCCESS;
      }
      acquiringMatCamImage_ = false;

      const int nrows = matCamInfo_->nrows;
      const int ncols = matCamInfo_->ncols;
      
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(matCamImage_, nrows, ncols, 4);
      
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
      
#else  // NOT defined(USE_MATLAB_FOR_MAT_CAMERA)
      
      // TODO: Hook this up to Pete's vision code
      PRINT("Robot::processMatImage(): embedded vision "
              "processing not hooked up yet.\n");
      retVal = -1;
      
#endif // defined(USE_MATLAB_FOR_MAT_CAMERA)
      
      return retVal;
      
    } // localizeWithMat()
    
    ReturnCode VisionSystem::setDockingWindow(const s16 xLeft, const s16 yTop,
                                              const s16 width, const s16 height)
    {
      ReturnCode retVal = EXIT_SUCCESS;
      
      if(xLeft >= 0 && yTop >= 0 &&
         (xLeft + width)<headCamInfo_->ncols &&
         (yTop + height)<headCamInfo_->nrows)
      {
        dockTargetWinX_ = xLeft;
        dockTargetWinY_ = yTop;
        dockTargetWinW_ = width;
        dockTargetWinH_ = height;
      } else {
        retVal = EXIT_FAILURE;
      }
      
      return retVal;
      
    } // setDockingWindow
    
    ReturnCode VisionSystem::findDockingTarget(DockingTarget& target)
    {
      ReturnCode retVal = EXIT_SUCCESS;

#if defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      // Start image capture
      if (!acquiringHeadCamImage_) {
        HAL::CameraStartFrame(HAL::CAMERA_FRONT, headCamImage_, HAL::CAMERA_MODE_VGA, HAL::CAMERA_UPDATE_SINGLE, 100, true);
        acquiringHeadCamImage_ = true;
      }
      
      // Wait until frame is ready
      if (!HAL::CameraIsEndOfFrame(HAL::CAMERA_FRONT)) {
        return EXIT_SUCCESS;
      }
      acquiringHeadCamImage_ = false;

      const u16 nrows = matCamInfo_->nrows;
      const u16 ncols = matCamInfo_->ncols;
      
      if(dockTargetWinX_ < 0 || dockTargetWinY_ < 0 ||
         dockTargetWinW_ < 0 || dockTargetWinH_ < 0)
      {
        PRINT("No docking window set for call to findDockingTarget().\n");
        retVal = EXIT_FAILURE;
      }
      else {

        mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(headCamImage_, nrows, ncols, 4);
        
        if(mxImg != NULL) {
          
          engPutVariable(matlabEngine_, "headCamImage", mxImg);
          engEvalString(matlabEngine_, "headCamImage = headCamImage(:,:,[3 2 1]);");
          
          mxArray *mxMask = mxCreateDoubleMatrix(1,4,mxREAL);
          double *mxMaskData = mxGetPr(mxMask);
          mxMaskData[0] = static_cast<double>(dockTargetWinX_+1);
          mxMaskData[1] = static_cast<double>(dockTargetWinY_+1);
          mxMaskData[2] = static_cast<double>(dockTargetWinW_);
          mxMaskData[3] = static_cast<double>(dockTargetWinH_);
          engPutVariable(matlabEngine_, "dockTargetMaskRect", mxMask);
          
          engEvalString(matlabEngine_,
                        "uMask = dockTargetMaskRect(1)+dockTargetMaskRect(3)*[0 0 1 1]; "
                        "vMask = dockTargetMaskRect(2)+dockTargetMaskRect(4)*[0 1 0 1]; "
                        "errMsg = ''; "
                        "try, "
                        "[xDock, yDock] = findFourDotTarget(headCamImage, "
                        "   'uMask', uMask, 'vMask', vMask, "
                        "   'squareDiagonal', sqrt(sum(dockTargetMaskRect(3:4).^2)), "
                        "   'TrueSquareWidth', BlockMarker3D.CodeSquareWidth, "
                        "   'TrueDotWidth', BlockMarker3D.DockingDotWidth, "
                        "   'TrueDotSpacing', BlockMarker3D.DockingDotSpacing); "
                        "catch E, "
                        "  xDock = []; yDock = []; "
                        "  errMsg = E.message; "
                        "end");
          
#if DISPLAY_MATLAB_IMAGES
          engEvalString(matlabEngine_,
                        "set(h_headImg, 'CData', headCamImage); "
                        "set(h_dockTargetWin, 'XData', uMask([1 2 4 3 1]), "
                        "                     'YData', vMask([1 2 4 3 1]));");
#endif
          
          mxArray *mx_xDock = engGetVariable(matlabEngine_, "xDock");
          mxArray *mx_yDock = engGetVariable(matlabEngine_, "yDock");
          mxArray *mx_errMsg = engGetVariable(matlabEngine_, "errMsg");
          
          if(not mxIsEmpty(mx_errMsg)) {
            char errStr[1024];
            mxGetString(mx_errMsg, errStr, 1023);
            PRINT("Error detected running findFourDotTarget: %s\n",
                    errStr);
            retVal = EXIT_FAILURE;
          }
          else if(mxGetNumberOfElements(mx_xDock) != 4 ||
             mxGetNumberOfElements(mx_yDock) != 4)
          {
            PRINT("xDock and yDock were not 4 elements long.\n");
            retVal = EXIT_FAILURE;
          }
          else
          {
            double *mx_xDockData = mxGetPr(mx_xDock);
            double *mx_yDockData = mxGetPr(mx_yDock);
            
            // Fill in the output target with what we got from Matlab
            for(int i=0; i<4; ++i) {
              target.dotX[i] = static_cast<f32>(mx_xDockData[i]);
              target.dotY[i] = static_cast<f32>(mx_yDockData[i]);
            }
          } // if mx_xDock or mx_yDock is empty
        
          
        } else {
          
          PRINT("Robot::findDockingTarget() could not get headCamImage "
                  "for processing in Matlab.\n");
          retVal = EXIT_FAILURE;
          
        } // if mxImg != NULL
      } // if docking window set
        

      if(retVal == EXIT_SUCCESS) {
        // Update the search window based on the target we found
        f32 xcen = 0.25f*(target.dotX[0] + target.dotX[1] +
                          target.dotX[2] + target.dotX[3]);
        f32 ycen = 0.25f*(target.dotY[0] + target.dotY[1] +
                          target.dotY[2] + target.dotY[3]);
        
        f32 width = 2.f*(target.dotX[3] - target.dotX[0]);
        f32 height = 2.f*(target.dotY[3] - target.dotY[0]);
        
        if(width <= 0.f || height <= 0.f) {
          PRINT("Width/height of docking target <= 0\n");
          retVal = EXIT_FAILURE;
        } else {
          dockTargetWinX_ = xcen - 0.5f*width;
          dockTargetWinY_ = ycen - 0.5f*height;
          dockTargetWinW_ = width;
          dockTargetWinH_ = height;
        }
        
      }
      return retVal;

#else  // NOT defined(USE_MATLAB_FOR_HEAD_CAMERA)
      
      // TODO: Hook this up to Pete's vision code
      PRINT("Robot::findDockingTarget(): embedded vision "
            "processing not hooked up yet.\n");
      
      return EXIT_FAILURE;

#endif
      
    } // findDockingTarget()

    
  } // namespace Cozmo
} // namespace Anki