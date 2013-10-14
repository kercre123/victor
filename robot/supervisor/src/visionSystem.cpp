#include "anki/cozmo/robot/visionSystem.h"


#include <iostream>

#define USING_MATLAB_VISION (defined(USE_MATLAB_FOR_HEAD_CAMERA) || \
                             defined(USE_MATLAB_FOR_MAT_CAMERA))

#if USING_MATLAB_VISION
// If using Matlab for any vision processing, enable the Matlab engine
#include "engine.h"
#include "anki/common/robot/matlabInterface.h"
#endif

namespace Anki {
  namespace Cozmo {

#pragma mark --- VisionSystem "Member Variables" ---
    
    namespace VisionSystem {
      typedef struct {
        
        bool isInitialized;
        
        HardwareInterface::FrameGrabber headCamFrameGrabber;
        HardwareInterface::FrameGrabber matCamFrameGrabber;
        
        const HardwareInterface::CameraInfo* headCamInfo;
        const HardwareInterface::CameraInfo* matCamInfo;
        
        BlockMarkerMailbox* blockMarkerMailbox;
        MatMarkerMailbox*   matMarkerMailbox;
        
#if USING_MATLAB_VISION
        Engine *matlabEngine_;
#endif
        
      } Members;
      
      // The actual static variable holding our members, initialized to
      // the default values:
      static Members this_ = {
        .isInitialized = false,
        .headCamFrameGrabber = NULL,
        .matCamFrameGrabber  = NULL,
        .headCamInfo = NULL,
        .matCamInfo  = NULL,
        .blockMarkerMailbox = NULL,
        .matMarkerMailbox  = NULL
      };
      
      
      inline const u8* getHeadCamImage()
      {
        assert(this_.headCamFrameGrabber != NULL);
        assert(this_.isInitialized);
        return this_.headCamFrameGrabber();
      }
      
      inline const u8* getMatCamImage()
      {
        assert(this_.matCamFrameGrabber != NULL);
        assert(this_.isInitialized);
        return this_.matCamFrameGrabber();
      }
      
    } // namespace VisionSystem
    
    
#pragma mark --- VisionSystem::Mailbox Implementations ---
    

    template<>
    void VisionSystem::MatMarkerMailbox::advanceIndex(u8 &index)
    {
      return;
    }
    
#pragma mark --- VisionSystem Method Implementations ---
    
    
    ReturnCode VisionSystem::Init(HardwareInterface::FrameGrabber      headCamFrameGrabberIn,
                                  HardwareInterface::FrameGrabber      matCamFrameGrabberIn,
                                  const HardwareInterface::CameraInfo* headCamInfoIn,
                                  const HardwareInterface::CameraInfo* matCamInfoIn,
                                  BlockMarkerMailbox*                  blockMarkerMailboxIn,
                                  MatMarkerMailbox*                    matMarkerMailboxIn)
    {
      this_.isInitialized = false;
      
      if(headCamFrameGrabberIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - HeadCam FrameGrabber is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.headCamFrameGrabber = headCamFrameGrabberIn;
      
      if(matCamFrameGrabberIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatCam FrameGrabber is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.matCamFrameGrabber  = matCamFrameGrabberIn;
      
      if(headCamInfoIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - HeadCam Info pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.headCamInfo = headCamInfoIn;
      
      if(matCamInfoIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatCam Info pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.matCamInfo  = matCamInfoIn;
      
      if(blockMarkerMailboxIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - BlockMarkerMailbox pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.blockMarkerMailbox = blockMarkerMailboxIn;
      
      if(matMarkerMailboxIn == NULL) {
        fprintf(stdout, "VisionSystem::Init() - MatMarkerMailbox pointer is NULL!\n");
        return EXIT_FAILURE;
      }
      this_.matMarkerMailbox = matMarkerMailboxIn;
      
#if USING_MATLAB_VISION
      
      this_.matlabEngine_ = NULL;
      if (!(this_.matlabEngine_ = engOpen(""))) {
        fprintf(stdout, "\nCan't start MATLAB engine!\n");
        return EXIT_FAILURE;
      }
      
      engEvalString(this_.matlabEngine_, "run('../../../../matlab/initCozmoPath.m');");
      
      // TODO: Pass camera calibration data back to Matlab
      
      char cmd[256];
      snprintf(cmd, 255, "h_imgFig = figure; "
               "subplot(121); "
               "h_headImg = imshow(zeros(%d,%d)); "
               "title('Head Camera'); "
               "subplot(122); "
               "h_matImg = imshow(zeros(%d,%d)); "
               "title('Mat Camera');",
               this_.headCamInfo->nrows,
               this_.headCamInfo->ncols,
               this_.matCamInfo->nrows,
               this_.matCamInfo->ncols);
      
      engEvalString(this_.matlabEngine_, cmd);
      
#endif // USING_MATLAB_VISION
      
      this_.isInitialized = true;
      return EXIT_SUCCESS;
    }
    
    
    bool VisionSystem::IsInitialized()
    {
      return this_.isInitialized;
    }
    
    void VisionSystem::Destroy()
    {
#if USING_MATLAB_VISION
      if(this_.matlabEngine_ != NULL) {
        engClose(this_.matlabEngine_);
      }
#endif
    }
    
    
    ReturnCode VisionSystem::lookForBlocks()
    {
      u32 numBlocks = 0;
      
      const u8 *image = getHeadCamImage();
      const s32 nrows = this_.headCamInfo->nrows;
      const s32 ncols = this_.headCamInfo->ncols;
      
#if defined(USE_MATLAB_FOR_HEAD_CAMERA)
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);
      
      if(mxImg != NULL) {
        // Send the image to matlab
        engPutVariable(this_.matlabEngine_, "headCamImage", mxImg);
        
        // Convert from GBRA format to RGB:
        engEvalString(this_.matlabEngine_, "headCamImage = headCamImage(:,:,[3 2 1]);");
        
        // Display (optional)
        engEvalString(this_.matlabEngine_, "set(h_headImg, 'CData', headCamImage);");
        
        // Detect BlockMarkers
        engEvalString(this_.matlabEngine_, "blockMarkers = simpleDetector(headCamImage); "
                      "numMarkers = length(blockMarkers);");
        
        int numMarkers = static_cast<int>(mxGetScalar(engGetVariable(this_.matlabEngine_, "numMarkers")));
        
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
                   "corners   = currentMarker.corners;", i_marker+1);
          
          engEvalString(this_.matlabEngine_, cmd);
          
          
          // Create a message from those pieces
          
          CozmoMsg_ObservedBlockMarker msg;
          
          mxArray *mxBlockType = engGetVariable(this_.matlabEngine_, "blockType");
          msg.blockType = static_cast<u32>(mxGetScalar(mxBlockType));
          
          mxArray *mxFaceType = engGetVariable(this_.matlabEngine_, "faceType");
          msg.faceType = static_cast<u32>(mxGetScalar(mxFaceType));
          
          mxArray *mxCorners = engGetVariable(this_.matlabEngine_, "corners");
          
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
          
          fprintf(stdout, "Sending ObsevedBlockMarker message: Block %d, Face %d "
                  "at [(%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f) (%.1f,%.1f)]\n",
                  msg.blockType, msg.faceType,
                  msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
                  msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
                  msg.x_imgUpperRight, msg.y_imgUpperRight,
                  msg.x_imgLowerRight, msg.y_imgLowerRight);
          
          this_.blockMarkerMailbox->putMessage(msg);
          
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
      const int nrows = this_.matCamInfo->nrows;
      const int ncols = this_.matCamInfo->ncols;
      
#if defined(USE_MATLAB_FOR_MAT_CAMERA)
      mxArray *mxImg = Anki::Embedded::imageArrayToMxArray(image, nrows, ncols, 4);
      
      if(mxImg != NULL) {
        engPutVariable(this_.matlabEngine_, "matCamImage", mxImg);
        engEvalString(this_.matlabEngine_, "matCamImage = matCamImage(:,:,[3 2 1]);");
        engEvalString(this_.matlabEngine_, "set(h_matImg, 'CData', matCamImage);");
        
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