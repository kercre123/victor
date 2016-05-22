#if !defined(__APPLE_CC__)
#error This file only builds in XCode
#endif

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include "debugStreamClient.h"

#define VERBOSITY 0

#define USE_OPENCV_DISPLAY 0

using namespace Anki::Embedded;

static const std::string DEFAULT_IP_PREFIX("192.168.3.");
static const s32 port = 5551;
static DebugStreamClient *parserThread = NULL; //(ipAddress, port);

void closeHelper(void) {
  if(parserThread != NULL) {
    mexPrintf("Closing parserThread.\n");

    if(parserThread->Close() != RESULT_OK) {
      mexWarnMsgTxt("Failed to close the parserThread cleanly!");
    }

    //mexPrintf("NOTE: not actually deleting the parser -- was crashing matlab.  Investigate.\n");
    delete parserThread;
    parserThread = NULL;
  }

#if USE_OPENCV_DISPLAY
  cv::destroyWindow("Robot Image");
#endif
}

enum Command {
  OPEN  = 0,
  GRAB  = 1,
  CLOSE = 2
};

// A Simple Wifi Camera Capture wrapper
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  // Necessary when using TCP (vs. UART)
#define USE_SOCKET

  mexAtExit(closeHelper);

  mxAssert(nrhs >= 1, "At least one input required.");

  Command cmd = static_cast<Command>(static_cast<int>(floor(mxGetScalar(prhs[0])+0.5)));

  switch(cmd)
  {
  case OPEN:
    {
      if(nrhs < 2) {
        mexErrMsgTxt("You must specify a device with the OPEN command.");
      }

      //printf("Starting display\n");
      SetLogSilence(true);

      std::string ipAddress(DEFAULT_IP_PREFIX);
      if(mxIsChar(prhs[1])) {
        char buffer[256];
        mxGetString(prhs[1], buffer, 256);
        ipAddress = buffer;
      }
      else {
        int device = static_cast<int>(mxGetScalar(prhs[1]));
        ipAddress += std::to_string(device);
      }

      //device = (int) mxGetScalar(prhs[1]);

      // Close device (if necessary) before reopening it:
      closeHelper();

      mexPrintf("Opening capture on IP %s.\n", ipAddress.c_str());
      parserThread = new DebugStreamClient(ipAddress.c_str(), port);

      //DEBUG_MSG(1, "Initialization pause...");
      mexEvalString("pause(1)");
      //DEBUG_MSG(1, "Done.\n");

      if(nlhs>0) {
        // If an output was requested with an OPEN command, trigger
        // a grab to happen below
        cmd = GRAB;
      }
      break;
    }

  case CLOSE:
    {
      closeHelper();
      break;
    }

  case GRAB:
    {
      if(nlhs==0) {
        mexErrMsgTxt("No output argument provided for GRAB.");
      }
      break;
    }

  default:
    mexErrMsgTxt("Unknown command.");
  } // SWITCH(cmd)

  if(cmd == GRAB)
  {
    mxAssert(nlhs > 0, "Output argument required for a GRAB.");

    //if(capture.empty() || capture[device] == NULL || not capture[device]->isOpened()) {
    if(parserThread == NULL) {
      mexErrMsgTxt("No camera open.");
    }

    DebugStreamClient::Object newObject = parserThread->GetNextObject(10);
    //printf("Received %s %s\n", newObject.typeName, newObject.newObject.objectName);

    // Simple example to display an image
    if(strcmp(newObject.objectName, "Robot Image") == 0) {
      Array<u8> image = *(reinterpret_cast<Array<u8>*>(newObject.startOfPayload));
#if USE_OPENCV_DISPLAY
      image.Show("Robot Image", false);
#endif
      //mexPrintf("Captured %dx%d frame.\n", image.get_size(1), image.get_size(0));
      plhs[0] = arrayToMxArray(image);
    } else {
      // Return an empty frame
      //mexWarnMsgTxt("Returning empty frame!");
      plhs[0] = mxCreateNumericMatrix(0,0, mxUINT8_CLASS, mxREAL);
    }

    if(newObject.buffer) {
      free(newObject.buffer);
      newObject.buffer = NULL;
    }
  } // IF(cmd==GRAB)
} // mexFunction()