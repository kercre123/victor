// Load an embedded Array file to a matlab array

// Example:
// array = mexLoadEmbeddedArray(filename);

#include "mex.h"

#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/matlabConverters_embedded.h"
#include "coretech/common/robot/serialize.h"

#include "coretech/common/matlab/mexWrappers.h"
#include "coretech/common/matlab/mexUtilities.h"
#include "coretech/common/shared/utilities_shared.h"

using namespace Anki;
using namespace Anki::Embedded;

mxArray* Load(const char *filename, void * allocatedBuffer, const s32 allocatedBufferLength)
{
  AnkiConditionalErrorAndReturnValue(filename && allocatedBuffer,
    NULL, "Load", "Invalid inputs");

  u16  basicType_sizeOfType;
  bool basicType_isBasicType;
  bool basicType_isInteger;
  bool basicType_isSigned;
  bool basicType_isFloat;
  bool basicType_isString;

  Array<u8> arrayTmp = LoadBinaryArray_UnknownType(
    filename,
    NULL, NULL,
    allocatedBuffer, allocatedBufferLength,
    basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString);

  mxArray * toReturn = NULL;
  if(basicType_isString) {
    Array<const char *> array = *reinterpret_cast<Array<const char*>* >( &arrayTmp );
    toReturn = stringArrayToMxCellArray(array);
  } else { // if(basicType_isString)
    if(basicType_isFloat) {
      if(basicType_sizeOfType == 4) {
        Array<f32> array = *reinterpret_cast<Array<f32>* >( &arrayTmp );
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<f64> array = *reinterpret_cast<Array<f64>* >( &arrayTmp );
        toReturn = arrayToMxArray(array);
      } else {
        AnkiError("Load", "can only load a basic type");
        return NULL;
      }
    } else { // if(basicType_isFloat)
      if(basicType_isSigned) {
        if(basicType_sizeOfType == 1) {
          Array<s8> array = *reinterpret_cast<Array<s8>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<s16> array = *reinterpret_cast<Array<s16>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<s32> array = *reinterpret_cast<Array<s32>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<s64> array = *reinterpret_cast<Array<s64>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else {
          AnkiError("Load", "can only load a basic type");
          return NULL;
        }
      } else { // if(basicType_isSigned)
        if(basicType_sizeOfType == 1) {
          Array<u8> array = *reinterpret_cast<Array<u8>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 2) {
          Array<u16> array = *reinterpret_cast<Array<u16>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 4) {
          Array<u32> array = *reinterpret_cast<Array<u32>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else if(basicType_sizeOfType == 8) {
          Array<u64> array = *reinterpret_cast<Array<u64>* >( &arrayTmp );
          toReturn = arrayToMxArray(array);
        } else {
          AnkiError("Load", "can only load a basic type");
          return NULL;
        }
      } // if(basicType_isSigned) ... else
    } // if(basicType_isFloat) ... else
  } // if(basicType_isString) ... else

  return toReturn;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexLoadEmbeddedArray", "Call this function as follows: array = mexLoadEmbeddedArray(filename);");

#ifdef _MSC_VER
  // 32-bit
  const s32 bufferSize = 200000000;
#else
  // 64-bit
  const s32 bufferSize = 0x3fffffff;
#endif

  void * allocatedBuffer = mxMalloc(bufferSize);

  AnkiConditionalErrorAndReturn(allocatedBuffer, "mexLoadEmbeddedArray", "Memory could not be allocated");

  char* filename = mxArrayToString(prhs[0]);

  plhs[0] = Load(filename, allocatedBuffer, bufferSize);

  mxFree(allocatedBuffer);
  mxFree(filename);
} // mexFunction()
