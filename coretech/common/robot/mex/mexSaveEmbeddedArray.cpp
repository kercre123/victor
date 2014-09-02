// Save a matlab array to an embedded Array file

// Example:
// mexSaveEmbeddedArray(array, filename);

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/serialize.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/matlab/mexUtilities.h"
#include "anki/common/shared/utilities_shared.h"

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> Result AllocateAndSave(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);
template<typename Type> Result AllocateAndSaveCell(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);
template<> Result AllocateAndSaveCell<char*>(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);

template<typename Type> Result AllocateAndSave(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  AnkiAssert(!mxIsCell(matlabArray));

  Array<Type> ankiArray = mxArrayToArray<Type>(matlabArray, scratch);

  if(!ankiArray.IsValid())
    return RESULT_FAIL;

  return ankiArray.SaveBinary(filename, compressionLevel, scratch);
}

template<> Result AllocateAndSaveCell<char*>(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  AnkiAssert(mxIsCell(matlabArray));

  Array<char *> ankiArray = mxCellArrayToStringArray(matlabArray, scratch);

  if(!ankiArray.IsValid())
    return RESULT_FAIL;

  return ankiArray.SaveBinary(filename, compressionLevel, scratch);
}

Result Save(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  const mxClassID matlabClassId = mxGetClassID(matlabArray);

  if(mxIsCell(matlabArray)) {
    const mxArray * firstElement = mxGetCell(matlabArray, 0);
    const mxClassID matlabCellClassId = mxGetClassID(firstElement);
    //if(matlabCellClassId == mxDOUBLE_CLASS) {
    //  return AllocateAndSaveCell<f64>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxSINGLE_CLASS) {
    //  return AllocateAndSaveCell<f32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT8_CLASS) {
    //  return AllocateAndSaveCell<s8>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT8_CLASS) {
    //  return AllocateAndSaveCell<u8>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT16_CLASS) {
    //  return AllocateAndSaveCell<s16>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT16_CLASS) {
    //  return AllocateAndSaveCell<u16>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT32_CLASS) {
    //  return AllocateAndSaveCell<s32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT32_CLASS) {
    //  return AllocateAndSaveCell<u32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT64_CLASS) {
    //  return AllocateAndSaveCell<s64>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT64_CLASS) {
    //  return AllocateAndSaveCell<u64>(matlabArray, filename, scratch);
    //} else
    if(matlabCellClassId == mxCHAR_CLASS) {
      return AllocateAndSaveCell<char *>(matlabArray, filename, compressionLevel, scratch);
    } else {
      AnkiAssert(false);
    }
  } else { // if(mxIsCell(matlabArray))
    if(matlabClassId == mxDOUBLE_CLASS) {
      return AllocateAndSave<f64>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxSINGLE_CLASS) {
      return AllocateAndSave<f32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT8_CLASS) {
      return AllocateAndSave<s8>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT8_CLASS) {
      return AllocateAndSave<u8>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT16_CLASS) {
      return AllocateAndSave<s16>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT16_CLASS) {
      return AllocateAndSave<u16>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT32_CLASS) {
      return AllocateAndSave<s32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT32_CLASS) {
      return AllocateAndSave<u32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT64_CLASS) {
      return AllocateAndSave<s64>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT64_CLASS) {
      return AllocateAndSave<u64>(matlabArray, filename, compressionLevel, scratch);
    } else {
      AnkiAssert(false);
    }
  } // if(mxIsCell(matlabArray)) ... else

  return RESULT_FAIL;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  s32 compressionLevel = 3;
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn((nrhs >= 2 && nrhs <= 3) && nlhs == 0, "mexSaveEmbeddedArray", "Call this function as follows: mexSaveEmbeddedArray(array, filename, <compressionLeve>);");

  //const s32 bufferSize = 8192 + mxGetM(prhs[0])*mxGetN(prhs[0])*8; // The 8 is for the maximum datatype size
  const s32 bufferSize = 100000000;
  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexSaveEmbeddedArray", "Memory could not be allocated");

  char* filename = mxArrayToString(prhs[1]);

  if(nrhs == 3) {
    compressionLevel = MAX(0, MIN(9, saturate_cast<s32>(mxGetScalar(prhs[2]))));
  }

  const Result result = Save(prhs[0], filename, compressionLevel, memory);

  mxFree(memory.get_buffer());
  mxFree(filename);
} // mexFunction()
