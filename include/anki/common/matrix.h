#ifndef _ANKICORETECH_COMMON_MATRIX_H_
#define _ANKICORETECH_COMMON_MATRIX_H_

#include "anki/common/config.h"
#include "anki/common/utilities.h"
#include "anki/common/memory.h"

#include <iostream>
#include <assert.h>

#if defined(ANKICORETECH_USE_OPENCV)
namespace cv
{
  template<typename _Tp> class Mat_;
}
#endif

namespace Anki
{ 
  // A Matrix is a lightweight templated class for holding two dimentional data. It does not allocate from the heap.
  // The data from Matrix is OpenCV-compatible, and accessable via get_CvMat_() and get_CvMat()
  // The MatlabInterface project can send and receive Matrix classes from Matlab
  template<typename T> class Matrix
  {
  public:
    static u32 ComputeMinimumRequiredStride(u32 numCols)
    {
      return static_cast<u32>(Anki::RoundUp<size_t>(sizeof(T)*numCols, Anki::MEMORY_ALIGNMENT));
    }

    static u32 ComputeMinimumRequiredMemory(u32 numRows, u32 numCols)
    {
      return numRows * Anki::Matrix<T>::ComputeMinimumRequiredStride(numCols);
    }

    // Factory method to create an AnkiMatrix from the heap. The data of the returned Matrix must be freed by the user.
    // This is seperate from the normal constructor, as it should not be used sparingly and not interchangably with normal Matrix constructors.
    static Anki::Matrix<T> AllocateMatrixFromHeap(u32 numRows, u32 numCols)
    {
      const u32 stride = Anki::Matrix<T>::ComputeMinimumRequiredStride(numCols);
      const u32 requiredMemory = 64 + 2*Anki::MEMORY_ALIGNMENT + Anki::Matrix<T>::ComputeMinimumRequiredMemory(numRows, numCols); // The required memory, plus a bit more just in case

      Anki::Matrix<u8> mat(numRows, numCols, reinterpret_cast<u8*>(calloc(requiredMemory, 1)), stride);

      return mat;
    }

    // Constructor for a Matrix, pointing to user-allocated data
    Matrix(u32 numRows, u32 numCols, T* data, u32 stride)
      : stride(stride)
    {
      initialize(numRows, 
                 numCols, 
                 data);
    }

    // Constructor for a Matrix, pointing to user-allocated MemoryStack
    Matrix(u32 numRows, u32 numCols, MemoryStack &memory)
      : stride(static_cast<u32>(Anki::RoundUp<size_t>(sizeof(T)*numCols, Anki::MEMORY_ALIGNMENT)))
    {
      initialize(numRows, 
                 numCols,  
                 static_cast<T*>(memory.Allocate(numRows*this->stride)));
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

    T* get_data()
    {
      return data;
    }

    const T* get_data() const
    {
      return data;
    }

  protected:
    u32 size[2];
    u32 stride;
    
    T* data;

    #if defined(ANKICORETECH_USE_OPENCV)
    cv::Mat_<T> cvMatMirror;
    #endif // #if defined(ANKICORETECH_USE_OPENCV)

    void initialize(u32 numRows, u32 numCols, T *data) {
      this->size[0] = numRows;
      this->size[1] = numCols;
      this->data = data;

      #if defined(ANKICORETECH_USE_OPENCV)
      cvMatMirror = cv::Mat_<T>(size[0], size[1], data, stride);
      #endif // #if defined(ANKICORETECH_USE_OPENCV)
    }
  };

  template<typename T1, typename T2> bool AreMatricesEqual_Size(const Matrix<T1> &mat1, const Matrix<T2> &mat2)
  {
    if(mat1.get_data() && mat2.get_data() &&
       mat1.get_size(0) == mat2.get_size(0) &&
       mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  }

  template<typename T> bool AreMatricesEqual_SizeAndType(const Matrix<T> &mat1, const Matrix<T> &mat2)
  {
    if(mat1.get_data() && mat2.get_data() &&
       mat1.get_size(0) == mat2.get_size(0) &&
       mat1.get_size(1) == mat2.get_size(1)) {
        return true;
    }

    return false;
  }
  
} //namespace Anki

#endif // _ANKICORETECH_COMMON_MATRIX_H_
