/**
File: array2d.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/utilities_c.h"
#include "coretech/common/robot/serialize.h"

namespace Anki
{
  namespace Embedded
  {
    template<> Result Array<bool>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<char *>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintString(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<const char *>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintString(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f32>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, version, minY, maxY, minX, maxX);
    }

    template<> Result Array<f64>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, version, minY, maxY, minX, maxX);
    }

    template<> template<> s32 Array<u8>::SetCast(const s32 * const values, const s32 numValues)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::SetCast", "Array<Type> is not valid");

      s32 numValuesSet = 0;

      for(s32 y=0; y<size[0]; y++) {
        u8 * restrict pThisData = Pointer(y, 0);

        const s32 numValuesThisRow = MAX(0, MIN(numValues - y*size[1], size[1]));

        if(numValuesThisRow > 0) {
          for(s32 x=0; x<numValuesThisRow; x++) {
            pThisData[x] = saturate_cast<u8>( (values+y*size[1])[x] );
          }
          numValuesSet += numValuesThisRow;
        }

        if(numValuesThisRow < size[1]) {
          memset(pThisData+numValuesThisRow*sizeof(u8), 0, (size[1]-numValuesThisRow)*sizeof(u8));
        }
      }

      return numValuesSet;
    }

    template<> template<> s32 Array<s16>::SetCast(const s32 * const values, const s32 numValues)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::SetCast", "Array<Type> is not valid");

      s32 numValuesSet = 0;

      for(s32 y=0; y<size[0]; y++) {
        s16 * restrict pThisData = Pointer(y, 0);

        const s32 numValuesThisRow = MAX(0, MIN(numValues - y*size[1], size[1]));

        if(numValuesThisRow > 0) {
          for(s32 x=0; x<numValuesThisRow; x++) {
            pThisData[x] = saturate_cast<s16>( (values+y*size[1])[x] );
          }
          numValuesSet += numValuesThisRow;
        }

        if(numValuesThisRow < size[1]) {
          memset(pThisData+numValuesThisRow*sizeof(u8), 0, (size[1]-numValuesThisRow)*sizeof(u8));
        }
      }

      return numValuesSet;
    }

    Array<u8> LoadBinaryArray_UnknownType(
      const char * filename,
      MemoryStack *scratch,
      MemoryStack *memory,
      void * allocatedBuffer,
      const s32 allocatedBufferLength,
      u16  &basicType_sizeOfType,
      bool &basicType_isBasicType,
      bool &basicType_isInteger,
      bool &basicType_isSigned,
      bool &basicType_isFloat,
      bool &basicType_isString
      )
    {
#if defined(__EDG__)
      AnkiError("Array<Type>::LoadBinaryArray_Generic", "Cannot load files on embedded");
      return Array<u8>();
#else     
      Array<u8> newArray = Array<u8>();

      MemoryStack scratch_local;

      bool useMalloc;
      if(!scratch) {
#if ANKICORETECH_EMBEDDED_USE_MALLOC
        AnkiAssert(!memory && allocatedBuffer && allocatedBufferLength > 0);
        useMalloc = true;
#else
        AnkiError("Array<Type>::LoadBinaryArray_Generic", "malloc is not enabled");
        return newArray;
#endif
      } else {
        AnkiAssert(memory && !allocatedBuffer && allocatedBufferLength < 0);
        useMalloc = false;

        AnkiConditionalErrorAndReturnValue(NotAliased(*scratch, *memory) && AreValid(*scratch, *memory),
          newArray, "Array<Type>::LoadBinaryArray_Generic", "scratch and memory must be different");

        scratch_local = MemoryStack(*scratch);
      }

      AnkiConditionalErrorAndReturnValue(filename,
        newArray, "Array<Type>::LoadBinaryArray_Generic", "Invalid inputs");

      FILE *fp = fopen(filename, "rb");

      AnkiConditionalErrorAndReturnValue(fp,
        newArray, "Array<Type>::LoadBinaryArray_Generic", "Could not open file %s", filename);

      fseek(fp, 0, SEEK_END);
      s32 tmpBufferLength = static_cast<s32>( ftell(fp) - ARRAY_FILE_HEADER_LENGTH );
      fseek(fp, 0, SEEK_SET);

      void * tmpBuffer = NULL;

#if ANKICORETECH_EMBEDDED_USE_MALLOC
      void * tmpBufferStart = nullptr;
#endif

      if(useMalloc) {
#if ANKICORETECH_EMBEDDED_USE_MALLOC
        tmpBufferStart = calloc(tmpBufferLength + 2*MEMORY_ALIGNMENT + 64, 1);
        tmpBuffer = tmpBufferStart;
#else
        AnkiAssert(false);
#endif
      } else {
        tmpBuffer = scratch_local.Allocate(tmpBufferLength + MEMORY_ALIGNMENT + 64);
      }

      // Align tmpBuffer to MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH
      tmpBuffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(tmpBuffer) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

#if ANKICORETECH_EMBEDDED_USE_ZLIB
      void * uncompressedBufferStart = NULL;
#endif

      // First, read the text header
      const size_t bytesRead1 = fread(tmpBuffer, 1, ARRAY_FILE_HEADER_LENGTH, fp);

      AnkiConditionalErrorAndReturnValue(bytesRead1 == ARRAY_FILE_HEADER_LENGTH && strncmp(reinterpret_cast<const char*>(tmpBuffer), ARRAY_FILE_HEADER, ARRAY_FILE_HEADER_VALID_LENGTH) == 0,
        newArray, "Array<Type>::LoadBinaryArray_Generic", "File is not an Anki Embedded Array");

      bool isCompressed = false;
      if(reinterpret_cast<const char*>(tmpBuffer)[ARRAY_FILE_HEADER_VALID_LENGTH+1] == 'z') {
#if ANKICORETECH_EMBEDDED_USE_ZLIB
        isCompressed = true;
#else
        AnkiError("Array<Type>::LoadBinaryArray_Generic", "zlib is not enabled");
        return newArray;
#endif
      }

      // Next, read the actual payload
      const size_t bytesRead2 = fread(tmpBuffer, 1, tmpBufferLength, fp);

      fclose(fp);

      AnkiConditionalErrorAndReturnValue(bytesRead2 > 0,
        newArray, "Array<Type>::LoadBinaryArray_Generic", "File is not an Anki Embedded Array");

      // Decompress the payload, if it is compressed
      if(isCompressed) {
#if ANKICORETECH_EMBEDDED_USE_ZLIB
        uLongf originalLength = static_cast<uLongf>( reinterpret_cast<s32*>(tmpBuffer)[0] );
        const s32 compressedLength = reinterpret_cast<s32*>(tmpBuffer)[1];

        uncompressedBufferStart = calloc(originalLength + MEMORY_ALIGNMENT + 64, 1);
        void * uncompressedBuffer = reinterpret_cast<void*>( RoundUp<size_t>(reinterpret_cast<size_t>(uncompressedBufferStart) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT) - MemoryStack::HEADER_LENGTH);

        uncompress(reinterpret_cast<Bytef*>(uncompressedBuffer), &originalLength, reinterpret_cast<Bytef*>(tmpBuffer) + 2*sizeof(s32), compressedLength);

        tmpBuffer = uncompressedBuffer;
        tmpBufferLength = static_cast<s32>(originalLength);
#endif
      }

      SerializedBuffer serializedBuffer(tmpBuffer, tmpBufferLength, Anki::Embedded::Flags::Buffer(false, true, true));

      SerializedBufferReconstructingIterator iterator(serializedBuffer);

      const char * typeName = NULL;
      const char * objectName = NULL;
      s32 dataLength;
      bool isReportedSegmentLengthCorrect;
      void * nextItem = iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect);

      if(!nextItem && strcmp(typeName, "Array") != 0) {
#if ANKICORETECH_EMBEDDED_USE_MALLOC
        if(useMalloc && tmpBufferStart) {
          free(tmpBufferStart);
        }
#endif

#if ANKICORETECH_EMBEDDED_USE_ZLIB
        if(isCompressed) {
          free(uncompressedBufferStart);
        }
#endif

        AnkiError("Array<Type>::LoadBinaryArray_Generic", "Could not parse data");
        return newArray;
      }

      char arrayName[128];

      {
        char localTypeName[128];
        char localObjectName[128];
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
#if ANKICORETECH_EMBEDDED_USE_MALLOC
        if(useMalloc) {
          free(tmpBufferStart);
        }
#endif

#if ANKICORETECH_EMBEDDED_USE_ZLIB
        if(isCompressed) {
          free(uncompressedBufferStart);
        }
#endif

        AnkiError("Load", "can only load a basic type or string");
        return newArray;
      }

      MemoryStack allocater_data;
      MemoryStack *allocater = &allocater_data;
      if(useMalloc) {
#if ANKICORETECH_EMBEDDED_USE_MALLOC
        *allocater = MemoryStack(allocatedBuffer, allocatedBufferLength);
#endif
      } else {
        allocater = memory;
      }

      if(basicType_isString) {
        Array<const char *> array = SerializedBuffer::DeserializeRawArray<const char *>(&arrayName[0], &nextItem, dataLength, *allocater);
        newArray = *reinterpret_cast<Array<u8>*>(&array);
      } else { // if(basicType_isString)
        if(basicType_isFloat) {
          if(basicType_sizeOfType == 4) {
            Array<f32> array = SerializedBuffer::DeserializeRawArray<f32>(&arrayName[0], &nextItem, dataLength, *allocater);
            newArray = *reinterpret_cast<Array<u8>*>(&array);
          } else if(basicType_sizeOfType == 8) {
            Array<f64> array = SerializedBuffer::DeserializeRawArray<f64>(&arrayName[0], &nextItem, dataLength, *allocater);
            newArray = *reinterpret_cast<Array<u8>*>(&array);
          }
        } else { // if(basicType_isFloat)
          if(basicType_isSigned) {
            if(basicType_sizeOfType == 1) {
              Array<s8> array = SerializedBuffer::DeserializeRawArray<s8>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 2) {
              Array<s16> array = SerializedBuffer::DeserializeRawArray<s16>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 4) {
              Array<s32> array = SerializedBuffer::DeserializeRawArray<s32>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 8) {
              Array<s64> array = SerializedBuffer::DeserializeRawArray<s64>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            }
          } else { // if(basicType_isSigned)
            if(basicType_sizeOfType == 1) {
              Array<u8> array = SerializedBuffer::DeserializeRawArray<u8>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 2) {
              Array<u16> array = SerializedBuffer::DeserializeRawArray<u16>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 4) {
              Array<u32> array = SerializedBuffer::DeserializeRawArray<u32>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            } else if(basicType_sizeOfType == 8) {
              Array<u64> array = SerializedBuffer::DeserializeRawArray<u64>(&arrayName[0], &nextItem, dataLength, *allocater);
              newArray = *reinterpret_cast<Array<u8>*>(&array);
            }
          } // if(basicType_isSigned) ... else
        } // if(basicType_isFloat) ... else
      } // if(basicType_isString) ... else

#if ANKICORETECH_EMBEDDED_USE_MALLOC
      if(useMalloc) {
        free(tmpBufferStart);
      }
#endif

#if ANKICORETECH_EMBEDDED_USE_ZLIB
      if(isCompressed) {
        free(uncompressedBufferStart);
      }
#endif

      return newArray;
#endif // #if defined(__EDG__) ... #else
    } // LoadBinaryArray_UnknownType()
  } // namespace Embedded
} // namespace Anki
