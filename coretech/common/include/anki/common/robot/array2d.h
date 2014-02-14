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

#include "anki/common/robot/array2d_declarations.h"

#include "anki/common/robot/utilities.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/geometry.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/robot/cInterfaces_c.h"
#include "anki/common/robot/sequences.h"
#include "anki/common/robot/matrix.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    //template<typename Type1, typename Type2> class Find;

    // #pragma mark --- Array Definitions ---

    // Factory method to create an Array from the heap. The data of the returned Array must be freed by the user.
    // This is separate from the normal constructor, as Array objects are not supposed to manage memory
#ifndef USING_MOVIDIUS_COMPILER
    //template<typename Type> Array<Type> AllocateArrayFromHeap(const s32 numRows, const s32 numCols, const Flags::Buffer flags=Flags::Buffer(true,false))
    //{
    //  const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array<Type>::ComputeMinimumRequiredMemory(numRows, numCols, flags); // The required memory, plus a bit more

    //  Type * const rawDataPointer = reinterpret_cast<Type*>(calloc(requiredMemory, 1));
    //  Type * const dataPointer = reinterpret_cast<Type*>(RoundUp<size_t>(reinterpret_cast<size_t>(rawDataPointer), MEMORY_ALIGNMENT));

    //  const s32 offsetAmount = static_cast<s32>(reinterpret_cast<size_t>(dataPointer) - reinterpret_cast<size_t>(rawDataPointer));

    //  Array<Type> mat(numRows, numCols, dataPointer, requiredMemory-offsetAmount, flags);

    //  mat.set_rawDataPointer(rawDataPointer);

    //  return mat;
    //}

    //template<typename Type> FixedPointArray<Type> AllocateFixedPointArrayFromHeap(const s32 numRows, const s32 numCols, const s32 numFractionalBits, const Flags::Buffer flags=Flags::Buffer(true,false))
    //{
    //  const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array<Type>::ComputeMinimumRequiredMemory(numRows, numCols, flags); // The required memory, plus a bit more

    //  Type * const rawDataPointer = reinterpret_cast<Type*>(calloc(requiredMemory, 1));
    //  Type * const dataPointer = reinterpret_cast<Type*>(RoundUp<size_t>(reinterpret_cast<size_t>(rawDataPointer), MEMORY_ALIGNMENT));

    //  const s32 offsetAmount = static_cast<s32>(reinterpret_cast<size_t>(dataPointer) - reinterpret_cast<size_t>(rawDataPointer));

    //  FixedPointArray<Type> mat(numRows, numCols, dataPointer, requiredMemory-offsetAmount, numFractionalBits, flags);

    //  mat.set_rawDataPointer(rawDataPointer);

    //  return mat;
    //}
#endif // #ifndef USING_MOVIDIUS_COMPILER

    template<typename Type> s32 Array<Type>::ComputeRequiredStride(const s32 numCols, const Flags::Buffer flags)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0,
        0, "Array<Type>::ComputeRequiredStride", "Invalid size");

      const s32 bufferRequired = static_cast<s32>(RoundUp<size_t>(sizeof(Type)*numCols, MEMORY_ALIGNMENT));

      const s32 extraBoundaryPatternBytes = flags.get_useBoundaryFillPatterns() ? (HEADER_LENGTH+FOOTER_LENGTH) : 0;

      const s32 totalRequired = bufferRequired + extraBoundaryPatternBytes;

      return totalRequired;
    }

    template<typename Type> s32 Array<Type>::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const Flags::Buffer flags)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0 && numRows > 0,
        0, "Array<Type>::ComputeMinimumRequiredMemory", "Invalid size");

      return numRows * Array<Type>::ComputeRequiredStride(numCols, flags);
    }

    template<typename Type> Array<Type>::Array()
    {
      InvalidateArray();
    }

    template<typename Type> Array<Type>::Array(const s32 numRows, const s32 numCols, void * data, const s32 dataLength, const Flags::Buffer flags)
    {
#if defined(USING_MOVIDIUS_COMPILER)
#if defined(USING_MOVIDIUS_GCC_COMPILER)
      data = ConvertCMXAddressToLeon(data);
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
      data = ConvertCMXAddressToShave(data);
#else
#error Unknown Movidius compiler
#endif
#endif // #if defined(USING_MOVIDIUS_COMPILER)

      InvalidateArray();

      AnkiConditionalErrorAndReturn(reinterpret_cast<size_t>(data)%MEMORY_ALIGNMENT == 0,
        "Array::Array", "If fully allocated, data must be %d byte aligned", MEMORY_ALIGNMENT);

      this->stride = ComputeRequiredStride(numCols, flags);

      AnkiConditionalErrorAndReturn(numCols >= 0 && numRows >= 0 && dataLength >= numRows*this->stride,
        "Array<Type>::Array", "Invalid size");

      if(flags.get_isFullyAllocated()) {
        AnkiConditionalErrorAndReturn(this->stride == (numCols*static_cast<s32>(sizeof(Type))),
          "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, the stride must be simple");

        AnkiConditionalErrorAndReturn((numCols*sizeof(Type)) % MEMORY_ALIGNMENT == 0,
          "Array<Type>::Array", "if the data buffer being passed in doesn't contain a raw buffer, (numCols*sizeof(Type)) mod MEMORY_ALIGNMENT must equal zero");

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

#if defined(USING_MOVIDIUS_COMPILER)
#if defined(USING_MOVIDIUS_GCC_COMPILER)
      allocatedBuffer = ConvertCMXAddressToLeon(allocatedBuffer);
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
      allocatedBuffer = ConvertCMXAddressToShave(allocatedBuffer);
#else
#error Unknown Movidius compiler
#endif
#endif // #if defined(USING_MOVIDIUS_COMPILER)

      InitializeBuffer(numRows,
        numCols,
        reinterpret_cast<Type*>(allocatedBuffer),
        numBytesAllocated,
        flags);
    }

    template<typename Type> const Type* Array<Type>::Pointer(const s32 index0, const s32 index1) const
    {
      AnkiConditionalWarnAndReturnValue(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1],
        0, "Array<Type>::Pointer", "Invalid location");

      AnkiConditionalWarnAndReturnValue(this->IsValid(),
        0, "Array<Type>::Pointer", "Array<Type> is not valid");

      return reinterpret_cast<const Type*>( reinterpret_cast<const char*>(this->data) + index0*stride ) + index1;
    }

    template<typename Type> Type* Array<Type>::Pointer(const s32 index0, const s32 index1)
    {
      AnkiConditionalWarnAndReturnValue(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1],
        0, "Array<Type>::Pointer", "Invalid location");

      AnkiConditionalWarnAndReturnValue(this->IsValid(),
        0, "Array<Type>::Pointer", "Array<Type> is not valid");

      return reinterpret_cast<Type*>( reinterpret_cast<char*>(this->data) + index0*stride ) + index1;
    }

    template<typename Type> inline const Type * Array<Type>::operator[](const s32 index0) const
    {
      return reinterpret_cast<const Type*>( reinterpret_cast<const char*>(this->data) + index0*stride );
    }

    template<typename Type> inline Type * Array<Type>::operator[](const s32 index0)
    {
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
    template<typename Type> cv::Mat_<Type>& Array<Type>::get_CvMat_()
    {
      AnkiConditionalError(this->IsValid(), "Array<Type>::get_CvMat_", "Array<Type> is not valid");

      return cvMatMirror;
    }
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> void Array<Type>::Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues, const bool fitImageToWindow) const
    {
      AnkiConditionalError(this->IsValid(), "Array<Type>::Show", "Array<Type> is not valid");

      if(fitImageToWindow) {
        cv::namedWindow(windowName, CV_WINDOW_NORMAL);
      } else {
        cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
      }

      if(scaleValues) {
        cv::Mat_<f64> scaledArray(this->get_size(0), this->get_size(1));
        scaledArray = cvMatMirror;

        const f64 minValue = Matrix::Min<Type>(*this);
        const f64 maxValue = Matrix::Max<Type>(*this);
        const f64 range = maxValue - minValue;

        scaledArray -= minValue;
        scaledArray /= range;

        cv::imshow(windowName, scaledArray);
      } else {
        cv::imshow(windowName, cvMatMirror);
      }

      if(waitForKeypress) {
        cv::waitKey();
      }
    }
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

    template<typename Type> Result Array<Type>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::Print", "Array<Type> is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Type * const pThisData = this->Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          pThisData[x].Print();
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    }

    template<typename Type> Result Array<Type>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return this->Print(variableName, minY, maxY, minX, maxX);
    }

    template<typename Type> bool Array<Type>::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(flags.get_useBoundaryFillPatterns()) {
        Flags::Buffer flagsWithoutBoundary = flags;
        flagsWithoutBoundary.set_useBoundaryFillPatterns(false);
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],flagsWithoutBoundary);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(flags.get_useBoundaryFillPatterns()) {
        return true;
      } // if(flags.get_useBoundaryFillPatterns()) { ... else
    }

    template<typename Type> Result Array<Type>::Resize(const s32 numRows, const s32 numCols, MemoryStack &memory)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0 && numRows > 0,
        RESULT_FAIL_INVALID_SIZE, "Array<Type>::Resize", "Invalid size");

      s32 numBytesAllocated = 0;

      this->rawDataPointer = AllocateBufferFromMemoryStack(numRows, ComputeRequiredStride(numCols, flags), memory, numBytesAllocated, flags, true);

      // Don't clear the reallocated memory
      const bool clearMemory = this->flags.get_zeroAllocatedMemory();
      this->flags.set_zeroAllocatedMemory(false);

      const Result result = InitializeBuffer(numRows,
        numCols,
        this->rawDataPointer,
        numBytesAllocated,
        flags);

      this->flags.set_zeroAllocatedMemory(clearMemory);

      return result;
    }

    template<typename Type> s32 Array<Type>::SetZero()
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::SetZero", "Array<Type> is not valid");

      const s32 strideWithoutFillPatterns = this->get_strideWithoutFillPatterns();
      for(s32 y=0; y<size[0]; y++) {
        char * restrict pThisData = reinterpret_cast<char*>(Pointer(y, 0));
        memset(pThisData, 0, strideWithoutFillPatterns);
      }

      return strideWithoutFillPatterns*size[0];
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

      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Set", "this Array is not valid");

      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        0, "Array<Type>::Set", "Array in is not valid");

      AnkiConditionalErrorAndReturnValue(inHeight == this->size[0] && inWidth == this->size[1],
        0, "Array<Type>::Set", "Array sizes don't match");

      for(s32 y=0; y<size[0]; y++) {
        const InType * restrict pIn = in.Pointer(y, 0);
        Type * restrict pThisData = Pointer(y, 0);

        for(s32 x=0; x<size[1]; x++) {
          pThisData[x] = static_cast<Type>(pIn[x]);
        }
      }

      return size[0]*size[1];
    }

    template<typename InType> s32 SetCast(const InType * const values, const s32 numValues)
    {
      // This is a little tough to write a general case for, so this method should be specialized
      // for each relevant case
      AnkiAssert(false);
			
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
            AnkiAssert(reinterpret_cast<size_t>(values+y*size[1]) % 4 == 0);
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
      this->rawDataPointer = rightHandSide.rawDataPointer;

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      // These two should be set, because if the Array constructor was not called, these will not be initialized
      this->cvMatMirror.step.p = this->cvMatMirror.step.buf;
      this->cvMatMirror.size = &this->cvMatMirror.rows;

      this->cvMatMirror = cv::Mat_<Type>(rightHandSide.size[0], rightHandSide.size[1], rightHandSide.data, rightHandSide.stride);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

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

    template<typename Type> s32 Array<Type>::get_strideWithoutFillPatterns() const
    {
      Flags::Buffer flagsWithoutBoundary = this->flags;
      flagsWithoutBoundary.set_useBoundaryFillPatterns(false);

      const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],flagsWithoutBoundary);
      return strideWithoutFillPatterns;
    }

    template<typename Type> void* Array<Type>::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    template<typename Type> const void* Array<Type>::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    template<typename Type> Flags::Buffer Array<Type>::get_flags() const
    {
      return flags;
    }

    template<typename Type> void* Array<Type>::AllocateBufferFromMemoryStack(const s32 numRows, const s32 stride, MemoryStack &memory, s32 &numBytesAllocated, const Flags::Buffer flags, bool reAllocate)
    {
      AnkiConditionalError(numRows > 0 && stride > 0,
        "Array<Type>::AllocateBufferFromMemoryStack", "Invalid size");

      this->stride = stride;

      const s32 extraBoundaryPatternBytes = flags.get_useBoundaryFillPatterns() ? static_cast<s32>(MEMORY_ALIGNMENT) : 0;
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;

      if(reAllocate) {
        return memory.Reallocate(this->rawDataPointer, numBytesRequested, numBytesAllocated);
      } else {
        return memory.Allocate(numBytesRequested, numBytesAllocated);
      }
    }

    template<typename Type> Result Array<Type>::InitializeBuffer(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const Flags::Buffer flags)
    {
      AnkiConditionalErrorAndReturnValue(numCols >= 0 && numRows >= 0 && dataLength >= 0,
        RESULT_FAIL_INVALID_SIZE, "Array<Type>::InitializeBuffer", "Negative dimension");

      this->flags = flags;
      this->size[0] = numRows;
      this->size[1] = numCols;

      // Initialize an empty array.
      //
      // An empty array is invalid, and will return false from
      // Array::IsValid(), but is a possible return value from some functions
      if(numCols == 0 || numRows == 0) {
        this->rawDataPointer = NULL;
        this->data = NULL;
        return RESULT_OK;
      }

      if(!rawData) {
        AnkiError("Anki.Array2d.initialize", "input data buffer is NULL");
        InvalidateArray();
        return RESULT_FAIL_UNINITIALIZED_MEMORY;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = flags.get_useBoundaryFillPatterns() ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,flags)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
        AnkiError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
        InvalidateArray();
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(flags.get_useBoundaryFillPatterns()) {
        Flags::Buffer flagsWithoutBoundary = flags;
        flagsWithoutBoundary.set_useBoundaryFillPatterns(false);
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], flagsWithoutBoundary);
        this->data = reinterpret_cast<Type*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<Type*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

      // Zero out the entire buffer
      if(flags.get_zeroAllocatedMemory())
        this->SetZero();

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      cvMatMirror = cv::Mat_<Type>(size[0], size[1], data, stride);
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV

      return RESULT_OK;
    } // Array<Type>::InitializeBuffer()

    // Set all the buffers and sizes to -1, to signal an invalid array
    template<typename Type> void Array<Type>::InvalidateArray()
    {
      this->size[0] = -1;
      this->size[1] = -1;
      this->stride = -1;
      this->data = NULL;
      this->rawDataPointer = NULL;
    } // void Array<Type>::InvalidateArray()

    template<typename Type> Result Array<Type>::PrintBasicType(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX)  const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Array<Type>::Print", "Array<Type> is not valid");

      const s32 realMinX = MAX(0,minX);
      const s32 realMaxX = MIN(maxX+1,size[1]);
      const s32 realMinY = MAX(0,minY);
      const s32 realMaxY = MIN(maxY+1,size[0]);

      printf("%s type(int:%d,signed:%d,float:%d,sizeof:%d):\n", variableName, Flags::TypeCharacteristics<Type>::isInteger, Flags::TypeCharacteristics<Type>::isSigned, Flags::TypeCharacteristics<Type>::isFloat, sizeof(Type));

      for(s32 y=realMinY; y<realMaxY; y++) {
        const Type * const pThisData = this->Pointer(y, 0);
        for(s32 x=realMinX; x<realMaxX; x++) {
          if(Flags::TypeCharacteristics<Type>::isBasicType) {
            if(Flags::TypeCharacteristics<Type>::isInteger) {
              if(sizeof(Type) == 1) {
                printf("%d ", static_cast<s32>(pThisData[x]));
              } else {
                printf("%d ", pThisData[x]);
              }
            } else {
              if(version==1) {
                printf("%f ", pThisData[x]);
              } else {
                printf("%e ", pThisData[x]);
              }
            }
          } else {
            printf("! ");
          }
        }
        printf("\n");
      }
      printf("\n");

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

    template<> Result Array<f32>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f64>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;

    template<> template<> s32 Array<u8>::SetCast(const s32 * const values, const s32 numValues);
    template<> template<> s32 Array<s16>::SetCast(const s32 * const values, const s32 numValues);

    // #pragma mark --- C Conversions ---
    C_Array_s32 get_C_Array_s32(Array<s32> &array);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
