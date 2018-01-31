#include "mex.h"

#include "coretech/common/robot/matlabInterface.h"

#include "coretech/common/matlab/mexWrappers.h"
#include "coretech/common/matlab/mexUtilities.h"
#include "coretech/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>
#include <cmath>
#include <math.h>

using namespace Anki;
using namespace Anki::Embedded;

// Works the same as Matlab's unique.m, but if the input is an integer array with a small maximum value, it uses a different (often faster) algorithm

// Example:
// uniqueValues = mexUnique(inputArray)

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  // If there's too many possible unique elements, this method uses too much memory
  const s32 MAX_POSSIBLE_UNIQUES = 10000000;

  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexUnique", "Call this function as follows: [uniqueValues = mexUnique(inputArray);");

  bool useMatlabDefault = true;

  // TODO: support other int classes
  if(mxGetClassID(prhs[0]) == mxINT32_CLASS) {
    s32 minValue, maxValue;
    maxAndMin<s32>(prhs[0], minValue, maxValue);

    const s32 numPossibleUnique = maxValue - minValue + 1;

    // If there's too many possible unique elements, this method isn't efficient, so just call the normal Matlab unique.m
    if((numPossibleUnique < mxGetNumberOfElements(prhs[0])) &&
      numPossibleUnique < MAX_POSSIBLE_UNIQUES) {
        useMatlabDefault = false;

        plhs[0] = findUnique<s32>(prhs[0], minValue, maxValue);
    }
  } else if(mxGetClassID(prhs[0]) == mxUINT8_CLASS) {
    s32 minValue = 0, maxValue = 255;

    const s32 numPossibleUnique = maxValue - minValue + 1;

    // If there's too many possible unique elements, this method isn't efficient, so just call the normal Matlab unique.m
    if((numPossibleUnique < mxGetNumberOfElements(prhs[0])) &&
      numPossibleUnique < MAX_POSSIBLE_UNIQUES) {
        useMatlabDefault = false;

        plhs[0] = findUnique<u8>(prhs[0], minValue, maxValue);
    }
  }

  // If the class type is not supported, or the statistics are non-ideal, just call Matlab's unique.m
  if(useMatlabDefault) {
    mxArray	*prhs_nonConst = const_cast<mxArray*>(prhs[0]);
    mexCallMATLAB(nlhs, plhs, 1, &prhs_nonConst, "unique");
  }
} // mexFunction()
