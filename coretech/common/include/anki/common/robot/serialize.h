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
    // #pragma mark

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

    template<typename Type> Result SerializedBuffer::EncodeArraySliceType(const ArraySlice<Type> &in, EncodedArraySlice &code)
    {
      // The first part of the code is the same as an Array
      EncodedArray arrayCode;
      const Result result = SerializedBuffer::EncodeArrayType(in.get_array(), arrayCode);

      if(result != RESULT_OK)
        return result;

      for(s32 i=0; i<5; i++) {
        code.code[i] = arrayCode.code[i];
      }

      // The second part is the properties of the slice

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 yEnd = ySlice.get_end();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xEnd = xSlice.get_end();

      code.code[5] = *reinterpret_cast<const u32*>(&yStart);
      code.code[6] = *reinterpret_cast<const u32*>(&yIncrement);
      code.code[7] = *reinterpret_cast<const u32*>(&yEnd);

      code.code[8] = *reinterpret_cast<const u32*>(&xStart);
      code.code[9] = *reinterpret_cast<const u32*>(&xIncrement);
      code.code[10]= *reinterpret_cast<const u32*>(&xEnd);

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeArray(const Array<Type> &in, void * data, const s32 dataLength)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeArray", "in Array is not Valid");

      // TODO: are the alignment restrictions still required for the M4?
      //AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
      //  RESULT_FAIL, "SerializedBuffer::SerializeArray", "data is not 4-byte aligned");

      u32 * dataU32 = reinterpret_cast<u32*>(data);

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

    template<typename Type> Result SerializedBuffer::DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory)
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
      if(SerializedBuffer::DecodeArrayType(code, height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
        return RESULT_FAIL;

      const s32 bytesLeft = dataLength - (EncodedArray::CODE_SIZE * sizeof(u32));

      AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
        RESULT_FAIL, "SerializedBuffer::DecodeArray", "Parsed stride is not reasonable");

      AnkiConditionalErrorAndReturnValue(bytesLeft == (height*stride),
        RESULT_FAIL, "SerializedBuffer::DecodeArray", "Not enought bytes left to set the array");

      out = Array<Type> (height, width, memory);

      memcpy(out.Pointer(0,0), dataU32, bytesLeft);

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeArraySlice(const ArraySlice<Type> &in, void * data, const s32 dataLength)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeArraySlice", "in ArraySlice is not Valid");

      // TODO: are the alignment restrictions still required for the M4?
      //AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
      //  RESULT_FAIL, "SerializedBuffer::SerializeArraySlice", "data is not 4-byte aligned");

      u32 * dataU32 = reinterpret_cast<u32*>(data);

      // NOTE: these parameters are the size that will be transmitted, not the original size
      const u32 height = in.get_ySlice().get_size();
      const u32 width = in.get_xSlice().get_size();
      const u32 stride = width*sizeof(Type);
      const u32 flags = in.get_flags().get_rawFlags();

      const s32 numRequiredBytes = height*stride + EncodedArraySlice::CODE_SIZE*sizeof(u32);
      AnkiConditionalErrorAndReturnValue(dataLength >= numRequiredBytes,
        RESULT_FAIL, "SerializedBuffer::SerializeArraySlice", "data needs at least %d bytes", numRequiredBytes);

      EncodedArraySlice code;
      SerializedBuffer::EncodeArraySliceType(in, code);

      for(s32 i=0; i<EncodedArraySlice::CODE_SIZE; i++) {
        dataU32[i] = code.code[i];
      }
      dataU32 += EncodedArraySlice::CODE_SIZE;

      // TODO: this could be done more efficiently
      Type * restrict pDataType = reinterpret_cast<Type*>(dataU32);
      s32 iData = 0;

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 yEnd = ySlice.get_end();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xEnd = xSlice.get_end();

      for(s32 y=yStart; y<=yEnd; y+=yIncrement) {
        Type * restrict pInData = in.Pointer(y, 0);
        for(s32 x=xStart; x<=xEnd; x+=xIncrement) {
          pDataType[iData] = pInData[x];
          iData++;
        }
      }

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::DeserializeArraySlice(const void * data, const s32 dataLength, ArraySlice<Type> &out, MemoryStack &memory)
    {
      const u32 * dataU32 = reinterpret_cast<const u32*>(data);

      EncodedArraySlice code;

      for(s32 i=0; i<EncodedArraySlice::CODE_SIZE; i++) {
        code.code[i] = dataU32[i];
      }
      dataU32 += EncodedArraySlice::CODE_SIZE;

      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      s32 ySlice_start;
      s32 ySlice_increment;
      s32 ySlice_end;
      s32 xSlice_start;
      s32 xSlice_increment;
      s32 xSlice_end;
      u8 basicType_size;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      if(SerializedBuffer::DecodeArraySliceType(code, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_end, xSlice_start, xSlice_increment, xSlice_end, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
        return RESULT_FAIL;

      const s32 bytesLeft = dataLength - (EncodedArraySlice::CODE_SIZE * sizeof(u32));

      AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
        RESULT_FAIL, "SerializedBuffer::DeserializeArraySlice", "Parsed stride is not reasonable");

      AnkiConditionalErrorAndReturnValue(bytesLeft == (height*stride),
        RESULT_FAIL, "SerializedBuffer::DeserializeArraySlice", "Not enought bytes left to set the array");

      Array<Type> array(height, width, memory);

      // TODO: this could be done more efficiently

      Type * restrict pDataType = reinterpret_cast<Type*>(dataU32);
      s32 iData = 0;

      for(s32 y=ySlice_start; y<=ySlice_end; y+=ySlice_increment) {
        Type * restrict pOutData = out.Pointer(y, 0);
        for(s32 x=xSlice_start; x<=xSlice_end; x+=xSlice_increment) {
          pOutData[x] = pDataType[iData];
          iData++;
        }
      }

      const LinearSequence<s32> ySlice(ySlice_start, ySlice_increment, ySlice_end);
      const LinearSequence<s32> xSlice(xSlice_start, xSlice_increment, xSlice_end);

      out = ArraySlice<Type>(array, ySlice, xSlice);

      return RESULT_OK;
    }

    // Helper functions to serialize or deserialize an FixedLengthList (which is just a type of ArraySlice)
    template<typename Type> Result SerializedBuffer::SerializeFixedLengthList(const FixedLengthList<Type> &in, void * data, const s32 dataLength)
    {
      return SerializeArraySlice(in, data, dataLength);
    }

    template<typename Type> Result SerializedBuffer::DeserializeFixedLengthList(const void * data, const s32 dataLength, FixedLengthList<Type> &out, MemoryStack &memory)
    {
      ArraySlice<Type> arraySlice;

      const Result result = SerializedBuffer::DeserializeArraySlice(data, dataLength, arraySlice, memory);

      if(result != RESULT_OK)
        return result;

      out = FixedLengthList<Type>();

      out.ySlice = arraySlice.get_ySlice();
      out.xSlice = arraySlice.get_xSlice();
      out.array = arraySlice.get_array();
      out.arrayData = out.array.Pointer(0,0);;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRaw(const Type &in, void ** buffer, s32 &bufferLength)
    {
      memcpy(*buffer, &in, sizeof(Type));

      *buffer = reinterpret_cast<u8*>(*buffer) + sizeof(Type);
      bufferLength -= sizeof(Type);

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRaw(const Array<Type> &in, void ** buffer, s32 &bufferLength)
    {
      EncodedArray code;

      if(EncodeArrayType<Type>(in, code) != RESULT_OK)
        return RESULT_FAIL;

      memcpy(*buffer, &code.code[0], EncodedArray::CODE_SIZE*sizeof(u32));

      *buffer = reinterpret_cast<u8*>(*buffer) + EncodedArray::CODE_SIZE*sizeof(u32);
      bufferLength -= EncodedArray::CODE_SIZE*sizeof(u32);

      memcpy(*buffer, in.Pointer(0,0), in.get_stride()*in.get_size(0));

      *buffer = reinterpret_cast<u8*>(*buffer) + in.get_stride()*in.get_size(0);
      bufferLength -= in.get_stride()*in.get_size(0);

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRaw(const ArraySlice<Type> &in, void ** buffer, s32 &bufferLength)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL, "SerializedBuffer::SerializeRaw", "in ArraySlice is not Valid");

      // TODO: are the alignment restrictions still required for the M4?
      //AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(in)%4 == 0,
      //  RESULT_FAIL, "SerializedBuffer::SerializeArraySlice", "in is not 4-byte aligned");

      // NOTE: these parameters are the size that will be transmitted, not the original size
      const u32 height = in.get_ySlice().get_size();
      const u32 width = in.get_xSlice().get_size();
      const u32 stride = width*sizeof(Type);
      const u32 flags = in.get_flags().get_rawFlags();

      const s32 numRequiredBytes = height*stride + EncodedArraySlice::CODE_SIZE*sizeof(u32);

      AnkiConditionalErrorAndReturnValue(inLength >= numRequiredBytes,
        RESULT_FAIL, "SerializedBuffer::SerializeRaw", "buffer needs at least %d bytes", numRequiredBytes);

      EncodedArraySlice code;
      SerializedBuffer::EncodeArraySliceType(in, code);

      memcpy(*buffer, &code.code[0], EncodedArraySlice::CODE_SIZE*sizeof(u32));

      *buffer = reinterpret_cast<u8*>(*buffer) + EncodedArraySlice::CODE_SIZE*sizeof(u32);
      bufferLength -= EncodedArraySlice::CODE_SIZE*sizeof(u32);

      // TODO: this could be done more efficiently
      Type * restrict pDataType = reinterpret_cast<Type*>(*buffer);
      s32 iData = 0;

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 yEnd = ySlice.get_end();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xEnd = xSlice.get_end();

      for(s32 y=yStart; y<=yEnd; y+=yIncrement) {
        Type * restrict pInData = in.Pointer(y, 0);
        for(s32 x=xStart; x<=xEnd; x+=xIncrement) {
          pDataType[iData] = pInData[x];
          iData++;
        }
      }

      *buffer = reinterpret_cast<u8*>(*buffer) + stride*height;
      bufferLength -= stride*height;

      return RESULT_OK;
    }

    template<typename Type> Result SerializedBuffer::SerializeRaw(const FixedLengthList<Type> &in, void ** buffer, s32 &bufferLength)
    {
      return SerializeRaw(*static_cast<ArraySlice<Type>*>(&in), data, dataLength);
    }

    template<typename Type> Type SerializedBuffer::DeserializeRaw(void ** buffer, s32 &bufferLength)
    {
      const Type var = *reinterpret_cast<Type*>(*buffer);

      *buffer = reinterpret_cast<u8*>(*buffer) + sizeof(Type);
      bufferLength -= sizeof(Type);

      return var;
    }

    template<typename Type> Array<Type> SerializedBuffer::DeserializeRawArray(void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      u8 basicType_size;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;

      {
        const u32 * bufferU32 = reinterpret_cast<const u32*>(*buffer);

        EncodedArray code;

        for(s32 i=0; i<EncodedArray::CODE_SIZE; i++) {
          code.code[i] = bufferU32[i];
        }

        if(SerializedBuffer::DecodeArrayType(code, height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
          return Array<Type>();
      }

      *buffer = reinterpret_cast<u8*>(*buffer) + (EncodedArray::CODE_SIZE * sizeof(u32));
      bufferLength -= (EncodedArray::CODE_SIZE * sizeof(u32));

      AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
        Array<Type>(), "SerializedBuffer::DeserializeRaw", "Parsed stride is not reasonable");

      AnkiConditionalErrorAndReturnValue(bufferLength >= (height*stride),
        Array<Type>(), "SerializedBuffer::DeserializeRaw", "Not enought bytes left to set the array");

      Array<Type> out = Array<Type> (height, width, memory);

      memcpy(out.Pointer(0,0), *buffer, height*stride);

      *buffer = reinterpret_cast<u8*>(*buffer) + height*stride;
      bufferLength -= height*stride;

      return out;
    }

    template<typename Type> ArraySlice<Type> SerializedBuffer::DeserializeRawArraySlice(void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      s32 ySlice_start;
      s32 ySlice_increment;
      s32 ySlice_end;
      s32 xSlice_start;
      s32 xSlice_increment;
      s32 xSlice_end;
      u8 basicType_size;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;

      {
        const u32 * bufferU32 = reinterpret_cast<const u32*>(buffer);

        EncodedArraySlice code;

        for(s32 i=0; i<EncodedArraySlice::CODE_SIZE; i++) {
          code.code[i] = bufferU32[i];
        }

        if(SerializedBuffer::DecodeArraySliceType(code, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_end, xSlice_start, xSlice_increment, xSlice_end, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
          return ArraySlice<Type>();
      }

      *buffer = reinterpret_cast<u8*>(*buffer) + (EncodedArraySlice::CODE_SIZE * sizeof(u32));
      bufferLength -= (EncodedArraySlice::CODE_SIZE * sizeof(u32));

      AnkiConditionalErrorAndReturnValue(stride == RoundUp(width*sizeof(Type), MEMORY_ALIGNMENT),
        RESULT_FAIL, "SerializedBuffer::DeserializeRawArraySlice", "Parsed stride is not reasonable");

      AnkiConditionalErrorAndReturnValue(bufferLength >= (height*stride),
        RESULT_FAIL, "SerializedBuffer::DeserializeRawArraySlice", "Not enought bytes left to set the array");

      Array<Type> array(height, width, memory);

      // TODO: this could be done more efficiently

      Type * restrict pDataType = reinterpret_cast<Type*>(*buffer);
      s32 iData = 0;

      for(s32 y=ySlice_start; y<=ySlice_end; y+=ySlice_increment) {
        Type * restrict pOutData = out.Pointer(y, 0);
        for(s32 x=xSlice_start; x<=xSlice_end; x+=xSlice_increment) {
          pOutData[x] = pDataType[iData];
          iData++;
        }
      }

      const LinearSequence<s32> ySlice(ySlice_start, ySlice_increment, ySlice_end);
      const LinearSequence<s32> xSlice(xSlice_start, xSlice_increment, xSlice_end);

      out = ArraySlice<Type>(array, ySlice, xSlice);

      *buffer = reinterpret_cast<u8*>(*buffer) + height*stride;
      bufferLength -= height*stride;

      return RESULT_OK;
    }

    template<typename Type> FixedLengthList<Type> SerializedBuffer::DeserializeRawFixedLengthList(void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      ArraySlice<Type> arraySlice;

      const Result result = SerializedBuffer::DeserializeRawArraySlice(buffer, bufferLength, arraySlice, memory);

      if(result != RESULT_OK)
        return result;

      out = FixedLengthList<Type>();

      out.ySlice = arraySlice.get_ySlice();
      out.xSlice = arraySlice.get_xSlice();
      out.array = arraySlice.get_array();
      out.arrayData = out.array.Pointer(0,0);;

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

      void * segment = PushBack(
        SerializedBuffer::DATA_TYPE_ARRAY,
        &code.code[0], EncodedArray::CODE_SIZE*sizeof(u32),
        reinterpret_cast<const void*>(in.Pointer(0,0)), in.get_stride()*in.get_size(0));

      return segment;
    }

    template<typename Type> void* SerializedBuffer::PushBack(const ArraySlice<Type> &in)
    {
      EncodedArraySlice code;

      if(EncodeArraySliceType<Type>(in, code) != RESULT_OK)
        return NULL;

      const LinearSequence<s32>& ySlice = in.get_ySlice();
      const LinearSequence<s32>& xSlice = in.get_xSlice();

      // NOTE: these parameters are the size that will be transmitted, not the original size
      const u32 height = ySlice.get_size();
      const u32 width = xSlice.get_size();
      const u32 stride = width*sizeof(Type);
      const u32 flags = in.get_flags().get_rawFlags();

      const s32 numRequiredDataBytes = height*stride;

      // WARNING: This copying breaks the CRC code generation
      void * segment = PushBack(
        SerializedBuffer::DATA_TYPE_ARRAYSLICE,
        &code.code[0], EncodedArraySlice::CODE_SIZE*sizeof(u32),
        NULL, numRequiredDataBytes);

      // TODO: this could be done more efficiently
      Type * restrict shiftedSegmentType = reinterpret_cast<Type*>( reinterpret_cast<u8*>(segment) + EncodedArraySlice::CODE_SIZE*sizeof(u32) );
      s32 iData = 0;

      const s32 yStart = ySlice.get_start();
      const s32 yIncrement = ySlice.get_increment();
      const s32 yEnd = ySlice.get_end();

      const s32 xStart = xSlice.get_start();
      const s32 xIncrement = xSlice.get_increment();
      const s32 xEnd = xSlice.get_end();

      for(s32 y=yStart; y<=yEnd; y+=yIncrement) {
        Type * restrict pInData = in.Pointer(y, 0);
        for(s32 x=xStart; x<=xEnd; x+=xIncrement) {
          shiftedSegmentType[iData] = pInData[x];
          iData++;
        }
      }

      return segment;
    }

    /*    template<typename Type> void* SerializedBuffer::PushBack(const FixedLengthList<Type> &in)
    {
    EncodedArray code;

    if(EncodeArrayType<Type>(in.get_array(), code) != RESULT_OK)
    return NULL;

    void * segment = PushBack(SerializedBuffer::DATA_TYPE_LIST,
    &code.code[0], EncodedArray::CODE_SIZE*sizeof(u32),
    reinterpret_cast<const void*>(in.Pointer(0)), in.get_size());

    return segment;
    }*/

    // #pragma mark --- Specializations ---
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
