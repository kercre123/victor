#include "opencv2/opencv.hpp"
#include "mexWrappers.h"

#define VERBOSITY 0

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    cv::Mat src, dst;
    mxArray2cvMat(prhs[0], src);
    
    double sigma = mxGetScalar(prhs[1]);
    double numSigma = mxGetScalar(prhs[2]);
    int k = 2*int(std::ceil(double(numSigma)*sigma)) + 1;
    cv::Size ksize(k,k); 
    
    cv::GaussianBlur(src, dst, ksize, sigma, sigma, cv::BORDER_REFLECT_101);
            
    plhs[0] = cvMat2mxArray(dst);
            
}