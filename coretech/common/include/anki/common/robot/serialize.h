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

#include "anki/common/robot/serialize_declarations.h"
#include "anki/common/robot/flags.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    template<typename Type> Result SerializedBuffer::EncodeBasicType(u32 &code)
    {
      code = 0;

      if(!Flags::TypeCharacteristics<Type>::isBasicType) {
        return RESULT_FAIL;
      }

      if(Flags::TypeCharacteristics<Type>::isInteger) {
        code |= 0xFF;
      }

      if(Flags::TypeCharacteristics<Type>::isSigned) {
        code |= 0xFF00;
      }

      if(Flags::TypeCharacteristics<Type>::isFloat) {
        code |= 0xFF0000;
      }

      code |= sizeof(Type) << 24;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::EncodeBasicTypeBuffer(const s32 numElements, EncodedBasicTypeBuffer &code)
    {
      SerializedBuffer::EncodeBasicType<Type>(code.code[0]);
      code.code[1] = numElements;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::EncodeArrayType(const Array<Type> &in, EncodedArray &code)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::EncodeArray", "in Array is invalid");

      if(SerializedBuffer::EncodeBasicType<Type>(code.code[0]) != RESULT_OK)
        return RESULT_FAIL;

      code.code[1] = in.get_size(0);
      code.code[2] = in.get_size(1);
      code.code[3] = in.get_stride();
      code.code[4] = in.get_flags().get_rawFlags();

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeArray(const Array<Type> &in, void * data, const s32 dataLength)
    {
      u32 * dataU32 = reinterpret_cast<u32*>(data);

      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeArray", "in Array is not Valid");

      AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
        RESULT_FAIL, "SerializedBuffer::SerializeArray", "data is not 4-byte aligned");

      const u32 height = in.get_size(0);
      const u32 width = in.get_size(1);
      const u32 stride = in.get_stride();
      const u32 flags = in.get_flags().get_rawFlags();

      const s32 numRequiredBytes = height*stride + EncodedArray::CODE_SIZE*sizeof(u32);
      AnkiConditionalErrorAndReturnValue(dataLength >= numRequiredBytes,
        RESULT_FAIL, "SerializedBuffer::SerializeArray", "data needs at least %d bytes", numRequiredBytes);

      EncodedArray code;
      SerializedBuffer::EncodeArrayType(in, code);

      for(s32 i=0; i<EncodedArray::CODE_SIZE; i++) {
        dataU32[i] = code.code[i];
      }
      dataU32 += EncodedArray::CODE_SIZE;

      const u32 * inData = reinterpret_cast<const u32*>(in.Pointer(0,0));

      const s32 numArrayBytes4 = (height*stride) >> 2;
      for(s32 i=0; i<numArrayBytes4; i++) {
        dataU32[i] = inData[i];
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::DeserializeArray(const bool swapEndianForHeaders, const bool swapEndianForContents, const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory)
    {
      const u32 * dataU32 = reinterpret_cast<const u32*>(data);

      EncodedArray code;

      for(s32 i=0; i<EncodedArray::CODE_SIZE; i++) {
        code.code[i] = dataU32[i];
      }
      dataU32 += EncodedArray::CODE_SIZE;

      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      u8 basicType_size;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      if(SerializedBuffer::DecodeArrayType(swapEndianForHeaders, code, height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
        return RESULT_FAIL;

      const s32 bytesLeft = dataLength - (EncodedArray::CODE_SIZE * sizeof(u32));

      AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
        RESULT_FAIL, "SerializedBuffer::DecodeArray", "Parsed stride is not reasonable");

      AnkiConditionalErrorAndReturnValue(bytesLeft == (height*stride),
        RESULT_FAIL, "SerializedBuffer::DecodeArray", "Not enought bytes left to set the array");

      out = Array<Type> (height, width, memory);

      if(swapEndianForContents) {
        u8 * pOut = out.Pointer(0,0);
        const u8 * pData = reinterpret_cast<const u8*>(dataU32); // Starts after the data header
        for(s32 i=0; i<bytesLeft; i++) {
          pOut[i^3] = pData[i];
        }
      } else {
        memcpy(out.Pointer(0,0), dataU32, bytesLeft);
      }

      return RESULT_OK;
    }

    template<typename Type> Type* SerializedBuffer::PushBack(const Type *data, const s32 dataLength)
    {
      EncodedBasicTypeBuffer code;

      if(SerializedBuffer::EncodeBasicTypeBuffer<Type>(dataLength/sizeof(Type), code) != RESULT_OK)
        return NULL;

      void * segment = PushBack(SerializedBuffer::DATA_TYPE_BASIC_TYPE_BUFFER,
        &code.code[0], EncodedBasicTypeBuffer::CODE_SIZE*sizeof(u32),
        reinterpret_cast<const void*>(data), dataLength);

      return reinterpret_cast<Type*>(segment);
    }

    template<typename Type> void* SerializedBuffer::PushBack(const Array<Type> &in)
    {
      EncodedArray code;

      if(EncodeArrayType<Type>(in, code) != RESULT_OK)
        return NULL;

      void * segment = PushBack(SerializedBuffer::DATA_TYPE_ARRAY,
        &code.code[0], EncodedArray::CODE_SIZE*sizeof(u32),
        reinterpret_cast<const void*>(in.Pointer(0,0)), in.get_stride()*in.get_size(0));

      return segment;
    }

#pragma mark --- Specializations ---
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
