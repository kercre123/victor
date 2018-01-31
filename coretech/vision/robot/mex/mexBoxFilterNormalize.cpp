#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "coretech/vision/robot/imageProcessing_declarations.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

using namespace Anki;
using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  mexAtExit(cv::destroyAllWindows);

  if(nrhs == 3 && nlhs == 1)
  {
    // Memory
    static const s32 OFFCHIP_BUFFER_SIZE = 2000000;
    //static const s32 ONCHIP_BUFFER_SIZE = 170000; // The max here is somewhere between 175000 and 180000 bytes
    //static const s32 CCM_BUFFER_SIZE = 50000; // The max here is probably 65536 (0x10000) bytes

    static char offchipBuffer[OFFCHIP_BUFFER_SIZE];
    //static char onchipBuffer[ONCHIP_BUFFER_SIZE];
    //static char ccmBuffer[CCM_BUFFER_SIZE];

    MemoryStack offchipMemory(offchipBuffer, OFFCHIP_BUFFER_SIZE);
    //MemoryStack onchipMemory(onchipBuffer,  ONCHIP_BUFFER_SIZE);
    //MemoryStack ccmMemory(ccmBuffer,     CCM_BUFFER_SIZE);

    // Get Initial Image
    mxAssert(mxGetClassID(prhs[0]) == mxUINT8_CLASS,
      "Image should be UINT8.");
    mxAssert(mxGetNumberOfDimensions(prhs[0]) == 2,
      "Image must be grayscale.");

    const s32 nrows = static_cast<s32>(mxGetM(prhs[0]));
    const s32 ncols = static_cast<s32>(mxGetN(prhs[0]));

    Array<u8> image(nrows, ncols, offchipMemory);
    mxAssert(image.IsValid(), "Unable to create Array<u8> for image -- out of scratch memory?");

    Array<u8> imageNorm(nrows, ncols, offchipMemory);
    mxAssert(imageNorm.IsValid(), "Unable to create Array<u8> for output normalized image -- out of scratch memory?");

    mxArrayToArray(prhs[0], image);

    const s32 boxSize = static_cast<s32>(mxGetScalar(prhs[1]));

    const u8  padValue = static_cast<u8>(mxGetScalar(prhs[2]));

    Result lastResult = ImageProcessing::BoxFilterNormalize(image, boxSize, padValue, imageNorm, offchipMemory);

    mxAssert(lastResult == RESULT_OK, "ImageProcessing::BoxFilterNormalize failed!");

    plhs[0] = arrayToMxArray(imageNorm);
  } else {
    mexPrintf("\n\n\timageNorm[UINT8] = mexBoxFilterNormalize(image[UINT8], boxWidth, padValue);\n\n");
  } // num argument checking

  return;
}
