/**
File: serialize_declarations.h
Author: Peter Barnum
Created: 2013

Utilities for serializing data into a buffer

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/flags_declarations.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Declarations ---
    // When transmitting a serialized buffer, the header and footer of the entire buffer are each 8 bytes
    // These just contain a validation pattern, not data
    static const s32 SERIALIZED_BUFFER_HEADER_LENGTH = 8;
    static const s32 SERIALIZED_BUFFER_FOOTER_LENGTH = 8;

    // This patterns are very unlikely to appear in an image
    // In addition, all values are different, which makes parsing easier
    static const u32 SERIALIZED_BUFFER_HEADER[SERIALIZED_BUFFER_HEADER_LENGTH] = {0xFF, 0x00, 0xFE, 0x02, 0xFD, 0x03, 0x04, 0xFC};
    static const u32 SERIALIZED_BUFFER_FOOTER[SERIALIZED_BUFFER_FOOTER_LENGTH] = {0xFE, 0x00, 0xFD, 0x02, 0xFC, 0x03, 0x04, 0xFB};

    // A SerializedBuffer is used to store data
    // Use a MemoryStackIterator to read out the data
    class SerializedBuffer
    {
    public:

      // The header and footer for individual segments within a SerializedBuffer
      // These only contain data and CRCs
      static const s32 SERIALIZED_SEGEMENT_HEADER_LENGTH = 8;
      static const s32 SERIALIZED_SEGMENT_FOOTER_LENGTH = 4;

      enum DataType
      {
        DATA_TYPE_UNKNOWN = 0,
        DATA_TYPE_RAW = 1,
        DATA_TYPE_BASIC_TYPE = 2,
        DATA_TYPE_ARRAY = 3
      };

      template<typename Type> static Result EncodeBasicType(u32 &code);
      static Result DecodeBasicType(const u32 code, u8 &size, bool &isInteger, bool &isSigned, bool &isFloat);

      // If the void* buffer is already allocated, use flags = Flags::Buffer(false,true,true)
      SerializedBuffer(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(false,true,false));

      void* PushBack(void * data, s32 dataLength);

      void* PushBack(void * header, s32 headerLength, void * data, s32 dataLength);

      template<typename Type> Result PushBack(Type &value);

      template<typename Type> Result PushBack(Array<Type> &value);

      bool IsValid() const;

      const MemoryStack& get_memoryStack() const;

      MemoryStack& get_memoryStack();

    protected:
      MemoryStack memoryStack;
    }; // class SerializedBuffer

    class SerializedBufferConstIterator : public MemoryStackConstIterator
    {
    public:
      SerializedBufferConstIterator(const SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackConstIterator::GetNext(), plus:
      // 1. Checks the CRC code and returns NULL if it fails
      // 2. dataLength is the number of bytes of the returned buffer
      const void * GetNext(s32 &dataLength, SerializedBuffer::DataType &type);
    }; // class MemoryStackConstIterator

    class SerializedBufferIterator : public SerializedBufferConstIterator
    {
    public:
      SerializedBufferIterator(SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackIterator::GetNext(), plus:
      // 1. Checks the CRC code and returns NULL if it fails
      // 2. dataLength is the number of bytes of the returned buffer
      void * GetNext(const bool swapEndian, s32 &dataLength, SerializedBuffer::DataType &type);
    }; // class MemoryStackConstIterator
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_DECLARATIONS_H_