#include <mex.h>

#include "../../basestation/src/perspectivePoseEstimation_impl.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  using namespace Anki;
  
  // Inputs:
  //  0 - image corners: [4x2] matrix
  //  1 - world corners: [4x3] matrix
  //  2 - focal length:  [1x2] vector
  //  3 - camera center: [1x2] vector
  //  4 - camera dims:   [1x2] vector (nrows,ncols)

  // Output:
  //  0 - Rotation:      [3x3] matrix
  //  1 - Translation:   [3x1] vector
  
  mxAssert(mxGetClassID(prhs[0]) == mxDOUBLE_CLASS,
           "Image corners matrix should be DOUBLE.");
  mxAssert(mxGetM(prhs[0])==4 && mxGetN(prhs[0])==2,
           "Image corners matrix should be 4x2.");
  const double* mxImgCorners = mxGetPr(prhs[0]);
  
  mxAssert(mxGetClassID(prhs[1]) == mxDOUBLE_CLASS,
           "World corners matrix should be DOUBLE.");
  mxAssert(mxGetM(prhs[1])==4 && mxGetN(prhs[1])==3,
           "World corners matrix should be 4x3.");
  const double* mxWorldCorners = mxGetPr(prhs[1]);
  
  mxAssert(mxGetClassID(prhs[2]) == mxDOUBLE_CLASS,
           "Focal length vector should be DOUBLE.");
  mxAssert(mxGetNumberOfElements(prhs[2])==2,
           "Focal length vector should have two elements.");
  const double* mxFocalLength = mxGetPr(prhs[2]);
  
  mxAssert(mxGetClassID(prhs[3]) == mxDOUBLE_CLASS,
           "Camera center vector should be DOUBLE.");
  mxAssert(mxGetNumberOfElements(prhs[3])==2,
           "Camera center vector should have two elements.");
  const double* mxCenter = mxGetPr(prhs[3]);
  
  mxAssert(mxGetClassID(prhs[4]) == mxDOUBLE_CLASS,
           "Camera dimensions vector should be DOUBLE.");
  mxAssert(mxGetNumberOfElements(prhs[4])==2,
           "Camera dimensions vector should have two elements.");
  const double* mxDims = mxGetPr(prhs[4]);
  
  Vision::CameraCalibration calib(static_cast<u16>(mxDims[0]),
                                  static_cast<u16>(mxDims[1]),
                                  static_cast<f32>(mxFocalLength[0]),
                                  static_cast<f32>(mxFocalLength[1]),
                                  static_cast<f32>(mxCenter[0]),
                                  static_cast<f32>(mxCenter[1]));

  Quadrilateral<2, double> imgQuad(Point<2,double>(mxImgCorners[0],mxImgCorners[4]),
                                   Point<2,double>(mxImgCorners[1],mxImgCorners[5]),
                                   Point<2,double>(mxImgCorners[2],mxImgCorners[6]),
                                   Point<2,double>(mxImgCorners[3],mxImgCorners[7]));
  
  Quadrilateral<3, double> worldQuad(Point<3,double>(mxWorldCorners[0],mxWorldCorners[4],mxWorldCorners[8]),
                                     Point<3,double>(mxWorldCorners[1],mxWorldCorners[5],mxWorldCorners[9]),
                                     Point<3,double>(mxWorldCorners[2],mxWorldCorners[6],mxWorldCorners[10]),
                                     Point<3,double>(mxWorldCorners[3],mxWorldCorners[7],mxWorldCorners[11]));
  
  Pose3d pose;
  Vision::P3P::computePose<double,double>(imgQuad, worldQuad, calib, pose);
  
  
  plhs[0] = mxCreateDoubleMatrix(3, 3, mxREAL);
  double* R = mxGetPr(plhs[0]);
  
  plhs[1] = mxCreateDoubleMatrix(3, 1, mxREAL);
  double* T = mxGetPr(plhs[1]);
  
  mwSize index=0;
  for(mwSize j=0; j<3; ++j) {
    T[j] = static_cast<double>(pose.get_translation()[j]);
    
    for(mwSize i=0; i<3; ++i, ++index) {
      R[index] = static_cast<double>(pose.get_rotationMatrix()(i,j));
    }
  }
  
}