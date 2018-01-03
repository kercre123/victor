#ifndef _MEX_UTILITES_H_
#define _MEX_UTILITES_H_

#include "anki/common/robot/config.h"

#include "mex.h"
#include "engine.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/matlab/mexWrappers.h"

#include "anki/common/matlab/mexUtilities_declarations.h"

namespace Anki
{
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
  } // maxAndMin()

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
    const mxClassID matlabClassId = GetMatlabClassID<Type>();
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
  } // findUnique()
} // namespace Anki

#endif // #ifndef _MEX_UTILITES_H_
