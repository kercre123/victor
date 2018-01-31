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

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/flags_declarations.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/array2d_declarations.h"

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
      // This is the string length for both an object name, and a custom type name
      static const s32 DESCRIPTION_STRING_LENGTH = 32;

      // Stores the eight-byte code of a basic data type buffer (like a buffer of unsigned shorts)
      class EncodedBasicTypeBuffer
      {
      public:
        const static s32 CODE_LENGTH = 2*sizeof(u32);
        //u32 code[EncodedBasicTypeBuffer::CODE_SIZE];

        template<typename Type> static Result Serialize(const bool updateBufferPointer, const s32 numElements, void ** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
        static Result Deserialize(const bool updateBufferPointer, u16 &sizeOfType, bool &isBasicType, bool &isInteger, bool &isSigned, bool &isFloat, bool &isString, s32 &numElements, void** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
      };

      // Stores the forty-byte code for an Array
      class EncodedArray
      {
      public:
        const static s32 CODE_LENGTH = 6*sizeof(u32);
        //u32 code[EncodedArray::CODE_SIZE];

        template<typename Type> static Result Serialize(const bool updateBufferPointer, const Array<Type> &in, void ** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
        static Result Deserialize(const bool updateBufferPointer, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, u16 &basicType_sizeOfType, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat, bool &basicType_isString, s32 &basicType_numElements, void** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
      };

      // Stores the ??-byte code for an ArraySlice
      class EncodedArraySlice
      {
      public:
        const static s32 CODE_LENGTH = 12*sizeof(u32);
        //u32 code[EncodedArraySlice::CODE_SIZE];

        template<typename Type> static Result Serialize(const bool updateBufferPointer, const ConstArraySlice<Type> &in, void ** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
        static Result Deserialize(const bool updateBufferPointer, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, s32 &ySlice_start, s32 &ySlice_increment, s32 &ySlice_size, s32 &xSlice_start, s32 &xSlice_increment, s32 &xSlice_size, u16 &basicType_sizeOfType, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat, bool &basicType_isString, s32 &basicType_numElements, void** buffer, s32 &bufferLength); // Updates the buffer pointer and length before returning
      };

      //
      // Various static functions for encoding and decoding serialized objects
      //

      // Search rawBuffer for the 8-byte serialized buffer headers and footers.
      // startIndex is the location after the header, or -1 if one is not found
      // endIndex is the location before the footer, or -1 if one is not found
      static Result FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex);

      // The first part of an object, after the header, is its 0-31 character name
      // The objectName can either be null, or a buffer of at least 32 bytes
      static Result SerializeDescriptionStrings(const char *typeNme, const char *objectName, void ** buffer, s32 &bufferLength);
      static Result DeserializeDescriptionStrings(    char *typeName,      char *objectName, void ** buffer, s32 &bufferLength);

      // Warning: Complex structures or classes require an explicit specialization
      // Updates the buffer pointer and length before returning
      template<typename Type> static Result SerializeRawBasicType(      const char *objectName, const Type &in,                        void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRawBasicType(      const char *objectName, const Type *in, const s32 numElements, void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRawArray(          const char *objectName, const Array<Type> &in,                 void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRawArraySlice(     const char *objectName, const ConstArraySlice<Type> &in,       void ** buffer, s32 &bufferLength);
      template<typename Type> static Result SerializeRawFixedLengthList(const char *objectName, const FixedLengthList<Type> &in,       void ** buffer, s32 &bufferLength);

      // Warning: Complex structures or classes require an explicit specialization
      // Updates the buffer pointer and length before returning
      template<typename Type> static Type                  DeserializeRawBasicType(      char *objectName, void ** buffer, s32 &bufferLength);
      template<typename Type> static Type*                 DeserializeRawBasicType(      char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory);
      template<typename Type> static Array<Type>           DeserializeRawArray(          char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory);
      template<typename Type> static ArraySlice<Type>      DeserializeRawArraySlice(     char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory);
      template<typename Type> static FixedLengthList<Type> DeserializeRawFixedLengthList(char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory);

      //
      // Non-static functions
      //

      SerializedBuffer();

      // If the void* buffer is already allocated, use flags = Flags::Buffer(false,true,true)
      SerializedBuffer(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(false,true,false));

      // objectName and typeName may each be up to 31 characters
      // Allocates dataLength + 2*DESCRIPTION_STRING_LENGTH bytes
      void* Allocate(const char *typeName, const char *objectName, const s32 dataLength);

      template<typename Type> void* PushBack(const char *objectName, const Type *data, const s32 numElements);

      // Push back an Array
      template<typename Type> void* PushBack(const char *objectName, const Array<Type> &in);

      // Push back an ArraySlice
      // WARNING: CRC code generation is broken
      template<typename Type> void* PushBack(const char *objectName, const ArraySlice<Type> &in);

      // Push back an FixedLengthList
      // WARNING: CRC code generation is broken
      template<typename Type> void* PushBack(const char *objectName, const FixedLengthList<Type> &in);

      // Push back a null-terminated string. Works like printf().
      void* PushBackString(const char * format, ...);

      bool IsValid() const;

      const MemoryStack& get_memoryStack() const;

      MemoryStack& get_memoryStack();

    protected:
      MemoryStack memoryStack;

      // Allocate memory (dataLength must include the memory to store the objectName and typeName)
      // Only use this for hierarchical structures
      void* AllocateRaw(const s32 dataLength);

      // The first part of an object, after the header, is its 0-31 character name
      // The objectName can either be null, or a buffer of at least 32 bytes
      static Result SerializeOneDescriptionString(const char *description, void ** buffer, s32 &bufferLength);
      static Result DeserializeOneDescriptionString(    char *description, void ** buffer, s32 &bufferLength);
    }; // class SerializedBuffer

    class SerializedBufferConstIterator : public MemoryStackConstIterator
    {
    public:
      SerializedBufferConstIterator(const SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackConstIterator::GetNext(), plus:
      // 1. dataLength is the number of bytes of the returned buffer
      // 2. If requireCRCmatch==true, checks the CRC code and returns NULL if it fails
      const void * GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, const bool requireFillPatternMatch=true);
    }; // class MemoryStackConstIterator

    class SerializedBufferIterator : public SerializedBufferConstIterator
    {
    public:
      SerializedBufferIterator(SerializedBuffer &serializedBuffer);

      // Same as the standard MemoryStackIterator::GetNext(), plus:
      // 1. dataLength is the number of bytes of the returned buffer
      // 2. If requireCRCmatch==true, checks the CRC code and returns NULL if it fails
      void * GetNext(const char **typeName, const char **objectName, s32 &dataLength, const bool requireFillPatternMatch=true);
    }; // class MemoryStackConstIterator

    class SerializedBufferReconstructingConstIterator : public MemoryStackReconstructingConstIterator
    {
      // See MemoryStackReconstructingConstIterator
    public:
      SerializedBufferReconstructingConstIterator(const SerializedBuffer &serializedBuffer);

      const void * GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect);
    }; // class MemoryStackConstIterator

    class SerializedBufferReconstructingIterator : public SerializedBufferReconstructingConstIterator
    {
      // See MemoryStackReconstructingConstIterator
    public:
      SerializedBufferReconstructingIterator(SerializedBuffer &serializedBuffer);

      void * GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect);
    };

    // Returns the total number of bytes in all null-terminated strings (including the null termination character), plus 4 bytes.
    // Returns 0 if not a string array.
    template<typename Type> s32 TotalArrayStringLengths(const Array<Type> &in);

    // Serialize strings
    // Copy all null-terminated strings from the input Array to the buffer
    // Does nothing if not a string array
    template<typename Type> void CopyArrayStringsToBuffer(const Array<Type> &in, void ** buffer, s32 &bufferLength);

    // Deserialize strings
    // Does nothing if not a string array
    template<typename Type> Result CopyArrayStringsFromBuffer(Array<Type> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_DECLARATIONS_H_
