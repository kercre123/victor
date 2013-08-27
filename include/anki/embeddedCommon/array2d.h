#ifndef _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_
#define _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_

#include "anki/embeddedCommon/config.h"
#include "anki/embeddedCommon/utilities.h"
#include "anki/embeddedCommon/memory.h"
#include "anki/embeddedCommon/DASlight.h"
#include "anki/embeddedCommon/dataStructures.h"

#include <iostream>
#include <assert.h>

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif



namespace Anki
{
  namespace Embedded
  {
    template<typename T> class Point2;

#pragma mark --- Array2d Class Definition ---

    // A Array2d is a lightweight templated class for holding two dimensional data. It does no
    // reference counting, or allocating/freeing from the heap. The data from Array2d is
    // OpenCV-compatible, and accessable via get_CvMat_(). The matlabInterface.h can send and receive
    // Array2d objects from Matlab
    template<typename T>
    class Array2d
    {
    public:
      static s32 ComputeRequiredStride(s32 numCols, bool useBoundaryFillPatterns);

      static s32 ComputeMinimumRequiredMemory(s32 numRows, s32 numCols, bool useBoundaryFillPatterns);

      Array2d();

      // Constructor for a Array2d, pointing to user-allocated data. If the pointer to *data is not
      // aligned to MEMORY_ALIGNMENT, this Array2d will start at the next aligned location.
      // Unfortunately, this is more restrictive than most matrix libraries, and as an example,
      // it may make it hard to convert from OpenCV to Array2d, though the reverse is trivial.
      Array2d(s32 numRows, s32 numCols, void * data, s32 dataLength, bool useBoundaryFillPatterns=false);

      // Constructor for a Array2d, pointing to user-allocated MemoryStack
      Array2d(s32 numRows, s32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns=false);

      // Pointer to the data, at a given (y,x) location
      const inline T* Pointer(s32 index0, s32 index1) const;

      // Pointer to the data, at a given (y,x) location
      inline T* Pointer(s32 index0, s32 index1);

      // Pointer to the data, at a given (y,x) location
      template<typename TPoint> const inline T* Pointer(Point2<TPoint> point) const;

      // Pointer to the data, at a given (y,x) location
      template<typename TPoint> inline T* Pointer(Point2<TPoint> point);

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      void Show(const std::string windowName, bool waitForKeypress) const;

      // Returns a templated cv::Mat_ that shares the same buffer with this Array2d. No data is copied.
      cv::Mat_<T>& get_CvMat_();
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      // Print out the contents of this Array2d
      void Print() const;

      // If the Array2d was constructed with the useBoundaryFillPatterns=true, then
      // return if any memory was written out of bounds (via fill patterns at the
      // beginning and end).  If the Array2d wasn't constructed with the
      // useBoundaryFillPatterns=true, this method always returns true
      bool IsValid() const;

      // Set every element in the Array2d to this value
      // Returns the number of values set
      s32 Set(T value);

      // Parse a space-seperated string, and copy values to this Array2d.
      // If the string doesn't contain enough elements, the remainded of the Array2d will be filled with zeros.
      // Returns the number of values set (not counting extra zeros)
      s32 Set(const std::string values);

      // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

      inline s32 get_nrows(void) const;
      inline s32 get_ncols(void) const;
      inline s32 get_numElements(void) const;

      s32 get_stride() const;

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

      T * data;

      // To enforce alignment, rawDataPointer may be slightly before T * data.
      // If the inputted data buffer was from malloc, this is the pointer that
      // should be used to free.
      void * rawDataPointer;

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cv::Mat_<T> cvMatMirror;
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

      void initialize(s32 numRows, s32 numCols, void * rawData, s32 dataLength,
        bool useBoundaryFillPatterns);
    }; // class Array2d

# pragma mark --- Array2dFixedPoint Definition ---

    template<typename T>
    class Array2dFixedPoint : public Array2d<T>
    {
    public:
      Array2dFixedPoint();

      // Constructor for a Array2dFixedPoint, pointing to user-allocated data.
      Array2dFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
        void * data, s32 dataLength, bool useBoundaryFillPatterns=false);

      // Constructor for a Array2dFixedPoint, pointing to user-allocated MemoryStack
      Array2dFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
        MemoryStack &memory, bool useBoundaryFillPatterns=false);

      Array2dFixedPoint(Array2d<T> &mat);

      bool IsValid() const;

      s32 get_numFractionalBits() const;

    protected:
      s32 numFractionalBits;
    }; // class Array2dFixedPoint

#pragma mark --- FixedLengthList Definition ---

    template<typename T>
    class FixedLengthList : public Array2d<T>
    {
    public:
      FixedLengthList();

      // Constructor for a FixedLengthList, pointing to user-allocated data.
      FixedLengthList(s32 maximumSize, void * data, s32 dataLength,
        bool useBoundaryFillPatterns=false);

      // Constructor for a FixedLengthList, pointing to user-allocated MemoryStack
      FixedLengthList(s32 maximumSize, MemoryStack &memory,
        bool useBoundaryFillPatterns=false);

      bool IsValid() const;

      Result PushBack(const T &value);

      // Will act as a normal pop, except when the list is empty. Then subsequent
      // calls will keep returning the first value in the list.
      T PopBack();

      void Clear();

      // Pointer to the data, at a given location
      inline T* Pointer(s32 index);

      // Pointer to the data, at a given location
      inline const T* Pointer(s32 index) const;

      s32 get_maximumSize() const;

      s32 get_size() const;

    protected:
      s32 capacityUsed;
    }; // class FixedLengthList

#pragma mark --- Array2d Implementations ---

    template<typename T>
    s32 Array2d<T>::ComputeRequiredStride(s32 numCols, bool useBoundaryFillPatterns)
    {
      assert(numCols > 0);
      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? (HEADER_LENGTH+FOOTER_LENGTH) : 0);
      return static_cast<s32>(RoundUp<size_t>(sizeof(T)*numCols, MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    template<typename T>
    s32 Array2d<T>::ComputeMinimumRequiredMemory(s32 numRows, s32 numCols,
      bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0);
      return numRows * Array2d<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    template<typename T>
    Array2d<T>::Array2d()
      : stride(0), useBoundaryFillPatterns(false), data(NULL), rawDataPointer(NULL)
    {
      this->size[0] = 0;
      this->size[1] = 0;
    }

    template<typename T>
    Array2d<T>::Array2d(s32 numRows, s32 numCols, void * data, s32 dataLength,
      bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    template<typename T>
    Array2d<T>::Array2d(s32 numRows, s32 numCols, MemoryStack &memory,
      bool useBoundaryFillPatterns)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      assert(numCols > 0 && numRows > 0);

      const s32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? static_cast<s32>(MEMORY_ALIGNMENT) : 0);
      const s32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      s32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<T*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

    template<typename T>
    const T* Array2d<T>::Pointer(s32 index0, s32 index1) const
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<const T*>( reinterpret_cast<const char*>(this->data) +
        index1*sizeof(T) + index0*stride );
    }

    template<typename T>
    T* Array2d<T>::Pointer(s32 index0, s32 index1)
    {
      assert(index0 >= 0 && index1 >= 0 && index0 < size[0] && index1 < size[1] &&
        this->rawDataPointer != NULL && this->data != NULL);

      return reinterpret_cast<T*>( reinterpret_cast<char*>(this->data) +
        index1*sizeof(T) + index0*stride );
    }

    template<typename T>
    template<typename TPoint>
    const T* Array2d<T>::Pointer(Point2<TPoint> point) const
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

    template<typename T>
    template<typename TPoint>
    T* Array2d<T>::Pointer(Point2<TPoint> point)
    {
      return Pointer(static_cast<s32>(point.y), static_cast<s32>(point.x));
    }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    template<typename T>
    void Array2d<T>::Show(const std::string windowName, bool waitForKeypress) const {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      cv::imshow(windowName, cvMatMirror);
      if(waitForKeypress) {
        cv::waitKey();
      }
    }

    template<typename T>
    cv::Mat_<T>& Array2d<T>::get_CvMat_()
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)

    template<typename T>
    void Array2d<T>::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const T * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          std::cout << rowPointer[x] << " ";
        }
        std::cout << "\n";
      }
    }

    template<typename T>
    bool Array2d<T>::IsValid() const
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
        return true; // Technically, we don't know if the Array2d is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }
    template<typename T>
    s32 Array2d<T>::Set(T value)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        T * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          rowPointer[x] = value;
        }
      }

      return size[0]*size[1];
    }

    template<typename T>
    s32 Array2d<T>::Set(const std::string values)
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      std::istringstream iss(values);
      s32 numValuesSet = 0;

      for(s32 y=0; y<size[0]; y++) {
        T * restrict rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          T value;
          if(iss >> value) {
            rowPointer[x] = value;
            numValuesSet++;
          } else {
            rowPointer[x] = 0;
          }
        }
      }

      return numValuesSet;
    }

    template<typename T>
    s32 Array2d<T>::get_size(s32 dimension) const
    {
      assert(dimension >= 0 && this->rawDataPointer != NULL && this->data != NULL);

      if(dimension > 1 || dimension < 0)
        return 0;

      return size[dimension];
    }

    template<typename T>
    s32 Array2d<T>::get_nrows(void) const
    {
      return this->size[0];
    }

    template<typename T>
    s32 Array2d<T>::get_ncols(void) const
    {
      return this->size[1];
    }

    template<typename T>
    s32 Array2d<T>::get_numElements(void) const
    {
      return (this->size[0] * this->size[1]);
    }

    template<typename T>
    s32 Array2d<T>::get_stride() const
    {
      return stride;
    }

    template<typename T>
    void* Array2d<T>::get_rawDataPointer()
    {
      return rawDataPointer;
    }

    template<typename T>
    const void* Array2d<T>::get_rawDataPointer() const
    {
      return rawDataPointer;
    }

    template<typename T>
    void Array2d<T>::initialize(s32 numRows, s32 numCols, void * rawData,
      s32 dataLength, bool useBoundaryFillPatterns)
    {
      assert(numCols > 0 && numRows > 0 && dataLength > 0);

      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
        DASError("Anki.Array2d.initialize", "input data buffer is NULL");
        this->size[0] = 0;
        this->size[1] = 0;
        this->data = NULL;
        this->rawDataPointer = NULL;
        return;
      }

      this->rawDataPointer = rawData;

      const size_t extraBoundaryPatternBytes = useBoundaryFillPatterns ? static_cast<size_t>(HEADER_LENGTH) : 0;
      const s32 extraAlignmentBytes = static_cast<s32>(RoundUp<size_t>(reinterpret_cast<size_t>(rawData)+extraBoundaryPatternBytes, MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<size_t>(rawData));
      const s32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
        DASError("Anki.Array2d.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
        this->size[0] = 0;
        this->size[1] = 0;
        this->data = NULL;
        this->rawDataPointer = NULL;
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const s32 strideWithoutFillPatterns = ComputeRequiredStride(size[1], false);
        this->data = reinterpret_cast<T*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(s32 y=0; y<size[0]; y++) {
          // Add the fill patterns just before the data on each line
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0] = FILL_PATTERN_START;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1] = FILL_PATTERN_START;

          // And also just after the data (including normal byte-alignment padding)
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0] = FILL_PATTERN_END;
          reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1] = FILL_PATTERN_END;
        }
      } else {
        this->data = reinterpret_cast<T*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes );
      }

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
      cvMatMirror = cv::Mat_<T>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
    } // Array2d<T>::initialize()

#pragma mark --- Array2d Specializations ---

    template<> void Array2d<u8>::Print() const;
    template<> s32 Array2d<u8>::Set(const std::string values);

#pragma mark --- Array2dFixedPoint Implementations ---

    template<typename T>
    Array2dFixedPoint<T>::Array2dFixedPoint()
      : Array2d<T>(), numFractionalBits(0)
    {
    }

#pragma mark --- FixedPointMatrix Implementations ---

    template<typename T>
    Array2dFixedPoint<T>::Array2dFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
      void * data, s32 dataLength, bool useBoundaryFillPatterns)
      : Array2d<T>(numRows, numCols, data, dataLength, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
    {
      assert(numFractionalBits >= 0 && numFractionalBits <= (sizeof(T)*8));
    }

    template<typename T>
    Array2dFixedPoint<T>::Array2dFixedPoint(s32 numRows, s32 numCols, s32 numFractionalBits,
      MemoryStack &memory, bool useBoundaryFillPatterns)
      : Array2d<T>(numRows, numCols, memory, useBoundaryFillPatterns), numFractionalBits(numFractionalBits)
    {
      assert(numFractionalBits >= 0 && numFractionalBits <= (sizeof(T)*8));
    }

    template<typename T>
    Array2dFixedPoint<T>::Array2dFixedPoint(Array2d<T> &mat)
      : Array2d<T>(mat), numFractionalBits(0)
    {
    }

    template<typename T>
    bool Array2dFixedPoint<T>::IsValid() const
    {
      if(numFractionalBits > (sizeof(T)*8)) {
        return false;
      }

      return Array2d<T>::IsValid();
    }

    template<typename T>
    s32 Array2dFixedPoint<T>::get_numFractionalBits() const
    {
      return numFractionalBits;
    }

#pragma mark --- FixedLengthList Implementations

    template<typename T>
    FixedLengthList<T>::FixedLengthList()
      : Array2d<T>(), capacityUsed(0)
    {
    }

    template<typename T>
    FixedLengthList<T>::FixedLengthList(s32 maximumSize, void * data, s32 dataLength,
      bool useBoundaryFillPatterns)
      : Array2d<T>(1, maximumSize, data, dataLength, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    template<typename T>
    FixedLengthList<T>::FixedLengthList(s32 maximumSize, MemoryStack &memory,
      bool useBoundaryFillPatterns)
      : Array2d<T>(1, maximumSize, memory, useBoundaryFillPatterns), capacityUsed(0)
    {
    }

    template<typename T>
    bool FixedLengthList<T>::IsValid() const
    {
      if(capacityUsed > this->get_maximumSize()) {
        return false;
      }

      return Array2d<T>::IsValid();
    }

    template<typename T>
    Result FixedLengthList<T>::PushBack(const T &value)
    {
      if(capacityUsed >= this->get_maximumSize()) {
        return RESULT_FAIL;
      }

      *this->Pointer(capacityUsed) = value;
      capacityUsed++;

      return RESULT_OK;
    }

    template<typename T>
    T FixedLengthList<T>::PopBack()
    {
      if(capacityUsed == 0) {
        return *this->Pointer(0);
      }

      const T value = *this->Pointer(capacityUsed-1);
      capacityUsed--;

      return value;
    }

    template<typename T>
    void FixedLengthList<T>::Clear()
    {
      this->capacityUsed = 0;
    }

    template<typename T>
    T* FixedLengthList<T>::Pointer(s32 index)
    {
      return Array2d<T>::Pointer(0, index);
    }

    // Pointer to the data, at a given location
    template<typename T>
    const T* FixedLengthList<T>::Pointer(s32 index) const
    {
      return Array2d<T>::Pointer(0, index);
    }

    template<typename T>
    s32 FixedLengthList<T>::get_maximumSize() const
    {
      return Array2d<T>::get_size(1);
    }

    template<typename T>
    s32 FixedLengthList<T>::get_size() const
    {
      return capacityUsed;
    }

#pragma mark --- Helper Functions ---

    template<typename T1, typename T2>
    bool AreMatricesEqual_Size(const Array2d<T1> &mat1, const Array2d<T2> &mat2)
    {
      if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
        mat1.get_size(0) == mat2.get_size(0) &&
        mat1.get_size(1) == mat2.get_size(1)) {
          return true;
      }

      return false;
    } // AreMatricesEqual_Size()

    template<typename T> bool AreMatricesEqual_SizeAndType(const Array2d<T> &mat1, const Array2d<T> &mat2)
    {
      if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
        mat1.get_size(0) == mat2.get_size(0) &&
        mat1.get_size(1) == mat2.get_size(1)) {
          return true;
      }

      return false;
    } // AreMatricesEqual_SizeAndType()

    // Factory method to create an Array2d from the heap. The data of the returned Array2d must be freed by the user.
    // This is seperate from the normal constructor, as Array2d objects are not supposed to manage memory
    template<typename T> Array2d<T> AllocateArray2dFromHeap(s32 numRows, s32 numCols, bool useBoundaryFillPatterns=false)
    {
      // const s32 stride = Array2d<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array2d<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

      Array2d<T> mat(numRows, numCols, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    } // AllocateArray2dFromHeap()

    template<typename T> Array2dFixedPoint<T> AllocateArray2dFixedPointFromHeap(s32 numRows, s32 numCols, s32 numFractionalBits, bool useBoundaryFillPatterns=false)
    {
      // const s32 stride = Array2dFixedPoint<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array2dFixedPoint<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

      Array2dFixedPoint<T> mat(numRows, numCols, numFractionalBits, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    }

    template<typename T> FixedLengthList<T> AllocateFixedLengthListFromHeap(s32 maximumSize, bool useBoundaryFillPatterns=false)
    {
      // const s32 stride = FixedLengthList<T>::ComputeRequiredStride(maximumSize, useBoundaryFillPatterns);
      const s32 requiredMemory = 64 + 2*MEMORY_ALIGNMENT + Array2d<T>::ComputeMinimumRequiredMemory(1, maximumSize, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

      FixedLengthList<T> mat(maximumSize, calloc(requiredMemory, 1), requiredMemory, useBoundaryFillPatterns);

      return mat;
    } // AllocateArray2dFixedPointFromHeap()
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_MATRIX_H_