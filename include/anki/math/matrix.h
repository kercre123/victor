#ifndef _ANKICORETECH_MATRIX_H_
#define _ANKICORETECH_MATRIX_H_

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "anki/common/array2d.h"
#include "anki/common/point.h"

namespace Anki {
  
#pragma mark --- Matrix Class Definiton ---
  
  // A class for a general matrix:
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
    
    // NOTE: element access inherited from Array2d
    
    // Matrix multiplication:
    Matrix<T> operator*(const Matrix<T> &other) const;
    
    // Matrix inversion:
    // TODO: provide way to specify method
    void      Invert(void); // in place
    Matrix<T> getInverse(void) const;
    
    // Matrix tranpose:
    Matrix<T> getTranpose(void) const;
    void      Transpose(void); // in place
    
    
  }; // class Matrix

  
  // A class for small matrices, whose size is known at compile time
  template<typename T, unsigned int NROWS, unsigned NCOLS>
  class SmallMatrix
#if defined(ANKICORETECH_USE_OPENCV)
  : private cv::Matx<T,NROWS,NCOLS> // private inheritance from cv::Matx
#endif
  {
  public:
    // Constructors:
    SmallMatrix();
    SmallMatrix(const T* vals); // *assumes* vals is NROWS*NCOLS long
    
    // Matrix element access:
    T&       operator() (unsigned int i, unsigned int j);
    const T& operator() (unsigned int i, unsigned int j) const;
    
    // Matrix multiplication:
    // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
    template<unsigned int KCOLS>
    SmallMatrix<T,NROWS,KCOLS> operator* (const SmallMatrix<T,NCOLS,KCOLS> &other) const;
    
    // Matrix inversion:
    void Invert(void);
    SmallMatrix<T,NROWS,NCOLS> getInverse(void) const;
    
    // Matrix transpose:
    SmallMatrix<T,NCOLS,NROWS> getTranspose(void) const;

  }; // class SmallMatrix
  
  
  // Typedef some common small matrix sizes, like 3x3
  typedef SmallMatrix<float,3,3> Matrix_3x3f;
  typedef SmallMatrix<float,3,4> Matrix_3x4f;
  
    
  
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
  
  
#if defined(ANKICORETECH_USE_OPENCV)
  template<typename T>
  Matrix<T>::Matrix(const cv::Mat_<T> &cvMatrix)
  : Array2d<T>(cvMatrix)
  {
    
  } // Constructor: Matrix<T>(cv::Mat_<T>)
#endif
  
  
  template<typename T>
  Matrix<T> Matrix<T>::operator*(const Matrix<T> &other) const
  {
    // Make sure the matrices have compatible sizes for multiplication
    assert(this->numCols() == other.numRows());
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix multiplication:
    Matrix<T> result( this->get_CvMat_() * other.get_CvMat_() );
#else
    assert(false);
    // TODO: Implement our own matrix multiplication.
#endif
    
    return result;
    
  } // Matrix<T>::operator*()
  
  template<typename T>
  void Matrix<T>::Invert(void)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix inversion:
    *this = this->get_CvMat_().inv();
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
  } // Matrix<T>::Invert()
  
  template<typename T>
  Matrix<T> Matrix<T>::getInverse(void) const
  {
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix inversion:
    Matrix<T> result(this->get_CvMat_().inv());
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
    
    return result;
  } // Matrix<T>::getInverse()
  
                   
  
  template<typename T>
  Matrix<T> Matrix<T>::getTranpose(void) const
  {
    
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix inversion:
    Matrix<T> result;
    cv::transpose(this->get_CvMat_(), result.get_CvMat_());
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
    
    return result;
    
  } // Matrix<T>::getTranspose()
  
  
  template<typename T>
  void Matrix<T>::Transpose(void)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    // For now (?), rely on OpenCV for matrix transpose:
    this->get_CvMat_().t();
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  } // Matrix<T>::Tranpose()
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
