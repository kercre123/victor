#include "opencv2/opencv.hpp"
#include "mexWrappers.h"

#define VERBOSITY 0

cv::VideoCapture *capture = NULL;

void closeHelper(void) {
    if(capture != NULL) {
        //mexPrintf("Closing VideoCapture camera.\n");
        delete capture;
        capture = NULL;
    }
}

// A Simple Camera Capture Framework
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    mexAtExit(closeHelper);
    
    if(nrhs>0) {
        int device = (int) mxGetScalar(prhs[0]);
        
        if(device < 0) {
            if(capture != NULL) {
                DEBUG_MSG(0, "Closing capture device.\n");
                capture->release();
            }
            return;
        }
        
        DEBUG_MSG(0, "Opening capture device %d.\n", device);
        
        if(capture != NULL) {
            delete capture;
            capture = NULL;
        }
        
        capture = new cv::VideoCapture(device);
        
        /* This seems to kill Matlab...
        if(capture != NULL && capture->isOpened()) {
            mexMakeMemoryPersistent(capture);
        }
         */
            
        DEBUG_MSG(3, "Capture object pointer: %p\n", capture);
        
        if(nrhs >= 3) {
            double width  = mxGetScalar(prhs[1]);
            double height = mxGetScalar(prhs[2]);
            
            capture->set(CV_CAP_PROP_CONVERT_RGB, 0.);
            
            if(not capture->set(CV_CAP_PROP_FRAME_WIDTH, width) || 
                    not capture->set(CV_CAP_PROP_FRAME_HEIGHT, height))
            {
                mexErrMsgTxt("Failed to adjust capture dimensions.");
            }
        
            DEBUG_MSG(0, "Set camera to capture at %.0fx%.0f\n", 
                    width, height);
        }
        
        DEBUG_MSG(1, "Initialization pause...");
        mexEvalString("pause(2)");
        DEBUG_MSG(1, "Done.\n");
    }
    else if(capture == NULL || not capture->isOpened()) {
        mexErrMsgTxt("No camera open, specify a device.");
    } 
    else {
        DEBUG_MSG(3, "Using capture object pointer: %p\n", capture);
    }
    
    if(nlhs>0)
    {   
        cv::Mat frame;
        bool success = capture->read(frame);
        
        if(not success) {
            mexErrMsgTxt("Frame grab failed!");
        }
        
        if(frame.empty()) {
            mexErrMsgTxt("Empty frame grabbed!");
        }
        else {
            cv::cvtColor(frame, frame, CV_BGR2RGB);
            DEBUG_MSG(2, "Captured %dx%d frame.\n", frame.cols, frame.rows);
        }
        
        plhs[0] = cvMat2mxArray(frame);
    }
}
