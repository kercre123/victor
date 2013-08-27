#ifndef _ANKICORETECH_MATRIX_H_
#define _ANKICORETECH_MATRIX_H_

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include "anki/common/array2d.h"
#include "anki/common/point.h"

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
    void      Invert(void); // in place
    Matrix<T> getInverse(void) const;
    
    // Matrix tranpose:
    Matrix<T> getTranpose(void) const;
    void      Transpose(void); // in place
    
    
  }; // class Matrix

  template<typename T>
  class Matrix3x3
  {
  public:
    Matrix3x3();
    Matrix3x3(const Point3<T> &col1, const Point3<T> &col2, const Point3<T> &col3);
  
    Point3<T>    operator* (const Point3<T>    &x) const;
    Matrix3x3<T> operator* (const Matrix3x3<T> &other) const;
    void         operator*=(const Matrix3x3<T> &other);
    
  protected:
    T data[9];
    T *row[3];
    
  }; // class Matrix3x3
  
  template<typename T>
  Matrix3x3<T>::Matrix3x3(void)
  {
    this->data = {
      static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
      static_cast<T>(0), static_cast<T>(0), static_cast<T>(0),
      static_cast<T>(0), static_cast<T>(0), static_cast<T>(0)};
    
    this->row[0] = data;
    this->row[1] = data + 3;
    this->row[2] = data + 6;
  }
  
  template<typename T>
  Point3<T> Matrix3x3<T>::operator*(const Point3<T> &p) const
  {
    return Point3<T>(row[0][0]*p.x + row[0][1]*p.y + row[0][2]*p.z,
                     row[1][0]*p.x + row[1][1]*p.y + row[1][2]*p.z,
                     row[2][0]*p.x + row[2][1]*p.y + row[2][2]*p.z);
  }
  
  template<typename T>
  Matrix3x3<T> Matrix3x3<T>::operator*(const Matrix3x3<T> &other) const
  {
    assert(false);
    // TODO: imeplement this
   
  }
  
  
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
