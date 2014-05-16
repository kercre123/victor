// function [homography] = mex_cp2tform_projective(input_points, base_points)

#include "opencv2/core/core.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/robot/matlabInterface.h"

#include "anki/common/shared/utilities_shared.h"

#define VERBOSITY 0

using namespace cv;
using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  // TODO: Remove these if they aren't needed
  /* Unused
  const mwSize array0_numDimensions = mxGetNumberOfDimensions(prhs[0]);
  const mwSize *array0_dimensions = mxGetDimensions(prhs[0]);
  const mwSize array1_numDimensions = mxGetNumberOfDimensions(prhs[1]);
  const mwSize *array1_dimensions = mxGetDimensions(prhs[1]);
  */
  std::vector<cv::Point_<double> > input_points = mxArray2CvPointVector<double>(prhs[0]);
  std::vector<cv::Point_<double> > base_points = mxArray2CvPointVector<double>(prhs[1]);

  AnkiConditionalErrorAndReturn(input_points.size()!=0 || base_points.size()!=0, "mex_cp2tform_projective", "input points or base points couldn't be parsed");

  AnkiConditionalErrorAndReturn(input_points.size() == base_points.size(), "mex_cp2tform_projective", "Point lists are of different sizes");

  //  std::cout << "input_points: " << input_points << "\n";
  //  std::cout << "base_points: " << base_points << "\n";

  Mat homography = findHomography( input_points, base_points, 0 );

  plhs[0] = cvMat2mxArray(homography);
}