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

template<typename Type> Result Save(const Array<Type> &array, const char *filename, MemoryStack scratch)
{
  const s32 serializedBufferLength = 4096 + array.get_size(0) * array.get_stride();
  void *buffer = scratch.Allocate(serializedBufferLength);

  AnkiConditionalErrorAndReturnValue(buffer,
    RESULT_FAIL_OUT_OF_MEMORY, "Save", "Memory could not be allocated");

  SerializedBuffer toSave(buffer, serializedBufferLength);

  toSave.PushBack<Type>("Array", array);

  s32 startIndex;
  u8 * bufferStart = reinterpret_cast<u8*>(toSave.get_memoryStack().get_validBufferStart(startIndex));
  const s32 validUsedBytes = toSave.get_memoryStack().get_usedBytes() - startIndex;

  const s32 startDiff = static_cast<s32>( reinterpret_cast<size_t>(bufferStart) - reinterpret_cast<size_t>(toSave.get_memoryStack().get_buffer()) );
  const s32 endDiff = toSave.get_memoryStack().get_totalBytes() - toSave.get_memoryStack().get_usedBytes();

  FILE *fp = fopen(filename, "wb");

  AnkiConditionalErrorAndReturnValue(fp,
    RESULT_FAIL_IO, "Save", "Could not open file");

  const size_t bytesWrittenForHeader = fwrite(&SERIALIZED_BUFFER_HEADER[0], SERIALIZED_BUFFER_HEADER_LENGTH, 1, fp);

  const size_t bytesWritten = fwrite(bufferStart, validUsedBytes, 1, fp);

  const size_t bytesWrittenForFooter = fwrite(&SERIALIZED_BUFFER_FOOTER[0], SERIALIZED_BUFFER_FOOTER_LENGTH, 1, fp);

  fclose(fp);

  return RESULT_OK;
}

template<typename Type> Result AllocateAndSave(const mxArray *matlabArray, const char *filename, MemoryStack scratch)
{
  Array<Type> ankiArray = mxArrayToArray<Type>(matlabArray, scratch);

  if(!ankiArray.IsValid())
    return RESULT_FAIL;

  return Save<Type>(ankiArray, filename, scratch);
}

Result Save(const mxArray *matlabArray, const char *filename, MemoryStack scratch)
{
  const mxClassID matlabClassId = mxGetClassID(matlabArray);

  if(matlabClassId == mxDOUBLE_CLASS) {
    return AllocateAndSave<f64>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxSINGLE_CLASS) {
    return AllocateAndSave<f32>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxINT8_CLASS) {
    return AllocateAndSave<s8>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxUINT8_CLASS) {
    return AllocateAndSave<u8>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxINT16_CLASS) {
    return AllocateAndSave<s16>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxUINT16_CLASS) {
    return AllocateAndSave<u16>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxINT32_CLASS) {
    return AllocateAndSave<s32>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxUINT32_CLASS) {
    return AllocateAndSave<u32>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxINT64_CLASS) {
    return AllocateAndSave<s64>(matlabArray, filename, scratch);
  } else if(matlabClassId == mxUINT64_CLASS) {
    return AllocateAndSave<u64>(matlabArray, filename, scratch);
  }

  return RESULT_FAIL;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 2 && nlhs == 0, "mexSaveEmbeddedArray", "Call this function as follows: mexSaveEmbeddedArray(array, filename);");

  const s32 bufferSize = 8192 + mxGetM(prhs[0])*mxGetN(prhs[0])*8; // The 8 is for the maximum datatype size
  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexSaveEmbeddedArray", "Memory could not be allocated");

  char* filename = mxArrayToString(prhs[1]);

  const Result result = Save(prhs[0], filename, memory);

  mxFree(memory.get_buffer());
  mxFree(filename);
} // mexFunction()
