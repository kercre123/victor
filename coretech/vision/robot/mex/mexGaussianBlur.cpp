#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "anki/common/matlab/mexWrappers.h"

#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  cv::Mat src, dst;
  mxArray2cvMat(prhs[0], src);

  double sigma = mxGetScalar(prhs[1]);
  double numSigma = mxGetScalar(prhs[2]);
  int k = 2*int(std::ceil(double(numSigma)*sigma)) + 1;
  cv::Size ksize(k,k);

  cv::GaussianBlur(src, dst, ksize, sigma, sigma, cv::BORDER_REFLECT_101);

  plhs[0] = cvMat2mxArray(dst);
}