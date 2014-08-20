// Load an embedded Array file to a matlab array

// Example:
// array = mexLoadEmbeddedArray(filename);

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/serialize.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/matlab/mexUtilities.h"
#include "anki/common/shared/utilities_shared.h"

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> Array<Type> Load(const char *filename, MemoryStack scratch, MemoryStack &memory)
{
  Array<Type> arr = Array<Type>();

  AnkiConditionalErrorAndReturnValue(NotAliased(scratch, memory),
    arr, "Load", "scratch and memory must be different");

  FILE *fp = fopen(filename, "rb");
  fseek(fp, 0L, SEEK_END);
  s32 bufferLength = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  void * buffer = scratch.Allocate(bufferLength);

  fread(buffer, fileSize, 1, fp);

  fclose(fp);

  SerializedBuffer serializedBuffer(buffer, bufferLength, Anki::Embedded::Flags::Buffer(false, true, true));

  SerializedBufferReconstructingIterator iterator(serializedBuffer);

  const char * typeName = NULL;
  const char * objectName = NULL;
  s32 dataLength;
  bool isReportedSegmentLengthCorrect;
  void * nextItem = iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect);

  if(!nextItem) {
    return arr;
  }

  if(strcmp(typeName, "Array") == 0) {
    arr = SerializedBuffer::DeserializeRawArray<Type>(NULL, &buffer, bufferLength, memory);
  }

  return arr;
}

mxArray* Load(const char *filename, MemoryStack scratch)
{
  FILE *fp = fopen(filename, "rb");
  fseek(fp, 0L, SEEK_END);
  s32 bufferLength = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  void * buffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(scratch.Allocate(bufferLength + MEMORY_ALIGNMENT + 64)) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

  fread(buffer, bufferLength, 1, fp);

  fclose(fp);

  SerializedBuffer serializedBuffer(buffer, bufferLength, Anki::Embedded::Flags::Buffer(false, true, true));

  SerializedBufferReconstructingIterator iterator(serializedBuffer);

  const char * typeName = NULL;
  const char * objectName = NULL;
  s32 dataLength;
  bool isReportedSegmentLengthCorrect;
  void * nextItem = iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect);

  if(!nextItem) {
    return NULL;
  }

  char localTypeName[128];
  char localObjectName[128];

  u16 basicType_sizeOfType;
  bool basicType_isBasicType;
  bool basicType_isInteger;
  bool basicType_isSigned;
  bool basicType_isFloat;

  {
    void * nextItemTmp = nextItem;
    s32 dataLengthTmp = dataLength;
    SerializedBuffer::DeserializeDescriptionStrings(localTypeName, localObjectName, &nextItemTmp, dataLengthTmp);

    s32 height;
    s32 width;
    s32 stride;
    Flags::Buffer flags;

    s32 basicType_numElements;
    SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &nextItemTmp, dataLengthTmp);
  }

  AnkiConditionalErrorAndReturnValue(basicType_isBasicType,
    NULL, "Load", "can only load a basic type");

  mxArray * toReturn = NULL;
  if(basicType_isFloat) {
    if(basicType_sizeOfType == 4) {
      Array<f32> array = SerializedBuffer::DeserializeRawArray<f32>(NULL, &nextItem, dataLength, scratch);
      toReturn = arrayToMxArray(array);
    } else if(basicType_sizeOfType == 8) {
      Array<f64> array = SerializedBuffer::DeserializeRawArray<f64>(NULL, &nextItem, dataLength, scratch);
      toReturn = arrayToMxArray(array);
    } else {
      AnkiError("Load", "can only load a basic type");
      return NULL;
    }
  } else {
    if(basicType_isSigned) {
      if(basicType_sizeOfType == 1) {
        Array<s8> array = SerializedBuffer::DeserializeRawArray<s8>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 2) {
        Array<s16> array = SerializedBuffer::DeserializeRawArray<s16>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 4) {
        Array<s32> array = SerializedBuffer::DeserializeRawArray<s32>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<s64> array = SerializedBuffer::DeserializeRawArray<s64>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else {
        AnkiError("Load", "can only load a basic type");
        return NULL;
      }
    } else {
      if(basicType_sizeOfType == 1) {
        Array<u8> array = SerializedBuffer::DeserializeRawArray<u8>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 2) {
        Array<u16> array = SerializedBuffer::DeserializeRawArray<u16>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 4) {
        Array<u32> array = SerializedBuffer::DeserializeRawArray<u32>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<u64> array = SerializedBuffer::DeserializeRawArray<u64>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else {
        AnkiError("Load", "can only load a basic type");
        return NULL;
      }
    }
  } // if(basicType_isFloat)

  return toReturn;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn(nrhs == 1 && nlhs == 1, "mexLoadEmbeddedArray", "Call this function as follows: array = mexLoadEmbeddedArray(filename);");

  const s32 bufferSize = 100000000;
  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexLoadEmbeddedArray", "Memory could not be allocated");

  char* filename = mxArrayToString(prhs[0]);

  plhs[0] = Load(filename, memory);

  mxFree(memory.get_buffer());
  mxFree(filename);
} // mexFunction()
