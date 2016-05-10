// Mex wrapper for Contrast Limited Adaptive Histogram Equalization (CLAHE)
//
// Andrew Stein
// 05-09-2016
//

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "mex.h"
#include "anki/common/types.h"
#include "anki/common/matlab/mexWrappers.h"

//int main(int argc, const char** argv)
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

  if(! (nlhs == 1 && nrhs == 3))
  {
    mexErrMsgTxt("Usage: img_eq = mexCLAHE(img, clipLimit, tileFraction);\n");
    return;
  }
  
  cv::Mat grayscaleFrame;
  mxArray2cvMat(prhs[0], grayscaleFrame);

  if(mxGetDimensions(prhs[0])[3] > 1) {
    cv::cvtColor(grayscaleFrame, grayscaleFrame, CV_RGB2GRAY);
  }

  const f32 clipLimit = mxGetScalar(prhs[1]);
  const f32 tileSize  = mxGetScalar(prhs[2]);

  cv::Size gridSize;
  if(tileSize < 1.f) {
    gridSize = cv::Size(tileSize*(f32)grayscaleFrame.cols,
                        tileSize*(f32)grayscaleFrame.rows);
  } else {
    gridSize = cv::Size(tileSize, tileSize);
  }
  
  mexPrintf("Using clipLimit=%.1f and gridSize=(%d,%d)\n",
            clipLimit, gridSize.width, gridSize.height);
  
  cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(clipLimit, gridSize);
  
  clahe->apply(grayscaleFrame, grayscaleFrame);

  plhs[0] = cvMat2mxArray(grayscaleFrame);

  return;
}





