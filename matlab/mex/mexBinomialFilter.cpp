#include "mexWrappers.h"

#include "anki/common.h"
#include "anki/vision.h"

#define VERBOSITY 0

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    Anki::Matrix<u8> img = mxArray2AnkiMatrix<u8>(prhs[0]);

    Anki::Matrix<u8> filteredImg = Anki::AllocateMatrixFromHeap<u8>(img.get_size(0), img.get_size(1));
    
    printf("%d %d\n", *img.Pointer(10,10), *img.Pointer(100,10));
    
    /*
    double sigma = mxGetScalar(prhs[1]);
    double numSigma = mxGetScalar(prhs[2]);
    int k = 2*int(std::ceil(double(numSigma)*sigma)) + 1;
    cv::Size ksize(k,k); 
    
    cv::GaussianBlur(src, dst, ksize, sigma, sigma, cv::BORDER_REFLECT_101);
            
    plhs[0] = cvMat2mxArray(dst);*/
}