#ifndef _ANKICORETECH_COMMON_MATRIX_H_
#define _ANKICORETECH_COMMON_MATRIX_H_

#include "anki/common/config.h"
#include "anki/common/utilities.h"
#include "anki/common/memory.h"
#include "anki/common/DASlight.h"

#include <iostream>
#include <assert.h>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

namespace Anki
{
  // A Matrix is a lightweight templated class for holding two dimentional data. It does no
  // reference counting, or allocating/freeing from the heap. The data from Matrix is
  // OpenCV-compatible, and accessable via get_CvMat_(). The matlabInterface.h can send and receive
  // Matrix objects from Matlab
  template<typename T> class Matrix
  {
  public:
    static u32 ComputeRequiredStride(u32 numCols, bool useBoundaryFillPatterns)
    {
      const u32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? 16 : 0);
      return static_cast<u32>(Anki::RoundUp<size_t>(sizeof(T)*numCols, Anki::MEMORY_ALIGNMENT)) + extraBoundaryPatternBytes;
    }

    static u32 ComputeMinimumRequiredMemory(u32 numRows, u32 numCols, bool useBoundaryFillPatterns)
    {
      return numRows * Anki::Matrix<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    }

    // Constructor for a Matrix, pointing to user-allocated data. If the pointer to *data is not
    // aligned to Anki::MEMORY_ALIGNMENT, this Matrix will start at the next aligned location. Unfortunately, this is more
    // restrictive than most matrix libraries, and as an example, it may make it hard to convert from
    // OpenCV to Anki::Matrix, though the reverse is trivial.
    Matrix(u32 numRows, u32 numCols, void * data, u32 dataLength, bool useBoundaryFillPatterns=false)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      initialize(numRows,
        numCols,
        data,
        dataLength,
        useBoundaryFillPatterns);
    }

    // Constructor for a Matrix, pointing to user-allocated MemoryStack
    Matrix(u32 numRows, u32 numCols, MemoryStack &memory, bool useBoundaryFillPatterns=false)
      : stride(ComputeRequiredStride(numCols, useBoundaryFillPatterns))
    {
      const u32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? Anki::MEMORY_ALIGNMENT : 0);
      const u32 numBytesRequested = numRows * this->stride + extraBoundaryPatternBytes;
      u32 numBytesAllocated = 0;

      void * allocatedBuffer = memory.Allocate(numBytesRequested, &numBytesAllocated);

      initialize(numRows,
        numCols,
        reinterpret_cast<T*>(allocatedBuffer),
        numBytesAllocated,
        useBoundaryFillPatterns);
    }

    // Pointer to the data, at a given (y,x) location
    const inline T* Pointer(u32 index0, u32 index1) const
    {
      assert(index0 < size[0] && index1 < size[1]);
      return reinterpret_cast<const T*>( reinterpret_cast<const char*>(this->data) + index1*sizeof(T) + index0*stride );
    }

    // Pointer to the data, at a given (y,x) location
    inline T* Pointer(u32 index0, u32 index1)
    {
      assert(index0 < size[0] && index1 < size[1]);
      return reinterpret_cast<T*>( reinterpret_cast<char*>(this->data) + index1*sizeof(T) + index0*stride );
    }

#if defined(ANKICORETECH_USE_OPENCV)
    // Returns a templated cv::Mat_ that shares the same buffer with this Anki::Matrix. No data is copied.
    cv::Mat_<T>& get_CvMat_()
    {
      return cvMatMirror;
    }
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    // Print out the contents of this matrix
    void Print() {
      for(u32 y=0; y<size[0]; y++) {
        T * rowPointer = Pointer(y, 0);
        for(u32 x=0; x<size[1]; x++) {
          std::cout << rowPointer[x] << " ";
        }
        std::cout << "\n";
      }
    }

    // If the Matrix was constructed with the useBoundaryFillPatterns=true, then return if any memory was written out of bounds (via fill patterns at the beginning and end)
    // If the Matrix wasn't constructed with the useBoundaryFillPatterns=true, this method always returns true
    bool IsValid()
    {
      if(useBoundaryFillPatterns) {
        const u32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);

        for(u32 y=0; y<size[0]; y++) {
          if((reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[0]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride - HEADER_LENGTH)[1]) != FILL_PATTERN_START ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[0]) != FILL_PATTERN_END ||
            (reinterpret_cast<u32*>( reinterpret_cast<char*>(this->data) + y*stride + strideWithoutFillPatterns)[1]) != FILL_PATTERN_END) {
              return false;
          }
        }

        return true;
      } else { // if(useBoundaryFillPatterns) {
        return true; // Technically, we don't know if the Matrix is valid. But we don't know it's NOT valid, so just return true.
      } // if(useBoundaryFillPatterns) { ... else
    }

    // Similar to Matlab's size(matrix, dimension), and dimension is in {0,1}
    u32 get_size(u32 dimension) const
    {
      if(dimension > 1)
        return 0;

      return size[dimension];
    }

    u32 get_stride() const
    {
      return stride;
    }

    void* get_rawDataPointer()
    {
      return rawDataPointer;
    }

    const void* get_rawDataPointer() const
    {
      return rawDataPointer;
    }

  protected:
    static const u32 FILL_PATTERN_START = 0X5432EF76; // Bit-inverse of MemoryStack patterns. The pattern will be put twice at the beginning and end of each line.
    static const u32 FILL_PATTERN_END = 0X7610FE76;
    //static const u32 FILL_PATTERN_START = 0xCCCCCCCC;
    //static const u32 FILL_PATTERN_END = 0xAAAAAAAA;

    static const u32 HEADER_LENGTH = 8;
    static const u32 FOOTER_LENGTH = 8;

    u32 size[2];
    u32 stride;
    bool useBoundaryFillPatterns;

    T * data;

    // To enforce alignment, rawDataPointer may be slightly before T * data.
    // If the inputted data buffer was from malloc, this is the pointer that
    // should be used to free.
    void * rawDataPointer;

#if defined(ANKICORETECH_USE_OPENCV)
    cv::Mat_<T> cvMatMirror;
#endif // #if defined(ANKICORETECH_USE_OPENCV)

    void initialize(u32 numRows, u32 numCols, void * rawData, u32 dataLength, bool useBoundaryFillPatterns) {
      this->useBoundaryFillPatterns = useBoundaryFillPatterns;

      if(!rawData) {
        DASError("Anki.Matrix.initialize", "input data buffer is NULL");
        this->size[0] = 0;
        this->size[1] = 0;
        this->data = NULL;
        this->rawDataPointer = NULL;
        return;
      }

      this->rawDataPointer = rawData;

      const u32 extraBoundaryPatternBytes = (useBoundaryFillPatterns ? HEADER_LENGTH : 0);
      const u32 extraAlignmentBytes = RoundUp(reinterpret_cast<u32>(rawData)+extraBoundaryPatternBytes, Anki::MEMORY_ALIGNMENT) - extraBoundaryPatternBytes - reinterpret_cast<u32>(rawData);
      const u32 requiredBytes = ComputeRequiredStride(numCols,useBoundaryFillPatterns)*numRows + extraAlignmentBytes;

      if(requiredBytes > dataLength) {
        DASError("Anki.Matrix.initialize", "Input data buffer is not large enough. %d bytes is required.", requiredBytes);
        this->size[0] = 0;
        this->size[1] = 0;
        this->data = NULL;
        this->rawDataPointer = NULL;
        return;
      }

      this->size[0] = numRows;
      this->size[1] = numCols;

      if(useBoundaryFillPatterns) {
        const u32 strideWithoutFillPatterns = ComputeRequiredStride(size[1],false);
        this->data = reinterpret_cast<T*>( reinterpret_cast<char*>(rawData) + extraAlignmentBytes + HEADER_LENGTH );
        for(u32 y=0; y<size[0]; y++) {
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

#if defined(ANKICORETECH_USE_OPENCV)
      cvMatMirror = cv::Mat_<T>(size[0], size[1], data, stride);
#endif // #if defined(ANKICORETECH_USE_OPENCV)
    }
  };

  template<> void Matrix<u8>::Print() {
    for(u32 y=0; y<size[0]; y++) {
      u8 * rowPointer = Pointer(y, 0);
      for(u32 x=0; x<size[1]; x++) {
        std::cout << static_cast<s32>(rowPointer[x]) << " ";
      }
      std::cout << "\n";
    }
  }

  template<typename T1, typename T2> bool AreMatricesEqual_Size(const Matrix<T1> &mat1, const Matrix<T2> &mat2)
  {
    if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
      mat1.get_size(0) == mat2.get_size(0) &&
      mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  }

  template<typename T> bool AreMatricesEqual_SizeAndType(const Matrix<T> &mat1, const Matrix<T> &mat2)
  {
    if(mat1.get_rawDataPointer() && mat2.get_rawDataPointer() &&
      mat1.get_size(0) == mat2.get_size(0) &&
      mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  }

  // Factory method to create an AnkiMatrix from the heap. The data of the returned Matrix must be freed by the user.
  // This is seperate from the normal constructor, as Matrix objects are not supposed to manage memory
  template<typename T> Matrix<T> AllocateMatrixFromHeap(u32 numRows, u32 numCols, bool useBoundaryFillPatterns=false)
  {
    const u32 stride = Anki::Matrix<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
    const u32 requiredMemory = 64 + 2*Anki::MEMORY_ALIGNMENT + Anki::Matrix<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns); // The required memory, plus a bit more just in case

    Matrix<T> mat(numRows, numCols, reinterpret_cast<u8*>(calloc(requiredMemory, 1)), requiredMemory, useBoundaryFillPatterns);

    return mat;
  }
} //namespace Anki

#endif // _ANKICORETECH_COMMON_MATRIX_H_