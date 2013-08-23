#ifndef _ANKICORETECH_MATRIX_H_
#define _ANKICORETECH_MATRIX_H_

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "anki/common/array2d.h"

namespace Anki {
  
#pragma mark --- Matrix Class Definiton ---
  
  template<typename T>
  class Matrix : public Array2d<T>
  {
  public:
    Matrix(); 
    Matrix(s32 nrows, s32 ncols);
    
#if defined(ANKICORETECH_USE_OPENCV)
    // Construct from an OpenCv cv::Mat_<T>.
    // NOTE: this *copies* all the data, to ensure the result
    //       is memory-aligned the way want.
    Matrix(const cv::Mat_<T> &cvMatrix);
#endif
    
    // Matrix multiplication:
    Matrix<T> operator*(const Matrix<T> &other);
    
    // Matrix inversion:
    // TODO: provide way to specify method
    Matrix<T> Inverse(void) const;
    
    // Matrix tranpose:
    Matrix<T> Tranpose(void) const;
    
  }; // class Matrix
  
  
#pragma mark --- Matrix Implementations ---
  
  template<typename T>
  Matrix<T>::Matrix()
  : Array2d<T>()
  {
    assert(false);
    // TODO: implement default constructor (need one for base class?)
  }
  
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols)
  : Array2d<T>(nrows, ncols)
  {
    
  }
  
  template<typename T>
  Matrix<T>::Matrix(const cv::Mat_<T> &cvMatrix)
  : Matrix<T>(cvMatrix.rows, cvMatrix.cols)
  {
    assert(this->get_nrows() == cvMatrix.rows &&
           this->get_ncols() == cvMatrix.cols);
    
    // Copy the data into the result.  This ensures that the
    // result still has memory-aligned data the way we want
    // (the way our Array2d class assumes), and that we
    // haven't accidentally let OpenCv's create() do things
    // differently.
    // TODO: Find a way to get this without the copy.
    for(s32 i=0; i<this->get_nrows(); ++i) {
      
      // Get pointers to the beginning of the row:
      const T *cvMatrix_i = cvMatrix[i];
      T *this_i     = this->cvMatMirror[i];
      
      for(s32 j=0; j<this->get_ncols(); ++j) {
        this_i[j] = cvMatrix_i[j];
      } // FOR j
      
    } // FOR i
    
  } // Constructor: Matrix<T>(cv::Mat_<T>)
  
  
  template<typename T>
  Matrix<T> Matrix<T>::operator*(const Matrix<T> &other)
  {
    // Make sure the matrices have compatible sizes for multiplication
    assert(this->ncols == other.nrows);

    // Create the output matrix:
    Matrix<T> result(this->get_nrows(), other.get_ncols());
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix multiplication:
    result.cvMatMirror = this->cvMatMirror * other.cvMatMirror;
#else
    assert(false);
    // TODO: Implement our own matrix multiplication.
#endif
    
    return result;
    
  } // Matrix<T>::operator*()
  
  
  template<typename T>
  Matrix<T> Matrix<T>::Inverse(void) const
  {
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix inversion:
    Matrix<T> result(this->cvMatMirror.inv());
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
    
    return result;
  } // Matrix<T>::Inverse()
  
  
  template<typename T>
  Matrix<T> Matrix<T>::Tranpose(void) const
  {
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix inversion:
    Matrix<T> result(this->cvMatMirror.t());
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
    
    return result;
    
  } // Matrix<T>::Transpose()
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
