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

mxArray* Load(const char *filename, MemoryStack scratch)
{
  AnkiConditionalErrorAndReturnValue(filename && scratch.IsValid(),
    NULL, "Load", "Invalid inputs");

  FILE *fp = fopen(filename, "rb");

  AnkiConditionalErrorAndReturnValue(fp,
    NULL, "Load", "Invalid inputs");

  fseek(fp, 0, SEEK_END);
  s32 bufferLength = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  void * buffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(scratch.Allocate(bufferLength + MEMORY_ALIGNMENT + 64)) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
  void * allocatedBufferStart = NULL;
#endif

  // First, read the text header
  fread(buffer, Anki::Embedded::ARRAY_FILE_HEADER_LENGTH, 1, fp);

  AnkiConditionalErrorAndReturnValue(strncmp(reinterpret_cast<const char*>(buffer), ARRAY_FILE_HEADER, ARRAY_FILE_HEADER_VALID_LENGTH) == 0,
    NULL, "Array<Type>::LoadBinary", "File is not an Anki Embedded Array");

  bool isCompressed = false;
  if(reinterpret_cast<const char*>(buffer)[ARRAY_FILE_HEADER_VALID_LENGTH+1] == 'z') {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
    isCompressed = true;
#else
    AnkiError("Array<Type>::LoadBinary", "Loading with compression requires OpenCV");
    return NULL;
#endif
  }

  fread(buffer, bufferLength, 1, fp);

  fclose(fp);

  // Decompress the payload, if it is compressed
#if ANKICORETECH_EMBEDDED_USE_OPENCV
  if(isCompressed) {
    uLongf originalLength = static_cast<uLongf>( reinterpret_cast<s32*>(buffer)[0] );
    const s32 compressedLength = reinterpret_cast<s32*>(buffer)[1];

    allocatedBufferStart = mxMalloc(originalLength + MEMORY_ALIGNMENT + 64);
    void * uncompressedBuffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(allocatedBufferStart) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

    const s32 compressionResult = uncompress(reinterpret_cast<Bytef*>(uncompressedBuffer), &originalLength, reinterpret_cast<Bytef*>(buffer) + 2*sizeof(s32), compressedLength);

    buffer = uncompressedBuffer;
    bufferLength = originalLength;
  }
#endif

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
  bool basicType_isString;

  {
    void * nextItemTmp = nextItem;
    s32 dataLengthTmp = dataLength;
    SerializedBuffer::DeserializeDescriptionStrings(localTypeName, localObjectName, &nextItemTmp, dataLengthTmp);

    s32 height;
    s32 width;
    s32 stride;
    Flags::Buffer flags;

    s32 basicType_numElements;
    SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, &nextItemTmp, dataLengthTmp);
  }

  if(!(basicType_isBasicType || basicType_isString)) {
    if(allocatedBufferStart)
      mxFree(allocatedBufferStart);

    AnkiError("Load", "can only load a basic type or string");
    return NULL;
  }

  mxArray * toReturn = NULL;
  if(basicType_isString) {
    Array<const char *> array = SerializedBuffer::DeserializeRawArray<const char *>(NULL, &nextItem, dataLength, scratch);
    toReturn = stringArrayToMxCellArray(array);
  } else { // if(basicType_isString)
    if(basicType_isFloat) {
      if(basicType_sizeOfType == 4) {
        Array<f32> array = SerializedBuffer::DeserializeRawArray<f32>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else if(basicType_sizeOfType == 8) {
        Array<f64> array = SerializedBuffer::DeserializeRawArray<f64>(NULL, &nextItem, dataLength, scratch);
        toReturn = arrayToMxArray(array);
      } else {
        if(allocatedBufferStart)
          mxFree(allocatedBufferStart);

        AnkiError("Load", "can only load a basic type");
        return NULL;
      }
    } else { // if(basicType_isFloat)
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
          if(allocatedBufferStart)
            mxFree(allocatedBufferStart);

          AnkiError("Load", "can only load a basic type");
          return NULL;
        }
      } else { // if(basicType_isSigned)
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
          if(allocatedBufferStart)
            mxFree(allocatedBufferStart);

          AnkiError("Load", "can only load a basic type");
          return NULL;
        }
      } // if(basicType_isSigned) ... else
    } // if(basicType_isFloat) ... else
  } // if(basicType_isString) ... else

  if(allocatedBufferStart)
    mxFree(allocatedBufferStart);

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
  const s32 bufferSize = 4000000000 / numThreads;
#endif

  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexLoadEmbeddedArray", "Memory could not be allocated");

  char* filename = mxArrayToString(prhs[0]);

  plhs[0] = Load(filename, memory);

  mxFree(memory.get_buffer());
  mxFree(filename);
} // mexFunction()
