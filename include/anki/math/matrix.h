#ifndef _ANKICORETECH_MATRIX_H_
#define _ANKICORETECH_MATRIX_H_

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#include <ostream>
#include <cstdio>
#include "anki/common/array2d.h"

#include "anki/math/point.h"

using namespace std;

namespace Anki {
  
#pragma mark --- Matrix Class Definiton ---
  
  // A class for a general matrix:
  template<typename T>
  class Matrix : public Array2d<T>
  {
  public:
    Matrix(); 
    Matrix(s32 nrows, s32 ncols);
    Matrix(s32 nrows, s32 ncols, const T& initVal);
    
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
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
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
    
    unsigned int numRows() const;
    unsigned int numCols() const;
    
#if defined(ANKICORETECH_USE_OPENCV)
    SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix);
    cv::Matx<T,NROWS,NCOLS>& get_CvMatx_();
    const cv::Matx<T,NROWS,NCOLS>& get_CvMatx_() const;
#endif

  }; // class SmallMatrix
    
  // Typedef some common small matrix sizes, like 3x3
  typedef SmallMatrix<float,2,2> Matrix_2x2f;
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
    CORETECH_THROW_IF(nrows == 0 || ncols == 0);
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, const T &initVal)
  : Array2d<T>(nrows, ncols, initVal)
  {
    CORETECH_THROW_IF(nrows == 0 || ncols == 0);
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
    CORETECH_THROW_IF(this->numCols() != other.numRows());
    
    
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
  
  
  template<typename T>
  ostream& operator<<(ostream& out, const Matrix<T>& m)
  {
    for (int i=0; i<m.numRows(); ++i) {
      for (int j=0; j<m.numCols(); ++j) {
        out << m(i,j) << " ";
      }
      out << "\n";
    }
    return out;
  }
  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS>::SmallMatrix()
#if defined(ANKICORETECH_USE_OPENCV)
  : cv::Matx<T,NROWS,NCOLS>()
#endif
  {
#if (!defined(ANKICORETECH_USE_OPENCV))
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }
  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS>::SmallMatrix(const T* vals)
#if defined(ANKICORETECH_USE_OPENCV)
  : cv::Matx<T,NROWS,NCOLS>(vals)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }

  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS>::SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix)
#if defined(ANKICORETECH_USE_OPENCV)
  : cv::Matx<T,NROWS,NCOLS>(cvMatrix)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }

  
  
  // Matrix element access:
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  T&  SmallMatrix<T,NROWS,NCOLS>::operator() (unsigned int i, unsigned int j)
  {
    CORETECH_THROW_IF(i >= NROWS || j >= NCOLS);
      
    return cv::Matx<T,NROWS,NCOLS>::operator()(i,j);
  }
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  const T& SmallMatrix<T,NROWS,NCOLS>::operator() (unsigned int i, unsigned int j) const
  {
    CORETECH_THROW_IF(i >= NROWS || j >= NCOLS);
    
    return cv::Matx<T,NROWS,NCOLS>::operator()(i,j);
  }
  
  // Matrix multiplication:
  // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  template<unsigned int KCOLS>
  SmallMatrix<T,NROWS,KCOLS> SmallMatrix<T,NROWS,NCOLS>::operator* (const SmallMatrix<T,NCOLS,KCOLS> &other) const
  {
#if defined(ANKICORETECH_USE_OPENCV)
    SmallMatrix<T,NROWS,KCOLS> res;
    res = (*this) * other;
    return res;
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }
  
  // Matrix inversion:
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  void SmallMatrix<T,NROWS,NCOLS>::Invert(void)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    (*this) = this->inv();
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
    
  }
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS> SmallMatrix<T,NROWS,NCOLS>::getInverse(void) const
  {
#if defined(ANKICORETECH_USE_OPENCV)
    SmallMatrix<T,NROWS,NCOLS> res(this->inv());
    return res;
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
    
  }
  
  
  
  // Matrix transpose:
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NCOLS,NROWS> SmallMatrix<T,NROWS,NCOLS>::getTranspose(void) const
  {
#if defined(ANKICORETECH_USE_OPENCV)
    return this->t();
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }
  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  unsigned int SmallMatrix<T,NROWS,NCOLS>::numRows() const
  {
    return NROWS;
  }
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  unsigned int SmallMatrix<T,NROWS,NCOLS>::numCols() const
  {
    return NCOLS;
  }

#if defined(ANKICORETECH_USE_OPENCV)
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  cv::Matx<T,NROWS,NCOLS>& SmallMatrix<T,NROWS,NCOLS>::get_CvMatx_()
  {
    return (*this);
  }
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  const cv::Matx<T,NROWS,NCOLS>& SmallMatrix<T,NROWS,NCOLS>::get_CvMatx_() const
  {
    return (*this);
  }
  
#endif  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  ostream& operator<<(ostream& out, const SmallMatrix<T, NROWS, NCOLS>& m)
  {
    for (int i=0; i<NROWS; ++i) {
      for (int j=0; j<NCOLS; ++j) {
        out << m(i,j) << " ";
      }
      out << "\n";
    }
    return out;
  }
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
