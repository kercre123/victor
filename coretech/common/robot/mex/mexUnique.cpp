#include "mex.h"

#include "anki/common/robot/matlabInterface.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

#include <string.h>
#include <vector>
#include <cmath>
#include <math.h>

using namespace Anki::Embedded;

// Works the same as Matlab's unique.m, but if the input is an integer array with a small maximum value, it uses a different (often faster) algorithm

// Example:
// uniqueValues = mexUnique(inputArray)

// Find the min an max values of the Matlab array
template<typename Type> void maxAndMin(const mxArray *array, Type &minValue, Type &maxValue)
{
  const s32 numElements = mxGetNumberOfElements(array);
  
  const Type * restrict pArray = reinterpret_cast<Type *>( mxGetData(array) );
  
  minValue = pArray[0];
  maxValue = pArray[0];
  
  for(s32 i=1; i<numElements; i++) {
    minValue = MIN(minValue, pArray[i]);
    maxValue = MAX(maxValue, pArray[i]);
  }
}

// Count the numbers of each unique value for the integer Matlab array, then allocate and return a Matlab array with the unique values
// TODO: return the counts too?
template<typename Type> mxArray* findUnique(const mxArray *array, const Type minValue, const Type maxValue)
{
  const s32 numElements = mxGetNumberOfElements(array);
  
  const Type * restrict pArray = reinterpret_cast<Type *>( mxGetData(array) );
  
  const s32 countsLength = maxValue - minValue + 1;
  
  s32 * counts = reinterpret_cast<s32*>( mxMalloc(countsLength*sizeof(s32)) );
  
  memset(counts, 0, countsLength*sizeof(s32));
  
  for(s32 i=0; i<numElements; i++) {
    const Type curValue = pArray[i] - minValue;
    counts[curValue]++;
  }

  mwSize numUnique = 0;
  
  for(s32 i=0; i<countsLength; i++) {
    if(counts[i] > 0)
      numUnique++;
  }
  
  const mwSize outputDims[2] = {numUnique, 1};
  const mxClassID matlabClassId = Matlab::GetMatlabClassID<Type>();
  mxArray* toReturn = mxCreateNumericArray(2, outputDims, matlabClassId, mxREAL);
  Type * restrict pToReturn = reinterpret_cast<Type *>( mxGetData(toReturn) );
  
  s32 cToReturn = 0;
  for(s32 i=0; i<countsLength; i++) {
    if(counts[i] > 0) {
      pToReturn[cToReturn] = static_cast<Type>(i + minValue);
      cToReturn++;
    }
  }
  
  mxFree(counts);
  
  return toReturn;
}

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
