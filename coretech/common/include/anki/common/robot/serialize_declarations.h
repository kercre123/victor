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
    class VisionMarker;

    // #pragma mark --- Declarations ---
    // When transmitting a serialized buffer, the header and footer of the entire buffer are each 8 bytes
    // These just contain a validation pattern, not data
    static const s32 SERIALIZED_BUFFER_HEADER_LENGTH = 8;
    static const s32 SERIALIZED_BUFFER_FOOTER_LENGTH = 8;

    // This patterns are very unlikely to appear in an image
    // In addition, all values are different, which makes parsing easier
    static const u8 SERIALIZED_BUFFER_HEADER[SERIALIZED_BUFFER_HEADER_LENGTH] = {0xFF, 0x00, 0xFE, 0x02, 0xFD, 0x03, 0x04, 0xFC};
    static const u8 SERIALIZED_BUFFER_FOOTER[SERIALIZED_BUFFER_FOOTER_LENGTH] = {0xFE, 0x00, 0xFD, 0x02, 0xFC, 0x03, 0x04, 0xFB};

    // A SerializedBuffer is used to store data
    // Use a SerializedBufferIterator to read out the data
    class SerializedBuffer
    {
    public:

      // The header and footer for individual segments within a SerializedBuffer
      // These only contain data and CRCs
      static const s32 SERIALIZED_SEGMENT_HEADER_LENGTH = 8;
      static const s32 SERIALIZED_SEGMENT_FOOTER_LENGTH = 4;

      static const s32 CUSTOM_TYPE_STRING_LENGTH = 32;

      enum DataType
      {
        DATA_TYPE_UNKNOWN = 0,
        DATA_TYPE_RAW = 1,
        DATA_TYPE_BASIC_TYPE_BUFFER = 2,
        DATA_TYPE_ARRAY = 3,
        DATA_TYPE_ARRAYSLICE = 4,
        DATA_TYPE_STRING = 5,
        DATA_TYPE_LIST = 6,
        DATA_TYPE_CUSTOM = 7 //< A custom type is defined by a unique 31-character string
      };

      // Stores the eight-byte code of a basic data type buffer (like a buffer of unsigned shorts)
      class EncodedBasicTypeBuffer
      {
      public:
        const static s32 CODE_SIZE = 2;
        u32 code[EncodedBasicTypeBuffer::CODE_SIZE];
      };

      // Stores the forty-byte code for an Array
      class EncodedArray
      {
      public:
        const static s32 CODE_SIZE = 5;
        u32 code[EncodedArray::CODE_SIZE];
      };

      // Stores the ??-byte code for an ArraySlice
      class EncodedArraySlice
      {
      public:
        const static s32 CODE_SIZE = 11;
        u32 code[EncodedArraySlice::CODE_SIZE];
      };

      //
      // Various static functions for encoding and decoding serialized objects
      //

      // Encode or decode the four-byte code of a basic data type (like a single unsigned shorts)
      template<typename Type> static Result EncodeBasicType(u32 &code);
      static Result DecodeBasicType(const u32 code, u8 &size, bool &isInteger, bool &isSigned, bool &isFloat);

      // Encode or decode the eight-byte code of a basic data type buffer (like a buffer of unsigned shorts)
      template<typename Type> static Result EncodeBasicTypeBuffer(const s32 numElements, EncodedBasicTypeBuffer &code);
      static Result DecodeBasicTypeBuffer(const EncodedBasicTypeBuffer &code, u8 &size, bool &isInteger, bool &isSigned, bool &isFloat, s32 &numElements);

      // Encode or decode the forty-byte code of an Array
      template<typename Type> static Result EncodeArrayType(const Array<Type> &in, EncodedArray &code);
      static Result DecodeArrayType(const EncodedArray &code, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, u8 &basicType_size, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat);

      // Encode or decode the ??-byte code of an ArraySlice
      template<typename Type> static Result EncodeArraySliceType(const ArraySlice<Type> &in, EncodedArraySlice &code);
      static Result DecodeArraySliceType(const EncodedArraySlice &code, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, s32 &ySlice_start, s32 &ySlice_increment, s32 &ySlice_end, s32 &xSlice_start, s32 &xSlice_increment, s32 &xSlice_end, u8 &basicType_size, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat);

      //// Encode or decode the forty-byte code of an Array
      //template<typename Type> static Result EncodeCustomType(const char * name, EncodedCustomType &code);
      //static Result DecodeCustomType(const EncodedCustomType &code, const char * name);

      // Helper functions to serialize or deserialize an array
      template<typename Type> static Result SerializeArray(const Array<Type> &in, void * data, const s32 dataLength);
      template<typename Type> static Result DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory);

      // Helper functions to serialize or deserialize an array slice
      template<typename Type> static Result SerializeArraySlice(const ArraySlice<Type> &in, void * data, const s32 dataLength);
      template<typename Type> static Result DeserializeArraySlice(const void * data, const s32 dataLength, ArraySlice<Type> &out, MemoryStack &memory);

      // Helper functions to serialize or deserialize an FixedLengthList (which is just a type of ArraySlice)
      template<typename Type> static Result SerializeFixedLengthList(const FixedLengthList<Type> &in, void * data, const s32 dataLength);
      template<typename Type> static Result DeserializeFixedLengthList(const void * data, const s32 dataLength, FixedLengthList<Type> &out, MemoryStack &memory);

      // Search rawBuffer for the 8-byte serialized buffer headers and footers.
      // startIndex is the location after the header, or -1 if one is not found
      // endIndex is the location before the footer, or -1 if one is not found
      static Result FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex);

      // Warning: Complex structures or classes require an explicit specialization
      // Updates the buffer pointer and length before returning
      template<typename Type> static Result SerializeRaw(const Type &in,                  void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRaw(const Array<Type> &in,           void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRaw(const ArraySlice<Type> &in,      void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRaw(const FixedLengthList<Type> &in, void ** buffer, s32 &bufferLength);

      // Warning: Complex structures or classes require an explicit specialization
      // Updates the buffer pointer and length before returning
      template<typename Type> static Type                  DeserializeRaw(               void ** buffer, s32 &bufferLength);
      template<typename Type> static Array<Type>           DeserializeRawArray(          void ** buffer, s32 &bufferLength, MemoryStack &memory);
      template<typename Type> static ArraySlice<Type>      DeserializeRawArraySlice(     void ** buffer, s32 &bufferLength, MemoryStack &memory);
      template<typename Type> static FixedLengthList<Type> DeserializeRawFixedLengthList(void ** buffer, s32 &bufferLength, MemoryStack &memory);

      //
      // Non-static functions
      //

      SerializedBuffer();

      // If the void* buffer is already allocated, use flags = Flags::Buffer(false,true,true)
      SerializedBuffer(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(false,true,false));

      // Don't allocate any headers or footers
      void* PushBackRaw(const void * data, const s32 dataLength);

      // Push back some data and/or a header
      void* PushBack(const void * data, s32 dataLength);
      void* PushBack(const DataType type, const void * data, s32 dataLength);
      void* PushBack(const void * header, s32 headerLength, const void * data, s32 dataLength);
      void* PushBack(const DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength);

      // Push back a custom type, defined by a unique customTypeName string
      // Could be made fancy and design patterns, if neccesary
      //
      // WARNING: CRC broken
      //
      // The returned pointer is the location of the start of the allocated buffer
      // afterHeader is a pointer to the start of the data buffer, after the header (add your custom payload here)
      void* PushBack(const char * customTypeName, const s32 dataLength, void ** afterHeader);

      // Push back a null-terminated string. Works like printf().
      void* PushBackString(const char * format, ...);

      // Note that dataLength should be numel(data)*sizeof(Type)
      // This is to make this call compatible with the standard void* PushBack()
      template<typename Type> Type* PushBack(const Type *data, const s32 dataLength);

      // Push back an Array
      template<typename Type> void* PushBack(const Array<Type> &in);

      // Push back an ArraySlice
      // WARNING: CRC code generation is broken
      template<typename Type> void* PushBack(const ArraySlice<Type> &in);

      // Push back a FixedLengthList
      //template<typename Type> void* PushBack(const FixedLengthList<Type> &in);

      bool IsValid() const;

      const MemoryStack& get_memoryStack() const;

      MemoryStack& get_memoryStack();

    protected:
      MemoryStack memoryStack;

      // The returned pointer is the location of the start of the allocated buffer
      // afterHeader is a pointer to the start of the data buffer, after the header
      void* PushBack_Generic(const SerializedBuffer::DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength, void ** afterHeader);
    }; // class SerializedBuffer

    class SerializedBufferConstIterator : public MemoryStackConstIterator
    {
    public:
      SerializedBufferConstIterator(const SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackConstIterator::GetNext(), plus:
      // 1. dataLength is the number of bytes of the returned buffer
      // 2. If requireCRCmatch==true, checks the CRC code and returns NULL if it fails
      const void * GetNext(s32 &dataLength, SerializedBuffer::DataType &type, const bool requireFillPatternMatch=true, const bool requireCRCmatch=true);
    }; // class MemoryStackConstIterator

    class SerializedBufferIterator : public SerializedBufferConstIterator
    {
    public:
      SerializedBufferIterator(SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackIterator::GetNext(), plus:
      // 1. dataLength is the number of bytes of the returned buffer
      // 2. If requireCRCmatch==true, checks the CRC code and returns NULL if it fails
      void * GetNext(s32 &dataLength, SerializedBuffer::DataType &type, const bool requireFillPatternMatch=true, const bool requireCRCmatch=true);
    }; // class MemoryStackConstIterator

    class SerializedBufferRawConstIterator : public MemoryStackRawConstIterator
    {
      // See MemoryStackRawConstIterator
    public:
      SerializedBufferRawConstIterator(const SerializedBuffer &serializedBuffer);

      const void * GetNext(s32 &dataLength, SerializedBuffer::DataType &type, bool &isReportedSegmentLengthCorrect);
    }; // class MemoryStackConstIterator

    class SerializedBufferRawIterator : public SerializedBufferRawConstIterator
    {
      // See MemoryStackRawConstIterator
    public:
      SerializedBufferRawIterator(SerializedBuffer &serializedBuffer);

      void * GetNext(s32 &dataLength, SerializedBuffer::DataType &type, bool &isReportedSegmentLengthCorrect);
    }; // class MemoryStackConstIterator
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_DECLARATIONS_H_
