// function [homography] = mex_cp2tform_projective(input_points, base_points)

#include "opencv2/opencv.hpp"
#include "mexWrappers.h"

#define VERBOSITY 0

using namespace cv;

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  const mwSize array0_numDimensions = mxGetNumberOfDimensions(prhs[0]);
  const mwSize *array0_dimensions = mxGetDimensions(prhs[0]);
  const mwSize array1_numDimensions = mxGetNumberOfDimensions(prhs[1]);
  const mwSize *array1_dimensions = mxGetDimensions(prhs[1]);

  std::vector<cv::Point_<double> > input_points = mxArray2CvPointVector<double>(prhs[0]);
  std::vector<cv::Point_<double> > base_points = mxArray2CvPointVector<double>(prhs[1]);

  ConditionalErrorAndReturn(input_points.size()!=0 || base_points.size()!=0, "mex_cp2tform_projective", "input points or base points couln't be parsed");
  
  ConditionalErrorAndReturn(input_points.size() == base_points.size(), "mex_cp2tform_projective", "Point lists are of different sizes");

//  std::cout << "input_points: " << input_points << "\n";
//  std::cout << "base_points: " << base_points << "\n";
    
  Mat homography = findHomography( input_points, base_points, 0 );

  plhs[0] = cvMat2mxArray(homography);           
}
