/**
File: array2d.h
Author: Peter Barnum
Created: 2013

Definitions of array2d_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_

#include <string.h>
#include <stdio.h>

#include "coretech/common/robot/array2d_declarations.h"

#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/geometry.h"
#include "coretech/common/robot/utilities_c.h"
#include "coretech/common/robot/sequences.h"
#include "coretech/common/robot/matrix.h"
#include "coretech/common/robot/comparisons.h"

#include "coretech/common/shared/utilities_shared.h"

#include "coretech/common/robot/serialize_declarations.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect.hpp"
#endif

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#define ANKICORETECH_EMBEDDED_USE_MALLOC 1
#define ANKICORETECH_EMBEDDED_USE_ZLIB 1
#endif

#if ANKICORETECH_EMBEDDED_USE_ZLIB
#include "zlib.h"
#endif

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    // #pragma mark --- Array Definitions ---

    template<typename Type> s32 Array<Type>::ComputeRequiredStride(const s32 numCols, const Flags::Buffer flags)
    {
      AnkiConditionalErrorAndReturnValue(numCols >= 0,
        0, "Array<Type>::ComputeRequiredStride", "Invalid size");

      const s32 numColsCapped = MAX(numCols, 1);

      const s32 bufferRequired = static_cast<s32>(RoundUp<size_t>(sizeof(Type)*numColsCapped, MEMORY_ALIGNMENT));

      return bufferRequired;
    }

    template<typename Type> s32 Array<Type>::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const Flags::Buffer flags)
    {
      AnkiConditionalErrorAndReturnValue(numCols >= 0 && numRows >= 0,
        0, "Array<Type>::ComputeMinimumRequiredMemory", "Invalid size");

      const s32 numRowsCapped = MAX(numRows, 1);

      return numRowsCapped * Array<Type>::ComputeRequiredStride(numCols, flags);
    }

    template<typename Type> Array<Type>::Array()
    {
      InvalidateArray();
    }

    template<typename Type> Array<Type>::Array(const s32 numRows, const s32 numCols, void * data, const s32 dataLength, const Flags::Buffer flags)
    {
      InvalidateArray();

      AnkiConditionalErrorAndReturn(reinterpret_cast<size_t>(data)%MEMORY_ALIGNMENT == 0,
        "Array::Array", "If fully allocated, data must be %d byte aligned", MEMORY_ALIGNMENT);

      this->stride = ComputeRequiredStride(numCols, flags);

      AnkiConditionalErrorAndReturn(numCols >= 0 && numRows >= 0 && dataLength >= numRows*this->stride,
        "Array<Type>::Array", "Invalid size");

      if(flags.get_isFullyAllocated()) {
        if(numRows == 1) {
          // If there's only one row, the stride restrictions are less stringent, though the buffer still must round up to a multiple of 16 bytes (or more)
          AnkiConditionalErrorAndReturn(this->stride <= dataLength,
            "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, the dataLength must be greater-than-or-equal-to the stride");
        } else {
          const s32 simpleStride = numCols * static_cast<s32>(sizeof(Type));

          AnkiConditionalErrorAndReturn(this->stride == simpleStride,
            "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, the stride must be simple");

          AnkiConditionalErrorAndReturn((numCols*sizeof(Type)) % MEMORY_ALIGNMENT == 0,
            "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, (numCols*sizeof(Type)) mod MEMORY_ALIGNMENT must equal zero");
        }

        AnkiConditionalErrorAndReturn(flags.get_useBoundaryFillPatterns() == false,
          "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, flags.get_useBoundaryFillPatterns must be false");
      }

      InitializeBuffer(numRows,
        numCols,
        data,
        dataLength,
        flags);
    }

    template<typename Type> Array<Type>::Array(const s32 numRows, const s32 numCols, MemoryStack &memory, const Flags::Buffer flags)
    {
      InvalidateArray();

      AnkiConditionalErrorAndReturn(numCols >= 0 && numRows >= 0,
        "Array<Type>::Array", "Invalid size");

      s32 numBytesAllocated = 0;

      void * allocatedBuffer = AllocateBufferFromMemoryStack(numRows, ComputeRequiredStride(numCols, flags), memory, numBytesAllocated, flags, false);

      InitializeBuffer(numRows,
        numCols,
        reinterpret_cast<Type*>(allocatedBuffer),
        numBytesAllocated,
        flags);
    }

    template<typename Type> Array<Type> Array<Type>::LoadImage(const char * filename, MemoryStack &memory)
    {
      Array<Type> newArray = Array<Type>();

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      const cv::Mat cvImage = cv::imread(filename, CV_LOAD_IMAGE_GRAYSCALE);

      AnkiConditionalErrorAndReturnValue(cvImage.cols > 0 && cvImage.rows > 0,
        newArray, "Array<Type>::LoadImage", "Could not load image");

      newArray = Array<Type>(cvImage.rows, cvImage.cols, memory);

      AnkiConditionalErrorAndReturnValue(newArray.IsValid(),
        newArray, "Array<Type>::LoadImage", "Invalid size");

      const u8 * restrict pCvImage = cvImage.data;

      for(s32 y=0; y<cvImage.rows; y++) {
        Type * restrict pNewArray = newArray.Pointer(y, 0);

        for(s32 x=0; x<cvImage.cols; x++) {
          pNewArray[x] = static_cast<Type>(pCvImage[x]);
        }

        pCvImage += cvImage.step.buf[0];
      }
#else
      AnkiError("Array<Type>::Array", "OpenCV is required to load an image from an image file");
#endif

      return newArray;
    } // Array<Type>::LoadImage(const char * filename, MemoryStack &memory)

    template<typename Type> Array<Type> LoadBinaryArray_Generic(const char * filename, MemoryStack *scratch, MemoryStack *memory, void * allocatedBuffer, const s32 allocatedBufferLength)
    {
      u16  basicType_sizeOfType;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      bool basicType_isString;

      Array<u8> rawArray = LoadBinaryArray_UnknownType(
        filename,
        scratch, memory,
        allocatedBuffer, allocatedBufferLength,
        basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString);

      // TODO: check that the types match

      Array<Type> newArray = *reinterpret_cast<Array<Type>*>( &rawArray );

      return newArray;
    } // / LoadBinaryArray_Generic()

    template<typename Type> Array<Type> Array<Type>::LoadBinary(const char * filename, MemoryStack scratch, MemoryStack &memory)
    {
      return LoadBinaryArray_Generic<Type>(filename, &scratch, &memory, NULL, -1);
    } // Array<Type>::LoadBinary(const char * filename, MemoryStack scratch, MemoryStack &memory)

    template<typename Type> Array<Type> Array<Type>::LoadBinary(const char * filename, void * allocatedBuffer, const s32 allocatedBufferLength) //< allocatedBuffer must be freed manually
    {
      return LoadBinaryArray_Generic<Type>(filename, NULL, NULL, allocatedBuffer, allocatedBufferLength);
    } // LoadBinaryMalloc()

    template<typename Type> Result Array<Type>::SaveBinary(const char * filename, const s32 compressionLevel, MemoryStack scratch) const
    {
      AnkiConditionalErrorAndReturnValue(AreValid(*this, scratch) && filename,
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::SaveBinary", "Invalid inputs");

      AnkiConditionalErrorAndReturnValue(compressionLevel >= 0 && compressionLevel <= 9,
        RESULT_FAIL_INVALID_PARAMETER, "Array<Type>::SaveBinary", "Invalid compression level");

      // If this is a string array, add the sizes of the null terminated strings (or zero otherwise)
      const s32 stringsLength = TotalArrayStringLengths<Type>(*this);

      const s32 serializedBufferLength = 4096 + ARRAY_FILE_HEADER_LENGTH + this->get_size(0) * this->get_stride() + stringsLength;
      void *buffer = scratch.Allocate(serializedBufferLength);

      AnkiConditionalErrorAndReturnValue(buffer,
        RESULT_FAIL_OUT_OF_MEMORY, "Array<Type>::SaveBinary", "Memory could not be allocated");

      SerializedBuffer toSave(buffer, serializedBufferLength);

      toSave.PushBack<Type>("Array", *this);

      s32 startIndex;
      u8 * bufferStart = reinterpret_cast<u8*>(toSave.get_memoryStack().get_validBufferStart(startIndex));
      const s32 validUsedBytes = toSave.get_memoryStack().get_usedBytes() - startIndex;

      // const s32 startDiff = static_cast<s32>( reinterpret_cast<size_t>(bufferStart) - reinterpret_cast<size_t>(toSave.get_memoryStack().get_buffer()) );
      // const s32 endDiff = toSave.get_memoryStack().get_totalBytes() - toSave.get_memoryStack().get_usedBytes();

      FILE *fp = fopen(filename, "wb");

      AnkiConditionalErrorAndReturnValue(fp,
        RESULT_FAIL_IO, "Array<Type>::SaveBinary", "Could not open file %s", filename);

      if(compressionLevel > 0) {
#if ANKICORETECH_EMBEDDED_USE_ZLIB
        char tmpTextHeader[ARRAY_FILE_HEADER_LENGTH+1];
        strncpy(tmpTextHeader, &ARRAY_FILE_HEADER[0], ARRAY_FILE_HEADER_LENGTH+1);
        snprintf(tmpTextHeader+ARRAY_FILE_HEADER_VALID_LENGTH+1, ARRAY_FILE_HEADER_LENGTH-ARRAY_FILE_HEADER_VALID_LENGTH, "z%s ", ZLIB_VERSION);

        const s32 originalLength = validUsedBytes + SERIALIZED_BUFFER_HEADER_LENGTH + SERIALIZED_BUFFER_FOOTER_LENGTH;

        uLongf compressedLength = 128 + saturate_cast<s32>(1.1 * originalLength);

        void * uncompressed = malloc(originalLength);
        void * compressed = malloc(compressedLength + 2*sizeof(s32));

        if(!uncompressed || !compressed) {
          if(uncompressed)
            free(uncompressed);

          if(compressed)
            free(compressed);

          AnkiError("Array<Type>::SaveBinary", "Out of memory");

          return RESULT_FAIL_OUT_OF_MEMORY;
        }

        // Copy the uncompressed data into one buffer
        {
          char * pUncompressed = reinterpret_cast<char*>(uncompressed);

          memcpy(pUncompressed, &SERIALIZED_BUFFER_HEADER[0], SERIALIZED_BUFFER_HEADER_LENGTH);
          pUncompressed += SERIALIZED_BUFFER_HEADER_LENGTH;

          memcpy(pUncompressed, bufferStart, validUsedBytes);
          pUncompressed += validUsedBytes;

          memcpy(pUncompressed, &SERIALIZED_BUFFER_FOOTER[0], SERIALIZED_BUFFER_FOOTER_LENGTH);
        }

        const s32 compressionResult = compress2(reinterpret_cast<Bytef*>(compressed) + 2*sizeof(s32), &compressedLength, reinterpret_cast<Bytef*>(uncompressed), originalLength, compressionLevel);

        if(compressionResult != Z_OK) {
          if(uncompressed)
            free(uncompressed);

          if(compressed)
            free(compressed);

          AnkiError("Array<Type>::SaveBinary", "Zlib error");
          return RESULT_FAIL_IO;
        }

        reinterpret_cast<s32*>(compressed)[0] = static_cast<s32>(originalLength);
        reinterpret_cast<s32*>(compressed)[1] = static_cast<s32>(compressedLength);

        const size_t bytesWrittenForTextHeader = fwrite(tmpTextHeader, 1, ARRAY_FILE_HEADER_LENGTH, fp);

        const size_t bytesWritten = fwrite(compressed, 1, compressedLength + 2*sizeof(s32), fp);

        if(uncompressed)
          free(uncompressed);

        if(compressed)
          free(compressed);

        AnkiConditionalErrorAndReturnValue(
          bytesWrittenForTextHeader == ARRAY_FILE_HEADER_LENGTH &&
          bytesWritten == (compressedLength + 2*sizeof(s32)),
          RESULT_FAIL_IO, "Array<Type>::SaveBinary", "Save failed");

#else
        AnkiError("Array<Type>::SaveBinary", "Saving with compression requires zlib");
        return RESULT_FAIL;
#endif
      } else {
        const size_t bytesWrittenForTextHeader = fwrite(&ARRAY_FILE_HEADER[0], 1, ARRAY_FILE_HEADER_LENGTH, fp);

        const size_t bytesWrittenForHeader = fwrite(&SERIALIZED_BUFFER_HEADER[0], 1, SERIALIZED_BUFFER_HEADER_LENGTH, fp);

        const size_t bytesWritten = fwrite(bufferStart, 1, validUsedBytes, fp);

        const size_t bytesWrittenForFooter = fwrite(&SERIALIZED_BUFFER_FOOTER[0], 1, SERIALIZED_BUFFER_FOOTER_LENGTH, fp);

        AnkiConditionalErrorAndReturnValue(
          bytesWrittenForTextHeader == ARRAY_FILE_HEADER_LENGTH &&
          bytesWrittenForHeader == SERIALIZED_BUFFER_HEADER_LENGTH &&
          bytesWritten == validUsedBytes &&
          bytesWrittenForFooter == SERIALIZED_BUFFER_FOOTER_LENGTH,
          RESULT_FAIL_IO, "Array<Type>::SaveBinary", "Save failed");
      }

      fclose(fp);

      return RESULT_OK;
    } // Array<Type>::SaveBinary(const char * filename, MemoryStack scratch)

    template<typename Type> const Type* Array<Type>::Pointer(const s32 index0, const s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1]);
      assert(this->IsValid());

      return reinterpret_cast<const Type*>( reinterpret_cast<const char*>(this->data) + index0*stride ) + index1;
    }

    template<typename Type> Type* Array<Type>::Pointer(const s32 index0, const s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1]);
      assert(this->IsValid());

      return reinterpret_cast<Type*>( reinterpret_cast<char*>(this->data) + index0*stride ) + index1;
    }

    template<typename Type> inline const Type * Array<Type>::operator[](const s32 index0) const
    {
      assert(index0 >= 0 && index0 < this->size[0]);

      return reinterpret_cast<const Type*>( reinterpret_cast<const char*>(this->data) + index0*stride );
    }

    template<typename Type> inline Type * Array<Type>::operator[](const s32 index0)
    {
      assert(index0 >= 0 && index0 < this->size[0]);

      return reinterpret_cast<Type*>( reinterpret_cast<char*>(this->data) + index0*stride );
    }

    template<typename Type> const Type* Array<Type>::Pointer(const Point<s16> &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    template<typename Type> Type* Array<Type>::Pointer(const Point<s16> &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    template<typename Type> const Type& Array<Type>::Element(const s32 elementIndex) const
    {
      const s32 index1 = elementIndex % size[1];
      const s32 index0 = (elementIndex - index1) / size[1];

      return *Pointer(index0, index1);
    }

    template<typename Type> Type& Array<Type>::Element(const s32 elementIndex)
    {
      const s32 index1 = elementIndex % size[1];
      const s32 index0 = (elementIndex - index1) / size[1];

      return *Pointer(index0, index1);
    }

    template<typename Type> ArraySlice<Type> Array<Type>::operator() ()
    {
      ArraySlice<Type> slice(*this);

      return slice;
    }

    template<typename Type> ArraySlice<Type> Array<Type>::operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
    {
      ArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ArraySlice<Type> Array<Type>::operator() (s32 minY, s32 maxY, s32 minX, s32 maxX)
    {
      LinearSequence<s32> ySlice = IndexSequence<s32>(minY, 1, maxY, this->size[0]);
      LinearSequence<s32> xSlice = IndexSequence<s32>(minX, 1, maxX, this->size[1]);

      ArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ArraySlice<Type> Array<Type>::operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX)
    {
      LinearSequence<s32> ySlice = IndexSequence(minY, incrementY, maxY, this->size[0]);
      LinearSequence<s32> xSlice = IndexSequence(minX, incrementX, maxX, this->size[1]);

      ArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ConstArraySlice<Type> Array<Type>::operator() () const
    {
      ConstArraySlice<Type> slice(*this);

      return slice;
    }

    template<typename Type> ConstArraySlice<Type> Array<Type>::operator() (const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice) const
    {
      ConstArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ConstArraySlice<Type> Array<Type>::operator() (s32 minY, s32 maxY, s32 minX, s32 maxX) const
    {
      LinearSequence<s32> ySlice = IndexSequence(minY, 1, maxY, this->size[0]);
      LinearSequence<s32> xSlice = IndexSequence(minX, 1, maxX, this->size[1]);

      ConstArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ConstArraySlice<Type> Array<Type>::operator() (s32 minY, s32 incrementY, s32 maxY, s32 minX, s32 incrementX, s32 maxX) const
    {
      LinearSequence<s32> ySlice = IndexSequence(minY, incrementY, maxY, this->size[0]);
      LinearSequence<s32> xSlice = IndexSequence(minX, incrementX, maxX, this->size[1]);

      ConstArraySlice<Type> slice(*this, ySlice, xSlice);

      return slice;
    }

    template<typename Type> ConstArraySliceExpression<Type> Array<Type>::Transpose() const
    {
      ConstArraySliceExpression<Type> expression(this->operator() ());
      expression.Transpose();

      return expression;
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> s32 Array<Type>::Set(const cv::Mat_<Type> &in)
    {
      const s32 inHeight = in.rows;
      const s32 inWidth = in.cols;

      AnkiConditionalErrorAndReturnValue(inHeight != 0,
        0, "Array<Type>::Set", "input cv::Mat is invalid. If you use the release OpenCV libraries with the debug build, lots of things like file loading don't work.");

      const bool isColor = in.channels() == 3 || inWidth == this->size[1]*3;

      if(isColor) {
        AnkiConditionalErrorAndReturnValue(inHeight == this->size[0],
          0, "Array<Type>::Set", "input cv::Mat is the incorrect size.");
      } else {
        AnkiConditionalErrorAndReturnValue(inHeight == this->size[0] && inWidth == this->size[1],
          0, "Array<Type>::Set", "input cv::Mat is the incorrect size.");
      }

      for(s32 y=0; y<this->size[0]; y++) {
        const Type * restrict pIn = reinterpret_cast<const Type*>(in.ptr(y,0));
        Type * restrict pThis = this->Pointer(y,0);

        // If grayscale, just copy. If color, convert to grayscale
        if(isColor) {
          for(s32 x=0; x<this->size[1]; x++) {
            pThis[x] = (pIn[3*x] + pIn[3*x + 1] + pIn[3*x + 2]) / 3;
          }
        } else {
          memcpy(pThis, pIn, inWidth*sizeof(Type));
        }
      }

      return this->size[0]*this->size[1];
    }
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

    template<typename Type> void Array<Type>::Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues, const bool fitImageToWindow) const
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      AnkiConditionalError(this->IsValid(), "Array<Type>::Show", "Array<Type> is not valid");

      if(fitImageToWindow) {
        cv::namedWindow(windowName, CV_WINDOW_NORMAL);
      } else {
        cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
      }

      if(scaleValues) {
        cv::Mat_<f64> scaledArray;

        if(ArrayToCvMat(*this, &scaledArray) != RESULT_OK)
          return;

        const f64 minValue = Matrix::Min<Type>(*this);
        const f64 maxValue = Matrix::Max<Type>(*this);
        const f64 range = maxValue - minValue;

        scaledArray -= minValue;
        scaledArray /= range;

        cv::imshow(windowName, scaledArray);
      } else {
        cv::Mat_<Type> arrayCopy;

        if(ArrayToCvMat(*this, &arrayCopy) != RESULT_OK)
          return;

        cv::imshow(windowName, arrayCopy);
      }

      cv::waitKey(waitForKeypress ? 0 : 1);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
    }

    template<typename Type> Result Array<Type>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::Print", "Array<Type> is not valid");

      CoreTechPrint("%s", variableName);
      CoreTechPrint(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Type * const pThisData = this->Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          pThisData[x].Print();
        }
        CoreTechPrint("\n");
      }
      CoreTechPrint("\n");

      return RESULT_OK;
    }

    template<typename Type> Result Array<Type>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return this->Print(variableName, minY, maxY, minX, maxX);
    }

    template<typename Type> bool Array<Type>::IsNearlyEqualTo(const Array<Type>& other, const Type epsilon) const
    {
      bool isSame = false;
      if(AreValid(*this, other)) {
        const s32 nrows = this->get_size(0);
        const s32 ncols = this->get_size(1);

        if(other.get_size(0)==nrows && other.get_size(1) == ncols) {
          isSame = true;
          for(s32 i=0; i<nrows && isSame; ++i) {
            const Type * restrict pThis  = this->Pointer(i,0);
            const Type * restrict pOther = other.Pointer(i,0);

            for(s32 j=0; j<ncols; ++j) {
              if (ABS(pThis[j] - pOther[j]) >= epsilon) {
                isSame = false;
                break;
              }
            } // for j
          } // for i
        } // if sizes match
      } // if both valid

      return isSame;
    } // IsNearlyEqualTo()

    template<typename Type> bool Array<Type>::IsValid() const
    {
      if(this->data == NULL) {
        return false;
      }

      if(size[0] < 0 || size[1] < 0) {
        return false;
      }

      return true;
    }

    template<typename Type> Result Array<Type>::Resize(const s32 numRows, const s32 numCols, MemoryStack &memory)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0 && numRows > 0,
        RESULT_FAIL_INVALID_SIZE, "Array<Type>::Resize", "Invalid size");

      s32 numBytesAllocated = 0;

      this->data = reinterpret_cast<Type*>( AllocateBufferFromMemoryStack(numRows, ComputeRequiredStride(numCols, flags), memory, numBytesAllocated, flags, true) );

      // Don't clear the reallocated memory
      const bool clearMemory = this->flags.get_zeroAllocatedMemory();
      this->flags.set_zeroAllocatedMemory(false);

      const Result result = InitializeBuffer(numRows,
        numCols,
        this->data,
        numBytesAllocated,
        this->flags);

      this->flags.set_zeroAllocatedMemory(clearMemory);

      return result;
    }

    template<typename Type> s32 Array<Type>::SetZero()
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::SetZero", "Array<Type> is not valid");

      const s32 numBytes = this->get_size(0)*this->get_stride();

      memset(this->Pointer(0,0), 0, numBytes);

      return numBytes;
    }

    template<typename Type> s32 Array<Type>::Set(const Type value)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Set", "Array<Type> is not valid");

      for(s32 y=0; y<size[0]; y++) {
        Type * restrict pThisData = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          pThisData[x] = value;
        }
      }

      return size[0]*size[1];
    }

    template<typename Type> s32 Array<Type>::Set(const Array<Type> &in)
    {
      return this->SetCast<Type>(in);
    }

    template<typename Type> template<typename InType> s32 Array<Type>::SetCast(const Array<InType> &in)
    {
      const s32 inHeight = in.get_size(0);
      const s32 inWidth = in.get_size(1);

      AnkiConditionalErrorAndReturnValue(AreValid(*this, in),
        0, "Array<Type>::Set", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(inHeight == this->size[0] && inWidth == this->size[1],
        0, "Array<Type>::Set", "Array sizes don't match");

      for(s32 y=0; y<size[0]; y++) {
        const InType * restrict pIn = in.Pointer(y, 0);
        Type * restrict pThisData = Pointer(y, 0);

        for(s32 x=0; x<size[1]; x++) {
          pThisData[x] = saturate_cast<Type>(pIn[x]);
        }
      }

      return size[0]*size[1];
    }

    template<typename InType> s32 SetCast(const InType * const values, const s32 numValues)
    {
      // This is a little tough to write a general case for, so this method should be specialized
      // for each relevant case
      assert(false);

      return 0;
    }

    template<typename Type> s32 Array<Type>::Set(const Type * const values, const s32 numValues)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Set", "Array<Type> is not valid");

      s32 numValuesSet = 0;

      for(s32 y=0; y<size[0]; y++) {
        u32 * restrict pThisData = reinterpret_cast<u32*>(Pointer(y, 0));

        const s32 numValuesThisRow = MAX(0, MIN(numValues - y*size[1], size[1]));

        if(numValuesThisRow > 0) {
          // For small data types, this may be too many bytes, but the stride padding should make
          // the writing okay (I think)
          const s32 numWordsToCopy = (sizeof(Type)*numValuesThisRow + 3) / 4;

          //memcpy(pThisData, values + y*size[1], numValuesThisRow*sizeof(Type));
          for(s32 x=0; x<numWordsToCopy; x++) {
            //assert(reinterpret_cast<size_t>(values+y*size[1]) % 4 == 0);
            pThisData[x] = reinterpret_cast<const u32*>(values+y*size[1])[x];
          }
          numValuesSet += numValuesThisRow;
        }

        if(numValuesThisRow < size[1]) {
          memset(pThisData+numValuesThisRow*sizeof(Type), 0, (size[1]-numValuesThisRow)*sizeof(Type));
        }
      }

      return numValuesSet;
    }

    template<typename Type> Array<Type>& Array<Type>::operator= (const Array<Type> & rightHandSide)
    {
      this->size[0] = rightHandSide.size[0];
      this->size[1] = rightHandSide.size[1];

      this->stride = rightHandSide.stride;
      this->flags = rightHandSide.flags;
      this->data = rightHandSide.data;

      return *this;
    }

    template<typename Type> s32 Array<Type>::get_size(s32 dimension) const
    {
      AnkiConditionalErrorAndReturnValue(dimension >= 0,
        0, "Array<Type>::get_size", "Negative dimension");

      if(dimension > 1 || dimension < 0)
        return 1;

      return size[dimension];
    }

    template<typename Type> s32 Array<Type>::get_stride() const
    {
      return stride;
    }

    template<typename Type> s32 Array<Type>::get_numElements() const
    {
      if(size[0] > 0 && size[1] > 0) {
        return size[0] * size[1];
      } else {
        return 0;
      }
    }

    template<typename Type> void* Array<Type>::get_buffer()
    {
      return data;
    }

    template<typename Type> const void* Array<Type>::get_buffer() const
    {
      return data;
    }

    template<typename Type> Flags::Buffer Array<Type>::get_flags() const
    {
      return flags;
    }

    template<typename Type> void* Array<Type>::AllocateBufferFromMemoryStack(const s32 numRows, const s32 stride, MemoryStack &memory, s32 &numBytesAllocated, const Flags::Buffer flags, bool reAllocate)
    {
      AnkiConditionalError(numRows >= 0 && stride > 0,
        "Array<Type>::AllocateBufferFromMemoryStack", "Invalid size");

      const s32 numRowsCapped = MAX(numRows, 1);

      this->stride = stride;

      const s32 numBytesRequested = numRowsCapped * this->stride;

      if(reAllocate) {
        return memory.Reallocate(this->data, numBytesRequested, numBytesAllocated);
      } else {
        return memory.Allocate(numBytesRequested, flags.get_zeroAllocatedMemory(), numBytesAllocated);
      }
    }

    template<typename Type> Result Array<Type>::InitializeBuffer(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const Flags::Buffer flags)
    {
      if(!rawData) {
        AnkiError("Anki.Array2d.initialize", "input data buffer is NULL, numRows=%d, numCols=%d, dataLength=%d",
                  numRows, numCols, dataLength);
        InvalidateArray();
        return RESULT_FAIL_UNINITIALIZED_MEMORY;
      }

      AnkiConditionalErrorAndReturnValue(numCols >= 0 && numRows >= 0 && dataLength >= MEMORY_ALIGNMENT,
        RESULT_FAIL_INVALID_SIZE, "Array<Type>::InitializeBuffer", "Negative dimension");

      AnkiConditionalErrorAndReturnValue(!flags.get_useBoundaryFillPatterns(),
        RESULT_FAIL_INVALID_PARAMETER, "Array<Type>::InitializeBuffer", "Fill patterns not supported for Array");

      this->flags = flags;
      this->size[0] = numRows;
      this->size[1] = numCols;

      // Initialize an empty array.

      this->data = reinterpret_cast<Type*>(rawData);

      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData), MEMORY_ALIGNMENT) - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,flags)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
        AnkiError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
        InvalidateArray();
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      this->data = reinterpret_cast<Type*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );

      //#if ANKICORETECH_EMBEDDED_USE_OPENCV
      //      this->UpdateCvMatMirror(*this);
      //#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

      return RESULT_OK;
    } // Array<Type>::InitializeBuffer()

    // Set all the buffers and sizes to -1, to signal an invalid array
    template<typename Type> void Array<Type>::InvalidateArray()
    {
      this->size[0] = -1;
      this->size[1] = -1;
      this->stride = -1;
      this->data = NULL;
    } // void Array<Type>::InvalidateArray()

    template<typename Type> Result Array<Type>::PrintBasicType(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX)  const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::Print", "Array<Type> is not valid");

      const s32 realMinX = MAX(0,minX);
      const s32 realMaxX = MIN(maxX+1,size[1]);
      const s32 realMinY = MAX(0,minY);
      const s32 realMaxY = MIN(maxY+1,size[0]);

      CoreTechPrint("%s type(int:%d,signed:%d,float:%d,sizeof:%zu):\n", variableName, Flags::TypeCharacteristics<Type>::isInteger, Flags::TypeCharacteristics<Type>::isSigned, Flags::TypeCharacteristics<Type>::isFloat, sizeof(Type));

      for(s32 y=realMinY; y<realMaxY; y++) {
        const Type * const pThisData = this->Pointer(y, 0);
        for(s32 x=realMinX; x<realMaxX; x++) {
          if(Flags::TypeCharacteristics<Type>::isBasicType) {
            if(Flags::TypeCharacteristics<Type>::isInteger) {
              CoreTechPrint("%d ", static_cast<s32>(pThisData[x]));
            } else {
              if(version==1) {
                CoreTechPrint("%f ", (float)pThisData[x]);
              } else {
                CoreTechPrint("%e ", (float)pThisData[x]);
              }
            }
          } else {
            CoreTechPrint("! ");
          }
        }
        CoreTechPrint("\n");
      }
      CoreTechPrint("\n");

      return RESULT_OK;
    }

    template<typename Type> Result Array<Type>::PrintString(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::PrintString", "Array<Type> is not valid");

      const s32 realMinX = MAX(0,minX);
      const s32 realMaxX = MIN(maxX+1,size[1]);
      const s32 realMinY = MAX(0,minY);
      const s32 realMaxY = MIN(maxY+1,size[0]);

      CoreTechPrint("%s:\n", variableName);

      for(s32 y=realMinY; y<realMaxY; y++) {
        const char * const * pThisData = this->Pointer(y, 0);
        for(s32 x=realMinX; x<realMaxX; x++) {
          const char * curString = pThisData[x];
          if(!curString) {
            CoreTechPrint("NULL, ");
          } else {
            CoreTechPrint("\"%s\", ", curString);
          }
        }
        CoreTechPrint("\n");
      }
      CoreTechPrint("\n");

      return RESULT_OK;
    }

    // #pragma mark --- FixedPointArray Definitions ---

    template<typename Type> FixedPointArray<Type>::FixedPointArray()
      : Array<Type>(), numFractionalBits(-1)
    {
    }

    template<typename Type> FixedPointArray<Type>::FixedPointArray(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const s32 numFractionalBits, const Flags::Buffer flags)
      : Array<Type>(numRows, numCols, data, dataLength, flags), numFractionalBits(numFractionalBits)
    {
      AnkiConditionalError(numFractionalBits >= 0 && numFractionalBits <= (sizeof(Type)*8),  "FixedPointArray<Type>", "numFractionalBits number is invalid");
    }

    template<typename Type> FixedPointArray<Type>::FixedPointArray(s32 numRows, s32 numCols, s32 numFractionalBits, MemoryStack &memory, const Flags::Buffer flags)
      : Array<Type>(numRows, numCols, memory, flags), numFractionalBits(numFractionalBits)
    {
      AnkiConditionalError(numFractionalBits >= 0 && numFractionalBits <= static_cast<s32>(sizeof(Type)*8),  "FixedPointArray<Type>", "numFractionalBits number is invalid");
    }

    template<typename Type> s32 FixedPointArray<Type>::get_numFractionalBits() const
    {
      return numFractionalBits;
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> Result ArrayToCvMat(const Array<Type> &in, cv::Mat *out)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid() && out,
        RESULT_FAIL, "ArrayToCvMat", "This Array is invalid");

//      out->refcount = NULL;
//
//      // These two should be set, because if the Mat_ constructor was not called, these will not be initialized
//      out->step.p = out->step.buf;
//      out->size = &out->rows;

      *out = cv::Mat_<Type>(in.get_size(0), in.get_size(1), const_cast<Type*>(in.Pointer(0,0)), static_cast<size_t>(in.get_stride()));

      return RESULT_OK;
    } // template<typename Type> Result ArrayToCvMat(const Array<Type> &in, cv::Mat *out)
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

    // #pragma mark --- Array Specializations ---

    template<> Result Array<bool>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<s8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<u16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<s16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<u32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<s32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<u64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<s64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<const char *>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<char *>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;

    template<> Result Array<f32>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f64>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;

    template<> template<> s32 Array<u8>::SetCast(const s32 * const values, const s32 numValues);
    template<> template<> s32 Array<s16>::SetCast(const s32 * const values, const s32 numValues);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
