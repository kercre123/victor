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
#if ANKICORETECH_EMBEDDED_USE_OPENCV
  void * uncompressed1Buffer = NULL;
  void * uncompressed2BufferStart = NULL;
#endif

  AnkiConditionalErrorAndReturnValue(filename && scratch.IsValid(),
    NULL, "Load", "Invalid inputs");

  FILE *fp = fopen(filename, "rb");
  fseek(fp, 0, SEEK_END);
  s32 loadBufferLength = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  void * loadBuffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(scratch.Allocate(loadBufferLength + MEMORY_ALIGNMENT + 64)) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

  // First, read the text header
  fread(loadBuffer, Anki::Embedded::ARRAY_FILE_HEADER_LENGTH, 1, fp);

  AnkiConditionalErrorAndReturnValue(strncmp(reinterpret_cast<const char*>(loadBuffer), ARRAY_FILE_HEADER, ARRAY_FILE_HEADER_VALID_LENGTH) == 0,
    NULL, "Array<Type>::LoadBinary", "File is not an Anki Embedded Array");

  bool isCompressed = false;
  if(reinterpret_cast<const char*>(loadBuffer)[ARRAY_FILE_HEADER_VALID_LENGTH+1] == 'z') {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
    isCompressed = true;
#else
    AnkiError("Array<Type>::LoadBinary", "Loading with compression requires OpenCV");
    return NULL;
#endif
  }

  fread(loadBuffer, loadBufferLength, 1, fp);

  fclose(fp);

  // Decompress the payload, if it is compressed
#if ANKICORETECH_EMBEDDED_USE_OPENCV
  if(isCompressed) {
    uLongf originalLength = static_cast<uLongf>( reinterpret_cast<s32*>(loadBuffer)[0] );
    uLongf compressed1Length = static_cast<uLongf>( reinterpret_cast<s32*>(loadBuffer)[1] );
    uLongf compressed2Length = static_cast<uLongf>( reinterpret_cast<s32*>(loadBuffer)[2] );

    uncompressed1Buffer = mxMalloc(compressed1Length + MEMORY_ALIGNMENT + 64);
    uncompressed2BufferStart = mxMalloc(originalLength + MEMORY_ALIGNMENT + 64);
    void * uncompressed2Buffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(uncompressed2BufferStart) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

    s32 compressionResult = uncompress(reinterpret_cast<Bytef*>(uncompressed1Buffer), &compressed1Length, reinterpret_cast<Bytef*>(loadBuffer) + 3*sizeof(s32), compressed2Length);
    compressionResult = uncompress(reinterpret_cast<Bytef*>(uncompressed2Buffer), &originalLength, reinterpret_cast<Bytef*>(uncompressed1Buffer), compressed1Length);

    loadBuffer = uncompressed2Buffer;
    loadBufferLength = originalLength;
  }
#endif

  SerializedBuffer serializedBuffer(loadBuffer, loadBufferLength, Anki::Embedded::Flags::Buffer(false, true, true));

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
    if(uncompressed1Buffer) mxFree(uncompressed1Buffer);
    if(uncompressed2BufferStart) mxFree(uncompressed2BufferStart);

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
        if(uncompressed1Buffer) mxFree(uncompressed1Buffer);
        if(uncompressed2BufferStart) mxFree(uncompressed2BufferStart);

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
          if(uncompressed1Buffer) mxFree(uncompressed1Buffer);
          if(uncompressed2BufferStart) mxFree(uncompressed2BufferStart);

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
          if(uncompressed1Buffer) mxFree(uncompressed1Buffer);
          if(uncompressed2BufferStart) mxFree(uncompressed2BufferStart);

          AnkiError("Load", "can only load a basic type");
          return NULL;
        }
      } // if(basicType_isSigned) ... else
    } // if(basicType_isFloat) ... else
  } // if(basicType_isString) ... else

  if(uncompressed1Buffer) mxFree(uncompressed1Buffer);
  if(uncompressed2BufferStart) mxFree(uncompressed2BufferStart);

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
