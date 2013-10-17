/**
 * File: matrix.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements a few classes for storing matrices.  All are 
 *              on storage type.
 *
 *              - "Matrix" is a general container for a matrix whose size is
 *                unknown at compile time.  It inherits from Array2d and adds 
 *                math operations.
 *          
 *              - "SmallMatrix" is a container for a fixed-size matrix whose
 *                dimensions are known at compile time.
 * 
 *              - "SmallSquareMatrix" is a sub-class of SmallMatrix for storing
 *                fixed-size square matrices.  It adds square-matrix specific 
 *                math operations.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_MATRIX_H_
#define _ANKICORETECH_MATRIX_H_

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#include <ostream>
#include <cstdio>

#include "anki/common/basestation/array2d.h"
#include "anki/common/basestation/math/point.h"

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
    Matrix(s32 nrows, s32 ncols, T *data); // Assumes data is nrows*ncols!
    Matrix(s32 nrows, s32 ncols, std::vector<T> &data);

    using Array2d<T>::operator=;
    
#if ANKICORETECH_USE_OPENCV
    // Construct from an OpenCv cv::Mat_<T>.
    // NOTE: this *copies* all the data, to ensure the result
    //       is memory-aligned the way want.
    Matrix(const cv::Mat_<T> &cvMatrix);
#endif
    
    // NOTE: element access inherited from Array2d
    
    // Matrix multiplication:
    Matrix<T> operator*(const Matrix<T> &other) const;
    Matrix<T>& operator*=(const Matrix<T> &other);
    
    // Matrix inversion:
    // TODO: provide way to specify method
    void      Invert(void); // in place
    Matrix<T> getInverse(void) const;
    
    // Matrix tranpose:
    Matrix<T> getTranpose(void) const;
    void      Transpose(void); // in place
    
    
  }; // class Matrix

  
  // A class for small matrices, whose size is known at compile time
  template<size_t NROWS, size_t NCOLS, typename T>
  class SmallMatrix
#if ANKICORETECH_USE_OPENCV
  : private cv::Matx<T,NROWS,NCOLS> // private inheritance from cv::Matx
#endif
  {
  public:
    // Constructors:
    SmallMatrix();
    SmallMatrix(const T* vals); // *assumes* vals is NROWS*NCOLS long
    SmallMatrix(const SmallMatrix<NROWS,NCOLS,T> &M);
    
    // Matrix element access:
    T&       operator() (unsigned int i, unsigned int j);
    const T& operator() (unsigned int i, unsigned int j) const;
    
    // Matrix multiplication:
    // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
    template<size_t KCOLS>
    SmallMatrix<NROWS,KCOLS,T> operator* (const SmallMatrix<NCOLS,KCOLS,T> &other) const;
    
    // Matrix transpose:
    SmallMatrix<NCOLS,NROWS,T> getTranspose(void) const;
    
    // TODO: Add explicit pseudo-inverse? (inv(A'A)*A')
    // SmallMatrix<T,NCOLS,NROWS> getPsuedoInverse(void) const;
    
    unsigned int numRows() const;
    unsigned int numCols() const;
    
#if ANKICORETECH_USE_OPENCV
  public:
    SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix);
    cv::Matx<T,NROWS,NCOLS>& get_CvMatx_();
    const cv::Matx<T,NROWS,NCOLS>& get_CvMatx_() const;
    
    using cv::Matx<T,NROWS,NCOLS>::operator=;
#endif
    
  protected:
    
    T* getDataPtr(void);
    const T* getDataPtr(void) const;
    
    // Do we want to provide non-square matrix inversion?
    // I'm putting this here so the SmallSquareMatrix subclass can get
    // access to cv::Matx::inv() though.
    void Invert(void); // in place
    SmallMatrix<NROWS,NCOLS,T> getInverse(void) const;

  }; // class SmallMatrix
  
  // Comparisons for equalit and near-equality:
  template<size_t NROWS, size_t NCOLS, typename T>
  bool operator==(const SmallMatrix<NROWS,NCOLS,T> &M1,
                  const SmallMatrix<NROWS,NCOLS,T> &M2);
  
  template<size_t NROWS, size_t NCOLS, typename T>
  bool nearlyEqual(const SmallMatrix<NROWS,NCOLS,T> &M1,
                   const SmallMatrix<NROWS,NCOLS,T> &M2,
                   const T eps = std::numeric_limits<T>::epsilon());
  
  // An extension of the SmallMatrix class for square matrices
  template<size_t DIM, typename T>
  class SmallSquareMatrix : public SmallMatrix<DIM,DIM,T>
  {
  public:
    
    SmallSquareMatrix();
    SmallSquareMatrix(const SmallMatrix<DIM,DIM,T> &M);
    
    using SmallMatrix<DIM,DIM,T>::operator();
    
    // Matrix multiplication in place...
    // ... this = this * other;
    void operator*=(const SmallSquareMatrix<DIM,T> &other);
    // ... this = other * this;
    void preMultiplyBy(const SmallSquareMatrix<DIM,T> &other);
       
    // Transpose: (Note that we can transpose square matrices in place)
    using SmallMatrix<DIM,DIM,T>::getTranspose;
    SmallSquareMatrix<DIM,T>& Transpose(void);
    
    // Matrix inversion:
    void Invert(void); // in place
    SmallSquareMatrix<DIM,T> getInverse(void) const;
    
  }; // class SmallSquareMatrix
  
    
  // Typedef some common small matrix sizes, like 3x3
  typedef SmallSquareMatrix<2,float> Matrix_2x2f;
  typedef SmallSquareMatrix<3,float> Matrix_3x3f;
  typedef SmallMatrix<3,4,float> Matrix_3x4f;
  
  // Common 2D matrix-point multiplication:
  template<typename T>
  Point2<T> operator*(const SmallSquareMatrix<2,T> &M,
                      const Point2<T> &p)
  {
    Point2<T> result(M(0,0)*p.x() + M(0,1)*p.y(),
                     M(1,0)*p.x() + M(1,1)*p.y());
    
    return result;
  }
  
  // Common 3D matrix-point multiplication:
  template<typename T>
  Point3<T> operator*(const SmallSquareMatrix<3,T> &M,
                      const Point3<T> &p)
  {
    Point3<T> result(M(0,0)*p.x() + M(0,1)*p.y() + M(0,2)*p.z(),
                     M(1,0)*p.x() + M(1,1)*p.y() + M(1,2)*p.z(),
                     M(2,0)*p.x() + M(2,1)*p.y() + M(2,2)*p.z());
    
    return result;
  }
  
  // Generic ND matrix-point multiplication
  template<size_t DIM, typename T>
  Point<DIM,T> operator*(const SmallSquareMatrix<DIM,T> &M,
                         const Point<DIM,T> &p)
  {
    Point<DIM,T> result;
    for(unsigned int i=0; i<DIM; ++i) {
      result[i] = M(i,0) * p[0];
      for(unsigned int j=1; j<DIM; ++j) {
        result[i] += M(i,j)*p[j];
      }
    }
    
    return result;
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
    CORETECH_THROW_IF(nrows == 0 || ncols == 0);
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, const T &initVal)
  : Array2d<T>(nrows, ncols, initVal)
  {
    CORETECH_THROW_IF(nrows == 0 || ncols == 0);
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, T *data)
  : Array2d<T>(nrows, ncols, data)
  {
    CORETECH_THROW_IF(nrows == 0 || ncols == 0 || data==NULL);
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, std::vector<T> &data)
  : Array2d<T>(nrows, ncols, data)
  {
    CORETECH_THROW_IF(nrows == 0 || ncols == 0);
  }
  
  
#if ANKICORETECH_USE_OPENCV
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
    
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix multiplication:
    Matrix<T> result( this->get_CvMat_() * other.get_CvMat_() );
#else
    assert(false);
    // TODO: Implement our own matrix multiplication.
#endif
    
    return result;
    
  } // Matrix<T>::operator*()
  
  template<typename T>
  Matrix<T>& Matrix<T>::operator*=(const Matrix<T> &other)
  {
    // Make sure the matrices have compatible sizes for multiplication
    CORETECH_THROW_IF(this->numCols() != other.numRows());
    
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix multiplication:
    *this = this->get_CvMat_() * other.get_CvMat_();
#else
    assert(false);
    // TODO: Implement our own matrix multiplication.
#endif
    
    return *this;
    
  } // Matrix<T>::operator*=()
  
  
  template<typename T>
  void Matrix<T>::Invert(void)
  {
    CORETECH_THROW_IF(this->numRows() != this->numCols());
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix inversion:
    *this = (Matrix<T>)this->get_CvMat_().inv();
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
  } // Matrix<T>::Invert()
  
  template<typename T>
  Matrix<T> Matrix<T>::getInverse(void) const
  {
    CORETECH_THROW_IF(this->numRows() != this->numCols());
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix inversion:
    Matrix<T> result(this->get_CvMat_().inv());
#else
    assert(false);
    // TODO: Define our own opencv-free transpose?
#endif
    
    return result;
  } // Matrix<T>::getInverse()
  
                   
  
  template<typename T>
  Matrix<T> Matrix<T>::getTranpose(void) const
  {
    
#if ANKICORETECH_USE_OPENCV
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
#if ANKICORETECH_USE_OPENCV
    cv::transpose(this->get_CvMat_(), this->get_CvMat_());
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  } // Matrix<T>::Tranpose()
  
  
  template<typename T>
  std::ostream& operator<<(std::ostream& out, const Matrix<T>& m)
  {
    for (int i=0; i<m.numRows(); ++i) {
      for (int j=0; j<m.numCols(); ++j) {
        out << m(i,j) << " ";
      }
      out << "\n";
    }
    return out;
  }
  
  
#pragma mark --- SmallMatrix Implementations ---
  
  
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix()
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>()
#endif
  {
#if (!defined(ANKICORETECH_USE_OPENCV))
    assert(false);
    // TODO: Define our own opencv-free constructor?
#endif
  }
  
  
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const T* vals)
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>(vals)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free constructor?
#endif
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const SmallMatrix<NROWS,NCOLS,T> &M)
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>(M)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free copy constructor?
#endif
  }

  
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix)
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>(cvMatrix)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free constructor?
#endif
  }

  // Matrix element access:
  template<size_t NROWS, size_t NCOLS, typename T>
  T&  SmallMatrix<NROWS,NCOLS,T>::operator() (unsigned int i, unsigned int j)
  {
    CORETECH_THROW_IF(i >= NROWS || j >= NCOLS);
      
    return cv::Matx<T,NROWS,NCOLS>::operator()(i,j);
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  const T& SmallMatrix<NROWS,NCOLS,T>::operator() (unsigned int i, unsigned int j) const
  {
    CORETECH_THROW_IF(i >= NROWS || j >= NCOLS);
    
    return cv::Matx<T,NROWS,NCOLS>::operator()(i,j);
  }
  
  // Matrix multiplication:
  // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
  template<size_t NROWS, size_t NCOLS, typename T>
  template<size_t KCOLS>
  SmallMatrix<NROWS,KCOLS,T> SmallMatrix<NROWS,NCOLS,T>::operator* (const SmallMatrix<NCOLS,KCOLS,T> &other) const
  {
#if ANKICORETECH_USE_OPENCV
    SmallMatrix<NROWS,KCOLS,T> res;
    res = this->get_CvMatx_() * other.get_CvMatx_();
    return res;
#else
    assert(false);
    // TODO: Define our own opencv-free multiplication?
#endif
  }
  
  
  
  // Matrix transpose:
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NCOLS,NROWS,T> SmallMatrix<NROWS,NCOLS,T>::getTranspose(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->t();
#else
    assert(false);
    // TODO: Define our own opencv-free tranpose?
#endif
  }
  
  
  template<size_t NROWS, size_t NCOLS, typename T>
  unsigned int SmallMatrix<NROWS,NCOLS,T>::numRows() const
  {
    return NROWS;
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  unsigned int SmallMatrix<NROWS,NCOLS,T>::numCols() const
  {
    return NCOLS;
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  bool operator==(const SmallMatrix<NROWS,NCOLS,T> &M1,
                  const SmallMatrix<NROWS,NCOLS,T> &M2)
  {
    for(unsigned int i=0; i<NROWS; ++i) {
      for(unsigned int j=0; j<NCOLS; ++j) {
        if(M1(i,j) != M2(i,j)) {
          return false;
        }
      }
    }
    
    return true;
  } // operator==(SmallMatrix,SmallMatrix)
  
  
  template<size_t NROWS, size_t NCOLS, typename T>
  bool nearlyEqual(const SmallMatrix<NROWS,NCOLS,T> &M1,
                   const SmallMatrix<NROWS,NCOLS,T> &M2,
                   const T eps)
  {
    for(unsigned int i=0; i<NROWS; ++i) {
      for(unsigned int j=0; j<NCOLS; ++j) {
        if(not NEAR(M1(i,j), M2(i,j), eps)) {
          return false;
        }
      }
    }
    
    return true;
  } // nearlyEqual(SmallMatrix,SmallMatrix)
  
  
#if ANKICORETECH_USE_OPENCV
  template<size_t NROWS, size_t NCOLS, typename T>
  cv::Matx<T,NROWS,NCOLS>& SmallMatrix<NROWS,NCOLS,T>::get_CvMatx_()
  {
    return (*this);
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  const cv::Matx<T,NROWS,NCOLS>& SmallMatrix<NROWS,NCOLS,T>::get_CvMatx_() const
  {
    return (*this);
  }
  
#endif
  
  template<size_t NROWS, size_t NCOLS, typename T>
  std::ostream& operator<<(std::ostream& out, const SmallMatrix<NROWS,NCOLS,T>& m)
  {
    for (int i=0; i<NROWS; ++i) {
      for (int j=0; j<NCOLS; ++j) {
        out << m(i,j) << " ";
      }
      out << "\n";
    }
    return out;
  }
  
  // Matrix inversion:
  template<size_t NROWS, size_t NCOLS, typename T>
  void SmallMatrix<NROWS,NCOLS,T>::Invert(void)
  {
#if ANKICORETECH_USE_OPENCV
    (*this) = this->inv();
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
    
  }
  
  template<size_t NROWS, size_t NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T> SmallMatrix<NROWS,NCOLS,T>::getInverse(void) const
  {
#if ANKICORETECH_USE_OPENCV
    SmallMatrix<NROWS,NCOLS,T> res(this->inv());
    return res;
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
#endif
    
  }
  
  
  
#pragma mark --- SmallSquareMatrix Implementations
  
  template<size_t DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(void)
  : SmallMatrix<DIM,DIM,T>()
  {
    
  }

  template<size_t DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(const SmallMatrix<DIM,DIM,T> &M)
  : SmallMatrix<DIM,DIM,T>(M)
  {
    
  }
  
  template<size_t DIM, typename T>
  void SmallSquareMatrix<DIM,T>::operator*=(const SmallSquareMatrix<DIM,T> &other)
  {
#if ANKICORETECH_USE_OPENCV
    (*this) = (*this) * other;
#else
    assert(false);
    // TODO: Define our own opencv-free in-place multiplication?
#endif
    
  }

  template<size_t DIM, typename T>
  void SmallSquareMatrix<DIM,T>::preMultiplyBy(const SmallSquareMatrix<DIM,T> &other)
  {
    // TODO: Come up with our own in-place, super-awesome pre-multiplcation
    // method (since opencv doesn't give us one, and this causes a copy)
    SmallSquareMatrix<DIM,T> otherCopy(other);
    otherCopy *= (*this);
    *this = otherCopy;
  }
  
  template<size_t DIM, typename T>
  SmallSquareMatrix<DIM,T>&  SmallSquareMatrix<DIM,T>::Transpose(void)
  {
    for(unsigned int i=0; i<DIM; ++i) {
      for(unsigned int j=(i+1); j<DIM; ++j) {
        std::swap((*this)(i,j), (*this)(j,i));
      }
    }
    
    return *this;
  } // SmallSquareMatrix::Tranpose()
   
  // Matrix inversion:
  template<size_t DIM, typename T>
  void SmallSquareMatrix<DIM,T>::Invert(void)
  {
    this->SmallMatrix<DIM,DIM,T>::Invert();
  }
  
  template<size_t DIM, typename T>
  SmallSquareMatrix<DIM,T> SmallSquareMatrix<DIM,T>::getInverse(void) const
  {
    return this->SmallMatrix<DIM,DIM,T>::getInverse();
  }
  
  
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
