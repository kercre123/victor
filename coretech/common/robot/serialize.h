/**
File: serialize.h
Author: Peter Barnum
Created: 2013

Definitions for serialize_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
#define _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_

#include "coretech/common/robot/serialize_declarations.h"
#include "coretech/common/robot/flags.h"
#include "coretech/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark

    template<typename Type> Result SerializedBuffer::EncodedBasicTypeBuffer::Serialize(const bool updateBufferPointer, const s32 numElements, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      u32 firstElement = 0;

      if(Flags::TypeCharacteristics<Type>::isBasicType) {
        firstElement |= 1;
      }

      if(Flags::TypeCharacteristics<Type>::isInteger) {
        firstElement |= (1<<1);
      }

      if(Flags::TypeCharacteristics<Type>::isSigned) {
        firstElement |= (1<<2);
      }

      if(Flags::TypeCharacteristics<Type>::isFloat) {
        firstElement |= (1<<3);
      }

      if(Flags::TypeCharacteristics<Type>::isString) {
        firstElement |= (1<<4);
      }

      // TODO: there's room in the firstElement for additional flags

      AnkiAssert(sizeof(Type) <= 0xFFFF);

      firstElement |= (sizeof(Type) & 0xFFFF) << 16;

      reinterpret_cast<u32*>(*buffer)[0] = firstElement;
      reinterpret_cast<u32*>(*buffer)[1] = numElements;

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH;;
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::EncodedArray::Serialize(const bool updateBufferPointer, const Array<Type> &in, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedArray::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::EncodedArray", "in Array is invalid");

      const s32 numElements = in.get_size(0) * in.get_size(1);

      EncodedBasicTypeBuffer::Serialize<Type>(false, numElements, buffer, bufferLength);

      reinterpret_cast<u32*>(*buffer)[2] = in.get_size(0);
      reinterpret_cast<u32*>(*buffer)[3] = in.get_size(1);
      reinterpret_cast<u32*>(*buffer)[4] = in.get_stride();
      reinterpret_cast<u32*>(*buffer)[5] = in.get_flags().get_rawFlags();

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedArray::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedArray::CODE_LENGTH;
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::EncodedArraySlice::Serialize(const bool updateBufferPointer, const ConstArraySlice<Type> &in, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < SerializedBuffer::EncodedArraySlice::CODE_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::EncodedArraySlice", "in Array is invalid");

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 ySize = ySlice.get_size();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xSize = xSlice.get_size();

      EncodedArray::Serialize<Type>(false, in.get_array(), buffer, bufferLength);

      reinterpret_cast<u32*>(*buffer)[6] = *reinterpret_cast<const u32*>(&yStart);
      reinterpret_cast<u32*>(*buffer)[7] = *reinterpret_cast<const u32*>(&yIncrement);
      reinterpret_cast<u32*>(*buffer)[8] = *reinterpret_cast<const u32*>(&ySize);

      reinterpret_cast<u32*>(*buffer)[9] = *reinterpret_cast<const u32*>(&xStart);
      reinterpret_cast<u32*>(*buffer)[10] = *reinterpret_cast<const u32*>(&xIncrement);
      reinterpret_cast<u32*>(*buffer)[11]= *reinterpret_cast<const u32*>(&xSize);

      if(updateBufferPointer) {
        *buffer = reinterpret_cast<u8*>(*buffer) + SerializedBuffer::EncodedArraySlice::CODE_LENGTH;
        bufferLength -= SerializedBuffer::EncodedArraySlice::CODE_LENGTH;
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRawBasicType(const char *objectName, const Type &in, void ** buffer, s32 &bufferLength)
    {
      return SerializeRawBasicType(objectName, &in, 1, buffer, bufferLength);
    }

    template<typename Type> Result SerializedBuffer::SerializeRawBasicType(const char *objectName, const Type *in, const s32 numElements, void ** buffer, s32 &bufferLength)
    {
      if(SerializeDescriptionStrings("Basic Type Buffer", objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      SerializedBuffer::EncodedBasicTypeBuffer::Serialize<Type>(true, numElements, buffer, bufferLength);

      memcpy(*buffer, in, numElements*sizeof(Type));

      *buffer = reinterpret_cast<u8*>(*buffer) + numElements*sizeof(Type);
      bufferLength -= numElements*sizeof(Type);

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRawArray(const char *objectName, const Array<Type> &in, void ** buffer, s32 &bufferLength)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeRawArraySlice", "in ArraySlice is not Valid");

      // If this is a string array, add the sizes of the null terminated strings (or zero otherwise)
      const s32 stringsLength = TotalArrayStringLengths<Type>(in);

      const s32 numRequiredBytes = in.get_size(0)*in.get_stride() + SerializedBuffer::EncodedArray::CODE_LENGTH + stringsLength;

      AnkiConditionalErrorAndReturnValue(bufferLength >= numRequiredBytes,
        RESULT_FAIL_OUT_OF_MEMORY, "SerializedBuffer::SerializeRawArray", "buffer needs at least %d bytes", numRequiredBytes);

      if(SerializeDescriptionStrings("Array", objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      SerializedBuffer::EncodedArray::Serialize<Type>(true, in, buffer, bufferLength);

      const s32 numElements = in.get_size(0) * in.get_size(1);

      if(numElements > 0) {
        memcpy(*buffer, in.Pointer(0,0), in.get_stride()*in.get_size(0));

        *buffer = reinterpret_cast<u8*>(*buffer) + in.get_stride()*in.get_size(0);
        bufferLength -= in.get_stride()*in.get_size(0);

        // If this is a string array, copy the null terminated strings to the end (or do nothing otherwise)
        CopyArrayStringsToBuffer<Type>(in, buffer, bufferLength);
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRawArraySlice(const char *objectName, const ConstArraySlice<Type> &in, void ** buffer, s32 &bufferLength)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeRawArraySlice", "in ArraySlice is not Valid");

      if(SerializeDescriptionStrings("ArraySlice", objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      // NOTE: these parameters are the size that will be transmitted, not the original size
      const u32 height = in.get_ySlice().get_size();
      const u32 width = in.get_xSlice().get_size();
      const u32 stride = width*sizeof(Type);

      const s32 numRequiredBytes = height*stride + SerializedBuffer::EncodedArraySlice::CODE_LENGTH;

      AnkiConditionalErrorAndReturnValue(bufferLength >= numRequiredBytes,
        RESULT_FAIL_OUT_OF_MEMORY, "SerializedBuffer::SerializeRawArraySlice", "buffer needs at least %d bytes", numRequiredBytes);

      SerializedBuffer::EncodedArraySlice::Serialize<Type>(true, in, buffer, bufferLength);

      // TODO: this could be done more efficiently
      Type * restrict pDataType = reinterpret_cast<Type*>(*buffer);
      s32 iData = 0;

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 ySize = ySlice.get_size();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xSize = xSlice.get_size();

      for(s32 iy=0; iy<ySize; iy++) {
        const s32 y = yStart + iy * yIncrement;
        const Type * restrict pInData = in.get_array().Pointer(y, 0);

        for(s32 ix=0; ix<xSize; ix++) {
          const s32 x = xStart + ix * xIncrement;
          pDataType[iData] = pInData[x];
          iData++;
        }
      }

      AnkiAssert(iData == stride*height);

      *buffer = reinterpret_cast<u8*>(*buffer) + stride*height;
      bufferLength -= stride*height;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRawFixedLengthList(const char *objectName, const FixedLengthList<Type> &in, void ** buffer, s32 &bufferLength)
    {
      return SerializeRawArraySlice<Type>(objectName, *static_cast<const ConstArraySlice<Type>*>(&in), buffer, bufferLength);
    }

    template<typename Type> Type SerializedBuffer::DeserializeRawBasicType(char *objectName, void ** buffer, s32 &bufferLength)
    {
      // TODO: check if description is valid
      DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength);

      // TODO: check if encoded type is valid
      u16 sizeOfType;
      bool isBasicType;
      bool isInteger;
      bool isSigned;
      bool isFloat;
      bool isString;
      s32 numElements;
      EncodedBasicTypeBuffer::Deserialize(true, sizeOfType, isBasicType, isInteger, isSigned, isFloat, isString, numElements, buffer, bufferLength);

      const Type var = *reinterpret_cast<Type*>(*buffer);

      AnkiConditionalErrorAndReturnValue(sizeOfType < 10000 && numElements > 0 && numElements < 1000000,
        Type(), "SerializedBuffer::DeserializeRawBasicType", "Unreasonable deserialized values");

      *buffer = reinterpret_cast<u8*>(*buffer) + sizeOfType*numElements;
      bufferLength -= sizeOfType*numElements;

      return var;
    }

    template<typename Type> Type* SerializedBuffer::DeserializeRawBasicType(char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // TODO: check if description is valid
      DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength);

      // TODO: check if encoded type is valid
      u16 sizeOfType;
      bool isBasicType;
      bool isInteger;
      bool isSigned;
      bool isFloat;
      bool isString;
      s32 numElements;
      EncodedBasicTypeBuffer::Deserialize(true, sizeOfType, isBasicType, isInteger, isSigned, isFloat, isString, numElements, buffer, bufferLength);

      AnkiConditionalErrorAndReturnValue(sizeOfType < 10000 && numElements > 0 && numElements < 1000000,
        NULL, "SerializedBuffer::DeserializeRawBasicType", "Unreasonable deserialized values");

      const s32 numBytes = numElements*sizeOfType;
      Type *var = reinterpret_cast<Type*>( memory.Allocate(numBytes) );

      memcpy(var, *buffer, numBytes);

      *buffer = reinterpret_cast<u8*>(*buffer) + numBytes;
      bufferLength -= numBytes;

      return var;
    }

    template<typename Type> Array<Type> SerializedBuffer::DeserializeRawArray(char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // TODO: check if description is valid
      DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength);

      // TODO: check if encoded type is valid
      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      u16 basicType_sizeOfType;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      bool basicType_isString;
      s32 basicType_numElements;
      EncodedArray::Deserialize(true, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, buffer, bufferLength);

      AnkiConditionalErrorAndReturnValue(
        height >= 0 && height < s32(1e9) &&
        width >= 0 && width < s32(2e9) &&
        stride > 0 && stride < s32(2e9) &&
        basicType_sizeOfType > 0 && basicType_sizeOfType < 10000 &&
        basicType_numElements >= 0 && basicType_numElements < s32(2e9),
        Array<Type>(), "SerializedBuffer::DeserializeRawArray", "Unreasonable deserialized values");

      if(width > 0) {
        AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
          Array<Type>(), "SerializedBuffer::DeserializeRawArray", "Parsed stride is not reasonable");
      }

      AnkiConditionalErrorAndReturnValue(bufferLength >= (height*stride),
        Array<Type>(), "SerializedBuffer::DeserializeRawArray", "Not enought bytes left to set the array");

      Array<Type> out = Array<Type>(height, width, memory);
      
      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        Array<Type>(), "SerializedBuffer::DeserializeRawArray", "Could not allocate array");

      const s32 numElements = out.get_size(0) * out.get_size(1);

      if(numElements > 0) {
        memcpy(out.Pointer(0,0), *buffer, height*stride);

        *buffer = reinterpret_cast<u8*>(*buffer) + height*stride;
        bufferLength -= height*stride;

        const Result stringCopyResult = CopyArrayStringsFromBuffer<Type>(out, buffer, bufferLength, memory);

        if(stringCopyResult != RESULT_OK) {
          return Array<Type>();
        }
      }

      return out;
    }

    template<typename Type> ArraySlice<Type> SerializedBuffer::DeserializeRawArraySlice(char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // TODO: check if description is valid
      DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength);

      // TODO: check if encoded type is valid
      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      s32 ySlice_start;
      s32 ySlice_increment;
      s32 ySlice_size;
      s32 xSlice_start;
      s32 xSlice_increment;
      s32 xSlice_size;
      u16 basicType_sizeOfType;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      bool basicType_isString;
      s32 basicType_numElements;
      EncodedArraySlice::Deserialize(true, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_size, xSlice_start, xSlice_increment, xSlice_size, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, buffer, bufferLength);

      AnkiConditionalErrorAndReturnValue(
        height >= 0 && height < s32(1e9) &&
        width >= 0 && width < s32(2e9) &&
        stride > 0 && stride < s32(2e9) &&
        ySlice_start >= 0 &&  ySlice_increment > 0 &&
        xSlice_start >= 0 && xSlice_increment > 0 && xSlice_increment < 1000000 &&
        basicType_sizeOfType > 0 && basicType_sizeOfType < 10000 &&
        basicType_numElements >= 0 && basicType_numElements < s32(2e9),

        ArraySlice<Type>(), "SerializedBuffer::DeserializeRawArraySlice", "Unreasonable deserialized values");

      if(width > 0) {
        AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
          ArraySlice<Type>(), "SerializedBuffer::DeserializeRawArraySlice", "Parsed stride is not reasonable");
      }

      const LinearSequence<s32> ySlice(ySlice_start, ySlice_increment, -1, ySlice_size);
      const LinearSequence<s32> xSlice(xSlice_start, xSlice_increment, -1, xSlice_size);

      AnkiConditionalErrorAndReturnValue(bufferLength >= static_cast<s32>(xSlice.get_size()*ySlice.get_size()*sizeof(Type)),
        ArraySlice<Type>(), "SerializedBuffer::DeserializeRawArraySlice", "Not enought bytes left to set the array");

      Array<Type> array(height, width, memory);

      AnkiConditionalErrorAndReturnValue(array.IsValid(),
        ArraySlice<Type>(), "SerializedBuffer::DeserializeRawArraySlice", "Out of memory");

      // TODO: this could be done more efficiently

      Type * restrict pDataType = reinterpret_cast<Type*>(*buffer);
      s32 iData = 0;

      for(s32 iy=0; iy<ySlice_size; iy++) {
        const s32 y = ySlice_start + iy * ySlice_increment;
        Type * restrict pArrayData = array.Pointer(y, 0);

        for(s32 ix=0; ix<xSlice_size; ix++) {
          const s32 x = xSlice_start + ix * xSlice_increment;
          pArrayData[x] = pDataType[iData];
          iData++;
        }
      }

      const s32 numElements = xSlice.get_size()*ySlice.get_size();

      AnkiConditionalErrorAndReturnValue(iData == numElements,
        ArraySlice<Type>(), "SerializedBuffer::DeserializeRawArraySlice", "Deserialization error");

      ArraySlice<Type> out = ArraySlice<Type>(array, ySlice, xSlice);

      *buffer = reinterpret_cast<u8*>(*buffer) + numElements*sizeof(Type);
      bufferLength -= numElements*sizeof(Type);

      return out;
    }

    template<typename Type> FixedLengthList<Type> SerializedBuffer::DeserializeRawFixedLengthList(char *objectName, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      ArraySlice<Type> arraySlice = SerializedBuffer::DeserializeRawArraySlice<Type>(objectName, buffer, bufferLength, memory);

      if(!arraySlice.IsValid())
        return FixedLengthList<Type>();

      FixedLengthList<Type> out;

      out.ySlice = arraySlice.get_ySlice();
      out.xSlice = arraySlice.get_xSlice();
      out.array = arraySlice.get_array();
      out.arrayData = out.array.Pointer(0,0);;

      return out;
    }

    template<typename Type> void* SerializedBuffer::PushBack(const char *objectName, const Type *data, const s32 numElements)
    {
      s32 totalDataLength = numElements*sizeof(Type) + SerializedBuffer::EncodedBasicTypeBuffer::CODE_LENGTH + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      void * const segmentStart = Allocate("Basic Type Buffer", objectName, totalDataLength);
      void * segment = reinterpret_cast<u8*>(segmentStart);

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      SerializeRawBasicType<Type>(objectName, data, numElements, &segment, totalDataLength);

      AnkiAssert(totalDataLength >= 0);

      return segmentStart;
    }

    template<typename Type> void* SerializedBuffer::PushBack(const char *objectName, const Array<Type> &in)
    {
      // If this is a string array, add the sizes of the null terminated strings (or zero otherwise)
      const s32 stringsLength = TotalArrayStringLengths<Type>(in);

      s32 totalDataLength = in.get_stride() * in.get_size(0) + SerializedBuffer::EncodedArray::CODE_LENGTH + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH + stringsLength;

      void * const segmentStart = Allocate("Array", objectName, totalDataLength);
      void * segment = segmentStart;

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      SerializeRawArray<Type>(objectName, in, &segment, totalDataLength);

      AnkiAssert(totalDataLength >= 0);

      return segmentStart;
    }

    template<typename Type> void* SerializedBuffer::PushBack(const char *objectName, const ArraySlice<Type> &in)
    {
      const u32 height = in.get_ySlice().get_size();
      const u32 width = in.get_xSlice().get_size();
      const u32 stride = width*sizeof(Type);

      s32 totalDataLength = height * stride + SerializedBuffer::EncodedArraySlice::CODE_LENGTH + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      void * const segmentStart = Allocate("ArraySlice", objectName, totalDataLength);
      void * segment = segmentStart;

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      SerializeRawArraySlice<Type>(objectName, in, &segment, totalDataLength);

      AnkiAssert(totalDataLength >= 0);

      return segmentStart;
    }

    template<typename Type> void* SerializedBuffer::PushBack(const char *objectName, const FixedLengthList<Type> &in)
    {
      return PushBack(objectName, *static_cast<const ArraySlice<Type>*>(&in));
    }

    template<typename Type> s32 TotalArrayStringLengths(const Array<Type> &in)
    {
      // Return nothing, since it's not a string array
      return 0;
    }

    template<typename Type> void CopyArrayStringsToBuffer(const Array<Type> &in, void ** buffer, s32 &bufferLength)
    {
      // Do nothing, since it's not a string array
    }

    template<typename Type> Result CopyArrayStringsFromBuffer(Array<Type> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // Do nothing, since it's not a string array
      return RESULT_OK;
    }

    // #pragma mark --- Array Specializations ---

    template<> s32 TotalArrayStringLengths<char*>(const Array<char*> &in);
    template<> s32 TotalArrayStringLengths<const char*>(const Array<const char*> &in);

    template<> void CopyArrayStringsToBuffer<char*>(const Array<char*> &in, void ** buffer, s32 &bufferLength);
    template<> void CopyArrayStringsToBuffer<const char*>(const Array<const char*> &in, void ** buffer, s32 &bufferLength);

    template<> Result CopyArrayStringsFromBuffer<char*>(Array<char*> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory);
    template<> Result CopyArrayStringsFromBuffer<const char*>(Array<const char*> &out, void ** buffer, s32 &bufferLength, MemoryStack &memory);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
