#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/memory.h"
#include "anki/embeddedCommon/errorHandling.h"
#include "anki/embeddedCommon/dataStructures.h"
#include "anki/embeddedCommon/geometry.h"
#include "anki/embeddedCommon/utilities_c.h"

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#endif

// #define ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT

#define ANKI_ARRAY_USE_ARRAY_SET

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Array Class Definition ---

    template<typename Type> class Array
    {
    public:
      static s32 ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns);

      Array();

      // Constructor for a Array, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array, though the reverse is trivial.
      // All memory in the array is zeroed out once it is allocated
      Array(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array, pointing to user-allocated MemoryStack
      // All memory in the array is zeroed out once it is allocated
      Array(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict array_rowPointer = Array.Pointer(5);", then index
      // array_rowPointer in the inner loop.
      inline const Type* Pointer(const s32 index0, const s32 index1) const;
      inline Type* Pointer(const s32 index0, const s32 index1);

      // Use this operator for normal C-style 2d matrix indexing. For example, "array[5][0] = 6;"
      // will set the element in the fifth row and first column to 6. This is the same as
      // "array.Pointer(5)[0] = 6;"
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict array_rowPointer = Array[5];", then index
      // array_rowPointer in the inner loop.
      inline const Type * operator[](const s32 index0) const;
      inline Type * operator[](const s32 index0);

      // Pointer to the data, at a given (y,x) location
      //
      // NOTE:
      // Using this in a inner loop is very innefficient. Instead, declare a pointer outside the
      // inner loop, like: "Type * restrict array_rowPointer = Array.Pointer(Point<s16>(5,0));",
      // then index array_rowPointer in the inner loop.
      inline const Type* Pointer(const Point<s16> &point) const;
      inline Type* Pointer(const Point<s16> &point);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      // Returns a templated cv::Mat_ that shares the same buffer with this Array. No data is copied.
      cv::Mat_<Type>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      void Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues=false) const;

      // Check every element of this array against the input array. If the arrays are different
      // sizes, uninitialized, or if any element is more different than the threshold, then
      // return false.
      bool IsElementwiseEqual(const Array &array2, const Type threshold = static_cast<Type>(0.0001)) const;

      // Check every element of this array against the input array. If the arrays are different
      // sizes or uninitialized, return false. The percentThreshold is between 0.0 and 1.0. To
      // return false, an element must fail both thresholds. The percent threshold fails if an
      // element is more than a percentage different than its matching element (calulated from the
      // maximum of the two).
      bool IsElementwiseEqual_PercentThreshold(const Array &array2, const double percentThreshold = 0.01, const double absoluteThreshold = 0.0001) const;

      // If this array or array2 are different sizes or uninitialized, then return false.
      bool IsEqualSize(const Array &array2) const;

      // Print out the contents of this Array
      Result Print(const char * const variableName = "Array", const s32 minY = 0, const s32 maxY = 0x7FFFFFE, const s32 minX = 0, const s32 maxX = 0x7FFFFFE) const;

      // If the Array was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array was not constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Return the minimum element in this matrix
      Type Min() const;

      // Return the maximum element in this matrix
      Type Max() const;

      // Set every element in the Array to zero, including the stride padding
      void SetZero();

      // Set every element in the Array to this value
      // Returns the number of values set
      s32 Set(const Type value);

      // Copy values to this Array.
      // If the input array does not contain enough elements, the remainder of this Array will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      // Note: The myriad has many issues with static initialization of arrays, so this should not used with caution
#ifdef ANKI_ARRAY_USE_ARRAY_SET
      s32 Set(const Type * const values, const s32 numValues);
#endif

      // Parse a space-seperated string, and copy values to this Array.
      // If the string does not contain enough elements, the remainder of the Array will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
      s32 Set(const char * const values);
#endif

      Array& operator= (const Array & rightHandSide);

      // Similar to Matlabs size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      // Get the stride, which is the number of bytes between an element at (n,m) and one at (n+1,m)
      s32 get_stride() const;

      // Get the stride, without the optional fill pattern. This is the number of bytes on a
      // horizontal line that can safely be read and written to. If this array was created without
      // fill patterns, it returns the same value as get_stride().
      s32 get_strideWithoutFillPatterns() const;

      void* get_rawDataPointer();

      const void* get_rawDataPointer() const;

    protected:
      // Bit-inverse of MemoryStack patterns. The pattern will be put twice at
      // the beginning and end of each line.
      static const u32 FILL_PATTERN_START = 0X5432EF76;
      static const u32 FILL_PATTERN_END = 0X7610FE76;

      static const s32 HEADER_LENGTH = 8;
      static const s32 FOOTER_LENGTH = 8;

      s32 size[2];
      s32 stride;
      bool useBoundaryFillPatterns;

      Type * data;

      // To enforce alignment, rawDataPointer may be slightly before Type * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      //s32 cvMatMirror_sizeBuffer[2];
      cv::Mat_<Type> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      void Initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns);

      void InvalidateArray(); // Set all the buffers and sizes to zero, to signal an invalid array

    private:
    }; // class Array

#pragma mark --- FixedPointArray Class Definition ---

    template<typename Type> class FixedPointArray : public Array<Type>
    {
    public:
      FixedPointArray();

      // Constructor for a Array, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array, though the reverse is trivial.
      // All memory in the array is zeroed out once it is allocated
      FixedPointArray(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const s32 numFractionalBits, const bool useBoundaryFillPatterns=false);

      // Constructor for a Array, pointing to user-allocated MemoryStack
      // All memory in the array is zeroed out once it is allocated
      FixedPointArray(const s32 numRows, const s32 numCols, const s32 numFractionalBits, MemoryStack &memory, const bool useBoundaryFillPatterns=false);

      s32 get_numFractionalBits() const;

    protected:
      s32 numFractionalBits;
    };

#pragma mark --- Array Implementations ---

    // Factory method to create an Array from the heap. The data of the returned Array must be freed by the user.
    // This is separate from the normal constructor, as Array objects are not supposed to manage memory
#ifndef USING_MOVIDIUS_COMPILER
    template<typename Type> Array<Type> AllocateArrayFromHeap(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns=false)
    {
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array<Type>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more

      Array<Type> mat(numRows, numCols, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    }

    template<typename Type> FixedPointArray<Type> AllocateFixedPointArrayFromHeap(const s32 numRows, const s32 numCols, const s32 numFractionalBits, const bool useBoundaryFillPatterns=false)
    {
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array<Type>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more

      FixedPointArray<Type> mat(numRows, numCols, calloc(requiredMemory, 1), requiredMemory, numFractionalBits, useBoundaryFillPatterns);

      return mat;
    }
#endif // #ifndef USING_MOVIDIUS_COMPILER

    template<typename Type> s32 Array<Type>::ComputeRequiredStride(const s32 numCols, const bool useBoundaryFillPatterns)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0,
        0, "Array<Type>::ComputeRequiredStride", "Invalid size");

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(Type)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    template<typename Type> s32 Array<Type>::ComputeMinimumRequiredMemory(const s32 numRows, const s32 numCols, const bool useBoundaryFillPatterns)
    {
      AnkiConditionalErrorAndReturnValue(numCols > 0 && numRows > 0,
        0, "Array<Type>::ComputeMinimumRequiredMemory", "Invalid size");

      return numRows * Array<Type>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    template<typename Type> Array<Type>::Array()
    {
      InvalidateArray();
    }

    template<typename Type> Array<Type>::Array(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      AnkiConditionalError(numCols > 0 && numRows > 0 && dataLength > 0,
        "Array<Type>::Array", "Invalid size");

      Initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    template<typename Type> Array<Type>::Array(const s32 numRows, const s32 numCols, MemoryStack &memory, const bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      AnkiConditionalError(numCols > 0 && numRows > 0,
        "Array<Type>::Array", "Invalid size");

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      Initialize(numRows,
        numCols,
        reinterpret_cast<Type*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

    template<typename Type> const Type* Array<Type>::Pointer(const s32 index0, const s32 index1) const
    {
      AnkiConditionalWarnAndReturnValue(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1],
        0, "Array<Type>::Pointer", "Invalid size");

      AnkiConditionalWarnAndReturnValue(this->IsValid(),
        0, "Array<Type>::Pointer", "Array<Type> is not valid");

      return reinterpret_cast<const Type*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(Type) + index0*stride );
    }

    template<typename Type> Type* Array<Type>::Pointer(const s32 index0, const s32 index1)
    {
      AnkiConditionalWarnAndReturnValue(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1],
        0, "Array<Type>::Pointer", "Invalid size");

      AnkiConditionalWarnAndReturnValue(this->IsValid(),
        0, "Array<Type>::Pointer", "Array<Type> is not valid");

      return reinterpret_cast<Type*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(Type) + index0*stride );
    }

    template<typename Type> inline const Type * Array<Type>::operator[](const s32 index0) const
    {
      return Pointer(index0, 0);
    }

    template<typename Type> inline Type * Array<Type>::operator[](const s32 index0)
    {
      return Pointer(index0, 0);
    }

    template<typename Type> const Type* Array<Type>::Pointer(const Point<s16> &point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    template<typename Type> Type* Array<Type>::Pointer(const Point<s16> &point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    template<typename Type> cv::Mat_<Type>& Array<Type>::get_CvMat_()
    {
      AnkiConditionalError(this->IsValid(), "Array<Type>::get_CvMat_", "Array<Type> is not valid");

      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    template<typename Type> void  Array<Type>::Show(const char * const windowName, const bool waitForKeypress, const bool scaleValues) const
    {
      // If opencv is not used, just do nothing
#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      AnkiConditionalError(this->IsValid(), "Array<Type>::Show", "Array<Type> is not valid");

      if(scaleValues) {
        cv::Mat_<f64> scaledArray(this->get_size(0), this->get_size(1));
        scaledArray = cvMatMirror;

        const f64 minValue = this->Min();
        const f64 maxValue = this->Max();
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
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    }

    template<typename Type> bool Array<Type>::IsElementwiseEqual(const Array &array2, const Type threshold) const
    {
      if(!this->IsEqualSize(array2))
        return false;

      for(s32 y=0; y<size[0]; y++) {
        const Type * const this_rowPointer = this->Pointer(y, 0);
        const Type * const array2_rowPointer = array2.Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          if(this_rowPointer[x] > array2_rowPointer[x]) {
            if((this_rowPointer[x] - array2_rowPointer[x]) > threshold)
              return false;
          } else {
            if((array2_rowPointer[x] - this_rowPointer[x]) > threshold)
              return false;
          }
        }
      }

      return true;
    }

    template<typename Type> bool Array<Type>::IsElementwiseEqual_PercentThreshold(const Array &array2, const double percentThreshold, const double absoluteThreshold) const
    {
      if(!this->IsEqualSize(array2))
        return false;

      for(s32 y=0; y<size[0]; y++) {
        const Type * const this_rowPointer = this->Pointer(y, 0);
        const Type * const array2_rowPointer = array2.Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          const double value1 = static_cast<double>(this_rowPointer[x]);
          const double value2 = static_cast<double>(array2_rowPointer[x]);
          const double percentThresholdValue = percentThreshold * MAX(value1,value2);

          if(fabs(value1 - value2) > percentThresholdValue && fabs(value1 - value2) > absoluteThreshold)
            return false;
        }
      }

      return true;
    }

    template<typename Type> bool Array<Type>::IsEqualSize(const Array &array2) const
    {
      if(!this->IsValid())
        return false;

      if(!array2.IsValid())
        return false;

      if(this->get_size(0) != array2.get_size(0) || this->get_size(1) != array2.get_size(1))
        return false;

      return true;
    }

    template<typename Type> Result Array<Type>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        RESULT_FAIL, "Array<Type>::Print", "Array<Type> is not valid");

      printf(variableName);
      printf(":\n");
      for(s32 y=MAX(0,minY); y<MIN(maxY+1,size[0]); y++) {
        const Type * const rowPointer = Pointer(y, 0);
        for(s32 x=MAX(0,minX); x<MIN(maxX+1,size[1]); x++) {
          printf("%d ", rowPointer[x]);
        }
        printf("\n");
      }
      printf("\n");

      return RESULT_OK;
    }

    template<typename Type> bool Array<Type>::IsValid() const
    {
      if(this->rawDataPointer == NULL || this->data == NULL) {
        return false;
      }

      if(size[0] < 1 || size[1] < 1) {
        return false;
      }

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(s32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true;
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Set every element in the Array to zero, including the stride padding, but not including the optional fill patterns (if they exist)
    template<typename Type> void Array<Type>::SetZero()
    {
      AnkiConditionalError(this->IsValid(), "Array<Type>::SetZero", "Array<Type> is not valid");

      const s32 strideWithoutFillPatterns = this->get_strideWithoutFillPatterns();
      for(s32 y=0; y<size[0]; y++) {
        char * restrict rowPointer = reinterpret_cast<char*>(Pointer(y, 0));
        memset(rowPointer, 0, strideWithoutFillPatterns);
      }
    }

#ifdef ANKI_ARRAY_USE_ARRAY_SET
    // Note: The myriad has many issues with static initialization of arrays, so this should not used with caution
    template<typename Type> s32 Array<Type>::Set(const Type value)
    {
      AnkiConditionalError(this->IsValid(), "Array<Type>::Set", "Array<Type> is not valid");

      for(s32 y=0; y<size[0]; y++) {
        Type * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }
#endif // ANKI_ARRAY_USE_ARRAY_SET

    template<typename Type> s32 Array<Type>::Set(const Type * const values, const s32 numValues)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Set", "Array<Type> is not valid");

      s32 numValuesSet = 0;

      for(s32 y=0; y<size[0]; y++) {
        Type * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          if(numValuesSet < numValues)
          {
            const Type value = values[numValuesSet++];
            rowPointer[x] = value;
          } else {
            rowPointer[x] = 0;
          }
        }
      }

      return numValuesSet;
    }

#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    template<typename Type> s32 Array<Type>::Set(const char * const values)
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Set", "Array<Type> is not valid");

      s32 numValuesSet = 0;

      const char * startPointer = values;
      char * endPointer = NULL;

      for(s32 y=0; y<size[0]; y++) {
        Type * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          const Type value = static_cast<Type>(strtol(startPointer, &endPointer, 10));
          if(startPointer != endPointer) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
          startPointer = endPointer;
        }
      }

      return numValuesSet;
    }
#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT

    template<typename Type> Array<Type>& Array<Type>::operator= (const Array<Type> & rightHandSide)
    {
      this->size[0] = rightHandSide.size[0];
      this->size[1] = rightHandSide.size[1];

      this->stride = rightHandSide.stride;
      this->useBoundaryFillPatterns = rightHandSide.useBoundaryFillPatterns;
      this->data = rightHandSide.data;
      this->rawDataPointer = rightHandSide.rawDataPointer;

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      // These two should be set, because if the Array constructor was not called, these will not be initialized
      this->cvMatMirror.step.p = this->cvMatMirror.step.buf;
      this->cvMatMirror.size = &this->cvMatMirror.rows;

      this->cvMatMirror = cv::Mat_<Type>(rightHandSide.size[0], rightHandSide.size[1], rightHandSide.data, rightHandSide.stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      return *this;
    }

    template<typename Type> s32 Array<Type>::get_size(s32 dimension) const
    {
      AnkiConditionalErrorAndReturnValue(dimension >= 0,
        0, "Array<Type>::get_size", "Negative dimension");

      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::get_size", "Array<Type> is not valid");

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    template<typename Type> s32 Array<Type>::get_stride() const
    {
      return stride;
    }

    template<typename Type> s32 Array<Type>::get_strideWithoutFillPatterns() const
    {
      const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);
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

    template<typename Type> void Array<Type>::Initialize(const s32 numRows, const s32 numCols, void * const rawData, const s32 dataLength, const bool useBoundaryFillPatterns)
    {
      AnkiConditionalErrorAndReturn(numCols > 0 && numRows > 0 && dataLength > 0,
        "Array<Type>::Initialize", "Negative dimension");

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
        AnkiError("Anki.Array2d.initialize", "input data buffer is NULL");
        InvalidateArray();
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
        AnkiError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
        InvalidateArray();
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
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
      // TODO: if this is slow, make this optional (or just remove it)
      this->SetZero();

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<Type>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array<Type>::Initialize()

    // Set all the buffers and sizes to zero, to signal an invalid array
    template<typename Type> void Array<Type>::InvalidateArray()
    {
      this->size[0] = 0;
      this->size[1] = 0;
      this->stride = 0;
      this->data = NULL;
      this->rawDataPointer = NULL;
    } // void Array<Type>::InvalidateArray()

    template<typename Type> Type Array<Type>::Min() const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Min", "Array<Type> is not valid");

      Type minValue = *this->Pointer(0, 0);
      for(s32 y=0; y<size[0]; y++) {
        const Type * const this_rowPointer = this->Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          minValue = MIN(minValue, this_rowPointer[x]);
        }
      }

      return minValue;
    }

    template<typename Type> Type Array<Type>::Max() const
    {
      AnkiConditionalErrorAndReturnValue(this->IsValid(),
        0, "Array<Type>::Min", "Array<Type> is not valid");

      Type maxValue = *this->Pointer(0, 0);
      for(s32 y=0; y<size[0]; y++) {
        const Type * const this_rowPointer = this->Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          maxValue = MAX(maxValue, this_rowPointer[x]);
        }
      }

      return maxValue;
    }

#pragma mark --- FixedPointArray Implementations ---

    template<typename Type> FixedPointArray<Type>::FixedPointArray()
      : Array<Type>(), numFractionalBits(-1)
    {
    }

    template<typename Type> FixedPointArray<Type>::FixedPointArray(const s32 numRows, const s32 numCols, void * const data, const s32 dataLength, const s32 numFractionalBits, const bool useBoundaryFillPatterns)
      : Array<Type>(numRows, numCols, data, dataLength, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
    {
      AnkiConditionalError(numFractionalBits >= 0 && numFractionalBits <= (sizeof(Type)*8),  "FixedPointArray<Type>", "numFractionalBits number is invalid");
    }

    template<typename Type> FixedPointArray<Type>::FixedPointArray(s32 numRows, s32 numCols, s32 numFractionalBits, MemoryStack &memory, bool useBoundaryFillPatterns)
      : Array<Type>(numRows, numCols, memory, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
    {
      AnkiConditionalError(numFractionalBits >= 0 && numFractionalBits <= static_cast<s32>(sizeof(Type)*8),  "FixedPointArray<Type>", "numFractionalBits number is invalid");
    }

    template<typename Type> s32 FixedPointArray<Type>::get_numFractionalBits() const
    {
      return numFractionalBits;
    }

#pragma mark --- Array Specializations ---

    template<> Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<f64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<Point<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<Rectangle<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<Quadrilateral<s16> >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;

    //#ifdef USING_MOVIDIUS_GCC_COMPILER
    //    template<> s32 Array<u8>::Set(const u8 * const values, const s32 numValues);
    //    template<> s32 Array<s8>::Set(const s8 * const values, const s32 numValues);
    //    template<> s32 Array<u16>::Set(const u16 * const values, const s32 numValues);
    //    template<> s32 Array<s16>::Set(const s16 * const values, const s32 numValues);
    //    template<> s32 Array<u32>::Set(const u32 * const values, const s32 numValues);
    //    template<> s32 Array<s32>::Set(const s32 * const values, const s32 numValues);
    //    template<> s32 Array<u64>::Set(const u64 * const values, const s32 numValues);
    //    template<> s32 Array<s64>::Set(const s64 * const values, const s32 numValues);
    //    template<> s32 Array<f32>::Set(const f32 * const values, const s32 numValues);
    //    template<> s32 Array<f64>::Set(const f64 * const values, const s32 numValues);
    //#endif

#ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
    template<> s32 Array<f32>::Set(const char * const values);
    template<> s32 Array<f64>::Set(const char * const values);
#endif // #ifdef ANKICORETECHEMBEDDED_ARRAY_STRING_INPUT
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAY2D_H_
