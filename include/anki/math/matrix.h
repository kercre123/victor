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
    Matrix<T> operator*(const Matrix<T> &other) const;
    
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
    
  }
  
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols)
  : Array2d<T>(nrows, ncols)
  {
    
  }
  
  template<typename T>
  Matrix<T>::Matrix(const cv::Mat_<T> &cvMatrix)
  : Array2d<T>(cvMatrix)
  {
    
  } // Constructor: Matrix<T>(cv::Mat_<T>)
  
  
  template<typename T>
  Matrix<T> Matrix<T>::operator*(const Matrix<T> &other) const
  {
    // Make sure the matrices have compatible sizes for multiplication
    assert(this->numCols() == other.numRows());
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix multiplication:
    Matrix<T> result( cv::Mat_<T>(*this) * cv::Mat_<T>(other) );
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
    Matrix<T> result(this->inv());
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
    Matrix<T> result(this->t());
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
    
    return result;
    
  } // Matrix<T>::Transpose()
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
