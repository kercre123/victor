/**
File: serialize.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/serialize.h"
#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/compress.h"

namespace Anki
{
  namespace Embedded
  {
    SerializedBuffer::SerializedBuffer()
    {
      this->memoryStack = MemoryStack();
    }

    SerializedBuffer::SerializedBuffer(void *buffer, s32 bufferLength, Flags::Buffer flags)

    {
      if(flags.get_isFullyAllocated()) {
        AnkiConditionalErrorAndReturn((reinterpret_cast<size_t>(buffer)+MemoryStack::HEADER_LENGTH)%MEMORY_ALIGNMENT == 0,
          "SerializedBuffer::SerializedBuffer", "If fully allocated, the %dth byte of the buffer must be %d byte aligned", MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT);
      }

      this->memoryStack = MemoryStack(buffer, bufferLength, flags);
    }

    Result SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(const bool updateBufferPointer, u16 &sizeOfType, bool &isBasicType, bool &isInteger, bool &isSigned, bool &isFloat, bool &isString, s32 &numElements, void** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(reinterpret_cast<u32*>(*buffer)[0] & 1)
        isBasicType = true;
      else
        isBasicType = false;

      if(reinterpret_cast<u32*>(*buffer)[0] & (1<<1))
        isInteger = true;
      else
        isInteger = false;

      if(reinterpret_cast<u32*>(*buffer)[0] & (1<<2))
        isSigned = true;
      else
        isSigned = false;

      if(reinterpret_cast<u32*>(*buffer)[0] & (1<<3))
        isFloat = true;
      else
        isFloat = false;

      if(reinterpret_cast<u32*>(*buffer)[0] & (1<<4))
        isString = true;
      else
        isString = false;

      sizeOfType = (reinterpret_cast<u32*>(*buffer)[0] & 0xFFFF0000) >> 16;

      numElements = static_cast<s32>(reinterpret_cast<u32*>(*buffer)[1]);

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH;
      }

      return RESULT_OK;
    }

    Result SerializedBuffer::EncodedArray::Deserialize(const bool updateBufferPointer, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, u16 &basicType_sizeOfType, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat, bool &basicType_isString, s32 &basicType_numElements, void** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedArray::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(false, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      height = reinterpret_cast<u32*>(*buffer)[2];
      width = reinterpret_cast<u32*>(*buffer)[3];
      stride = reinterpret_cast<u32*>(*buffer)[4];
      flags.set_rawFlags(reinterpret_cast<u32*>(*buffer)[5]);

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedArray::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedArray::CODE_LENGTH;
      }

      return RESULT_OK;
    }

    Result SerializedBuffer::EncodedArraySlice::Deserialize(const bool updateBufferPointer, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, s32 &ySlice_start, s32 &ySlice_increment, s32 &ySlice_size, s32 &xSlice_start, s32 &xSlice_increment, s32 &xSlice_size, u16 &basicType_sizeOfType, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat, bool &basicType_isString, s32 &basicType_numElements, void** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedArraySlice::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      ySlice_start = reinterpret_cast<u32*>(*buffer)[6];
      ySlice_increment = reinterpret_cast<u32*>(*buffer)[7];
      ySlice_size = reinterpret_cast<u32*>(*buffer)[8];
      xSlice_start = reinterpret_cast<u32*>(*buffer)[9];
      xSlice_increment = reinterpret_cast<u32*>(*buffer)[10];
      xSlice_size = reinterpret_cast<u32*>(*buffer)[11];

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedArraySlice::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedArraySlice::CODE_LENGTH;
      }

      return RESULT_OK;
    }

    Result SerializedBuffer::FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex)
    {
      startIndex = -1;
      endIndex = -1;

      AnkiConditionalErrorAndReturnValue(rawBuffer != NULL,
        RESULT_FAIL_UNINITIALIZED_MEMORY, "SerializedBuffer::FindSerializedBuffer", "rawBuffer is NULL");

      AnkiConditionalErrorAndReturnValue(rawBufferLength >= 0,
        RESULT_FAIL_INVALID_PARAMETER, "SerializedBuffer::FindSerializedBuffer", "rawBufferLength is >= 0");

      if(rawBufferLength == 0)
        return RESULT_OK;

      s32 state = 0;
      s32 index = 0;

      // Look for the header
      while(index < rawBufferLength) {
        if(state == SERIALIZED_BUFFER_HEADER_LENGTH) {
          startIndex = index;
          break;
        }

        if(reinterpret_cast<const u8*>(rawBuffer)[index] == SERIALIZED_BUFFER_HEADER[state]) {
          state++;
        } else if(reinterpret_cast<const u8*>(rawBuffer)[index] == SERIALIZED_BUFFER_HEADER[0]) {
          state = 1;
        } else {
          state = 0;
        }

        index++;
      } // while(index < rawBufferLength)

      // Look for the footer
      state = 0;
      while(index < rawBufferLength) {
        if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
          endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
          break;
        }

        //CoreTechPrint("%d) %d %x %x\n", index, state, rawBufferU8[index], SERIALIZED_BUFFER_FOOTER[state]);

        if(reinterpret_cast<const u8*>(rawBuffer)[index] == SERIALIZED_BUFFER_FOOTER[state]) {
          state++;
        } else if(reinterpret_cast<const u8*>(rawBuffer)[index] == SERIALIZED_BUFFER_FOOTER[0]) {
          state = 1;
        } else {
          state = 0;
        }

        index++;
      } // while(index < rawBufferLength)

      if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
        endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
      }

      return RESULT_OK;
    } // void FindSerializedBuffer(const void * rawBuffer, s32 &startIndex, s32 &endIndex)

    Result SerializedBuffer::SerializeOneDescriptionString(const char *description, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < DESCRIPTION_STRING_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      s32 iChar = 0;

      // If objectName is not NULL, copy it
      if(description) {
        for(iChar=0; iChar<(DESCRIPTION_STRING_LENGTH-1); iChar++) {
          if(description[iChar] == '\0') {
            break;
          }

          reinterpret_cast<u8*>(*buffer)[iChar] = description[iChar];
        }
      } // if(objectName)

      reinterpret_cast<u8*>(*buffer)[iChar] = '\0';

      *buffer = reinterpret_cast<u8*>(*buffer) + DESCRIPTION_STRING_LENGTH;
      bufferLength -= DESCRIPTION_STRING_LENGTH;

      return RESULT_OK;
    }

    Result SerializedBuffer::DeserializeOneDescriptionString(char *description, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < DESCRIPTION_STRING_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(description) {
        s32 iChar = 0;

        for(iChar=0; iChar<(DESCRIPTION_STRING_LENGTH-1); iChar++) {
          if(reinterpret_cast<u8*>(*buffer)[iChar] == '\0') {
            break;
          }

          description[iChar] = reinterpret_cast<u8*>(*buffer)[iChar];
        }

        description[iChar] = '\0';
      }

      *buffer = reinterpret_cast<u8*>(*buffer) + DESCRIPTION_STRING_LENGTH;
      bufferLength -= DESCRIPTION_STRING_LENGTH;

      return RESULT_OK;
    }

    Result SerializedBuffer::SerializeDescriptionStrings(const char *typeName, const char *objectName, void ** buffer, s32 &bufferLength)
    {
      SerializeOneDescriptionString(typeName, buffer, bufferLength);
      SerializeOneDescriptionString(objectName, buffer, bufferLength);

      return RESULT_OK;
    }

    Result SerializedBuffer::DeserializeDescriptionStrings(char *typeName, char *objectName, void ** buffer, s32 &bufferLength)
    {
      DeserializeOneDescriptionString(typeName, buffer, bufferLength);
      DeserializeOneDescriptionString(objectName, buffer, bufferLength);

      return RESULT_OK;
    }

    void* SerializedBuffer::AllocateRaw(const s32 dataLength)
    {
      const s32 bytesRequired = RoundUp<s32>(dataLength, MEMORY_ALIGNMENT);

      s32 numBytesAllocated;

      u8 * segment = reinterpret_cast<u8*>( memoryStack.Allocate(bytesRequired, numBytesAllocated) );

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::AllocateRaw", "Could not add data");

      return segment;
    } // static void* PushBack(const SerializedBuffer::DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength, MemoryStack *scratch)

    void* SerializedBuffer::Allocate(const char *typeName, const char *objectName, const s32 dataLength)
    {
      s32 totalLength = dataLength + 2*DESCRIPTION_STRING_LENGTH;
      void *segment = AllocateRaw(totalLength);

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::Allocate", "Could not add data");

      if(SerializeDescriptionStrings(typeName, objectName, &segment, totalLength) != RESULT_OK)
        return NULL;

      return segment;
    }

    void* SerializedBuffer::PushBackString(const char * format, ...)
    {
      // After switching to a better board, this will work
#if 0
      const s32 outputStringLength = 1024;
      static char outputString[outputStringLength];

      va_list arguments;
      va_start(arguments, format);
      snprintf(outputString, outputStringLength, format, arguments);
      va_end(arguments);

      s32 usedLength = outputStringLength;
      for(s32 i=0; i<outputStringLength; i++) {
        if(outputString[i] == '\0') {
          usedLength = i;
          break;
        }
      }

      if(usedLength == outputStringLength) {
        outputString[outputStringLength-1] = '\0';
      }

      return PushBack("String", DATA_TYPE_STRING, &outputString[0], usedLength);
#else
      // Temporary hack

      s32 usedLength = -1;
      for(s32 i=0; i<1024; i++) {
        if(format[i] == '\0') {
          usedLength = i;
          break;
        }
      }

      if(usedLength == -1)
        return NULL;

      void* segment = Allocate("String", "String", usedLength);

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::PushBackString", "Could not add data");

      memcpy(segment, format, usedLength);

      return segment;
#endif
    }

    bool SerializedBuffer::IsValid() const
    {
      return memoryStack.IsValid();
    }

    const MemoryStack& SerializedBuffer::get_memoryStack() const
    {
      return memoryStack;
    }

    MemoryStack& SerializedBuffer::get_memoryStack()
    {
      return memoryStack;
    }

    SerializedBufferConstIterator::SerializedBufferConstIterator(const SerializedBuffer &serializedBuffer)
      : MemoryStackConstIterator(serializedBuffer.get_memoryStack())
    {
    }

    const void * SerializedBufferConstIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, const bool requireFillPatternMatch)
    {
      dataLength = -1;
      const char * segment = reinterpret_cast<const char*>( MemoryStackConstIterator::GetNext(dataLength, requireFillPatternMatch) );

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      *typeName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      dataLength -= SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      *objectName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      dataLength -= SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      return segment;
    }

    SerializedBufferIterator::SerializedBufferIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferIterator::GetNext(const char **typeName, const char **objectName, s32 &dataLength, const bool requireFillPatternMatch)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const
      void * segment = const_cast<void*>(SerializedBufferConstIterator::GetNext(typeName, objectName, dataLength, requireFillPatternMatch));

      return segment;
    }

    SerializedBufferReconstructingConstIterator::SerializedBufferReconstructingConstIterator(const SerializedBuffer &serializedBuffer)
      : MemoryStackReconstructingConstIterator(serializedBuffer.get_memoryStack())
    {
    }

    const void * SerializedBufferReconstructingConstIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect)
    {
      s32 trueSegmentLength;
      s32 reportedSegmentLength;
      const char * segment = reinterpret_cast<const char*>( MemoryStackReconstructingConstIterator::GetNext(trueSegmentLength, reportedSegmentLength) );

      if(trueSegmentLength == reportedSegmentLength) {
        isReportedSegmentLengthCorrect = true;
      } else {
        isReportedSegmentLengthCorrect = false;
      }

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      *typeName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      *objectName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      // TODO: return true, or reported?
      dataLength = reportedSegmentLength;

      return segment;
    }

    SerializedBufferReconstructingIterator::SerializedBufferReconstructingIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferReconstructingConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferReconstructingIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const
      void * segment = const_cast<void*>(SerializedBufferReconstructingConstIterator::GetNext(typeName, objectName, dataLength, isReportedSegmentLengthCorrect));

      return segment;
    }

    template<> s32 TotalArrayStringLengths<const char*>(const Array<const char*> &in)
    {
      const s32 inHeight = in.get_size(0);
      const s32 inWidth = in.get_size(1);

      s32 stringsLength = sizeof(s32);
      for(s32 y=0; y<inHeight; y++) {
        char const * const * restrict pIn = in.Pointer(y,0);
        for(s32 x=0; x<inWidth; x++) {
          stringsLength += strlen(pIn[x]) + 1;
        }
      }

      return stringsLength;
    }

    template<> s32 TotalArrayStringLengths<char*>(const Array<char*> &in)
    {
      // Add const qualifier and call the const version
      const Array<const char*> constIn = *reinterpret_cast<const Array<const char*>*>(&in);
      return TotalArrayStringLengths(constIn);
    }

    template<> void CopyArrayStringsToBuffer<const char*>(const Array<const char*> &in, void ** buffer, s32 &bufferLength)
    {
      const s32 inHeight = in.get_size(0);
      const s32 inWidth = in.get_size(1);

      const s32 stringsLength = TotalArrayStringLengths(in);

      // Copy the null terminated strings to the end

      // First copy the total size of the strings
      reinterpret_cast<u32*>(*buffer)[0] = stringsLength;
      *buffer = reinterpret_cast<u8*>(*buffer) + sizeof(u32);
      bufferLength -= sizeof(u32);

      // Then copy the strings
      for(s32 y=0; y<inHeight; y++) {
        char const * const * restrict pIn = in.Pointer(y,0);
        for(s32 x=0; x<inWidth; x++) {
          const s32 curStringLength = static_cast<s32>( strlen(pIn[x]) + 1 );

          memcpy(*buffer, pIn[x], curStringLength);

          *buffer = reinterpret_cast<u8*>(*buffer) + curStringLength;
          bufferLength -= curStringLength;
        }
      }
    }

    template<> void CopyArrayStringsToBuffer<char*>(const Array<char*> &in, void ** buffer, s32 &bufferLength)
    {
      // Just cast the "char *" as "const char *", and serialize
      const Array<const char*> constIn = *reinterpret_cast<const Array<const char*> *>(&in);
      return CopyArrayStringsToBuffer<const char*>(constIn, buffer, bufferLength);
    }

    template<> Result CopyArrayStringsFromBuffer<char*>(Array<char*> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // Copy the null terminated strings from the end

      // First, get the total size of the strings
      const u32 stringsLength = reinterpret_cast<u32*>(*buffer)[0];
      *buffer = reinterpret_cast<u8*>(*buffer) + sizeof(u32);
      bufferLength -= sizeof(u32);

      // Allocate the space for all strings
      char * stringBuffer = reinterpret_cast<char*>( memory.Allocate(stringsLength) );

      AnkiConditionalErrorAndReturnValue(stringBuffer,
        RESULT_FAIL_OUT_OF_MEMORY, "SerializedBuffer::DeserializeRawArray", "Out of memory");

      s32 stringsLengthLeft = stringsLength;

      // Copy each string and set the pointer for Array out
      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);
      for(s32 y=0; y<outHeight; y++) {
        char ** restrict pOut = out.Pointer(y,0);
        for(s32 x=0; x<outWidth; x++) {
          const s32 curStringLength = static_cast<s32>( strlen(reinterpret_cast<const char*>(*buffer)) + 1 );

          AnkiConditionalErrorAndReturnValue(curStringLength <= stringsLengthLeft,
            RESULT_FAIL, "SerializedBuffer::DeserializeRawArray", "Not enought bytes left to set the array");

          pOut[x] = stringBuffer;
          memcpy(stringBuffer, *buffer, curStringLength);
          stringBuffer += curStringLength;

          *buffer = reinterpret_cast<u8*>(*buffer) + curStringLength;
          bufferLength -= curStringLength;
          stringsLengthLeft -= curStringLength;
        }
      }

      return RESULT_OK;
    }

    template<> Result CopyArrayStringsFromBuffer<const char*>(Array<const char*> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // Just cast the "const char *" as "char *", and deserialize
      Array<char*> nonConstIn = *reinterpret_cast<Array<char*> *>(&out);
      return CopyArrayStringsFromBuffer<char*>(nonConstIn, buffer, bufferLength, memory);
    }
  } // namespace Embedded
} // namespace Anki
