#include <mex.h>

#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/common/matlab/mexWrappers.h"

using namespace Anki;

static Vector::VisionSystem* visionSystem = nullptr;

void Clear()
{
  mexPrintf("Clearing vision system.");
  if(visionSystem != nullptr) {
    delete visionSystem;
  }
}

Result Update(const mxArray* mxImg)
{
  cv::Mat_<u8> cvImg;
  mxArray2cvMatScalar(mxImg, cvImg);
  
  // Using bogus robot state
  Vector::MessageRobotState robotState;
  
  Vision::Image img(cvImg);
  
  // Make sure parameters match the current image's resolution
  // NOTE: Just putting in bogus focal length parameters here since all we really
  //  care about for detection is the image size
  Vision::CameraCalibration camCalib(img.GetNumRows(), img.GetNumCols(),
                                     1.f, 1.f,
                                     img.GetNumCols()/2, img.GetNumRows()/2);
  
  visionSystem->Init(camCalib);
  
  return visionSystem->Update(robotState, img);
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  mexAtExit(Clear);
  
  const s32 commandStrLen = 32;
  char commandStrBuf[commandStrLen];
  mxGetString(prhs[0], commandStrBuf, commandStrLen);
  
  const std::string command(commandStrBuf);
  
  if(command == "Init") {
    const s32 modelPathLen = 1024;
    char modelPathBuf[modelPathLen];
    mxGetString(prhs[1], modelPathBuf, modelPathLen);
    
    Clear();
    visionSystem = new Vector::VisionSystem(modelPathBuf);
    
    mexMakeMemoryPersistent(visionSystem);

    mexPrintf("Constructed new vision system.\n");
  }
  else if(!visionSystem) {
    mexErrMsgTxt("VisionSystem not initialized. Call with 'Init' first.");
  }
  else if(command == "DetectMarkers"){

    visionSystem->StartMarkerDetection();
    
    Result result = Update(prhs[1]);
    
    visionSystem->StopMarkerDetection();
    
    if(RESULT_OK != result) {
      mexErrMsgTxt("VisionSystem::Update() failed.\n");
    }
    
    std::vector<Vision::ObservedMarker> obsMarkers;
    
    // Check the mailbox for any markers
    Vision::ObservedMarker obsMarker;
    while(visionSystem->CheckMailbox(obsMarker)) {
      obsMarkers.emplace_back(obsMarker);
    }
    mexPrintf("Found %lu markers.\n", obsMarkers.size());
    
    plhs[0] = mxCreateCellMatrix(1, obsMarkers.size());
    for(s32 i=0; i<obsMarkers.size(); ++i) {
      mxArray* mxQuad = mxCreateDoubleMatrix(4, 2, mxREAL);
      double *x = mxGetPr(mxQuad);
      double *y = x + 4;

      const Quad2f& corners = obsMarkers[i].GetImageCorners();
      for(int iCorner=0; iCorner<4; ++iCorner) {
        Quad::CornerName cornerName = static_cast<Quad::CornerName>(iCorner);
        x[iCorner] = corners[cornerName].x();
        y[iCorner] = corners[cornerName].y();
      }
      mxSetCell(plhs[0], i, mxQuad);
    }
  }
  else if(command == "DetectFaces") {
    visionSystem->StartDetectingFaces();
    
    Result result = Update(prhs[1]);
    
    visionSystem->StopDetectingFaces();
    
    if(RESULT_OK != result) {
      mexErrMsgTxt("VisionSystem::Update() failed.\n");
    }
  }

  
  
}