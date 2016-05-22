#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "anki/common/matlab/mexWrappers.h"

#ifdef _MSC_VER
#include<ciso646>
#endif

#define VERBOSITY 0

std::vector<cv::VideoCapture *> capture;

void closeHelper(size_t device) {
  if(capture[device] != NULL) {
    //mexPrintf("Closing VideoCapture camera.\n");
    capture[device]->release();
    delete capture[device];
    capture[device] = NULL;
  }
}

void closeHelper(void) {
  size_t numDevices = capture.size();
  for(size_t i=0; i<numDevices; ++i)
  {
    closeHelper(i);
  }
}

enum Command {
  OPEN  = 0,
  GRAB  = 1,
  CLOSE = 2,
  CHANGE_RESOLUTION = 3
};

// A Simple Camera Capture Framework
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  const int MAX_READ_TRIES = 50;

  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  mexAtExit(closeHelper);

  mxAssert(nrhs >= 1, "At least one input required.");

  Command   cmd    = static_cast<Command>(static_cast<int>(floor(mxGetScalar(prhs[0])+0.5)));
  int       device = -1;

  switch(cmd)
  {
  case OPEN:
    {
      if(nrhs < 2) {
        mexErrMsgTxt("You must specify a device with the OPEN command.");
      }

      device = (int) mxGetScalar(prhs[1]);

      if(device >= capture.size()) {
        // Make room for new capture device:
        capture.resize(device+1, NULL);
      }

      // Close device (if necessary) before reopening it:
      closeHelper(device);

      DEBUG_MSG(0, "Opening capture device %d.\n", device);
      capture[device] = new cv::VideoCapture(device);

      if(nrhs >= 3) {
        double width  = mxGetScalar(prhs[2]);
        double height = mxGetScalar(prhs[3]);

        capture[device]->set(CV_CAP_PROP_CONVERT_RGB, 0.);

        if(not capture[device]->set(CV_CAP_PROP_FRAME_WIDTH, width) ||
          not capture[device]->set(CV_CAP_PROP_FRAME_HEIGHT, height))
        {
          mexErrMsgTxt("Failed to adjust capture dimensions.");
        }

        DEBUG_MSG(0, "Set camera to capture at %.0fx%.0f\n",
          width, height);
      }

      DEBUG_MSG(1, "Initialization pause...");
      mexEvalString("pause(2)");
      DEBUG_MSG(1, "Done.\n");

      if(nlhs>0) {
        // If an output was requested with an OPEN command, trigger
        // a grab to happen below
        cmd = GRAB;
      }
      break;
    }

  case CLOSE:
    {
      if(nrhs > 1) {
        device = (int) mxGetScalar(prhs[1]);
      }

      if(device < 0) {
        // Close all devices
        closeHelper();
      }
      else {
        // Close specified device:
        if(device < capture.size() && capture[device] != NULL) {
          DEBUG_MSG(0, "Closing capture device %d.\n", device);
          capture[device]->release();
          delete capture[device];
          capture[device] = NULL;
        }
      }
      break;
    }

  case GRAB:
    {
      if(nrhs < 2) {
        mexErrMsgTxt("You must specify a device with the GRAB command.");
      }

      device = (int) mxGetScalar(prhs[1]);

      if(nlhs==0) {
        mexErrMsgTxt("No output argument provided for GRAB.");
      }
      break;
    }

  case CHANGE_RESOLUTION:
    {
      if(nrhs < 4) {
        mexErrMsgTxt("You must supply device and resolution arguments "
          "with the CHANGE_RESOLUTION command.");
      }

      device = (int) mxGetScalar(prhs[1]);
      if(device < 0 || device >= capture.size()) {
        mexErrMsgTxt("Invalid device specified.");
      }

      double width  = mxGetScalar(prhs[2]);
      double height = mxGetScalar(prhs[3]);

      if(not capture[device]->set(CV_CAP_PROP_FRAME_WIDTH, width) ||
        not capture[device]->set(CV_CAP_PROP_FRAME_HEIGHT, height))
      {
        mexErrMsgTxt("Failed to adjust capture dimensions.");
      }

      DEBUG_MSG(0, "Set camera to capture at %.0fx%.0f\n",
        width, height);
      break;
    }

  default:
    mexErrMsgTxt("Unknown command.");
  } // SWITCH(cmd)

  if(cmd == GRAB)
  {
    mxAssert(nlhs > 0, "Output argument required for a GRAB.");

    if(capture.empty() || capture[device] == NULL || not capture[device]->isOpened()) {
      mexErrMsgTxt("No camera open.");
    }

    cv::Mat frame;
    bool success = false;
    int iterations = 0;
    while(not success && iterations++ < MAX_READ_TRIES) {
      success = capture[device]->read(frame);
    }

    if(not success) {
      mexWarnMsgTxt("Frame grab failed!");
    }
    if(frame.empty()) {
      mexWarnMsgTxt("Empty frame grabbed!");
      success = false;
    }

    if(not success)
    {
      double width = capture[device]->get(CV_CAP_PROP_FRAME_WIDTH);
      double height = capture[device]->get(CV_CAP_PROP_FRAME_HEIGHT);

      mwSize dims[3] = {static_cast<mwSize>(height), static_cast<mwSize>(width), 3};
      plhs[0] = mxCreateNumericArray(3, dims, mxDOUBLE_CLASS, mxREAL);
    }
    else {
      //cv::cvtColor(frame, frame, CV_BGR2RGB);
      DEBUG_MSG(2, "Captured %dx%d frame.\n", frame.cols, frame.rows);
      plhs[0] = cvMat2mxArray(frame);
    }
  } // IF(cmd==GRAB)
} // mexFunction()