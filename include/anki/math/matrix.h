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

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/core/core.hpp"
#endif

#include <ostream>
#include <cstdio>
#include "anki/common/array2d.h"

#include "anki/math/point.h"

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
    
#if defined(ANKICORETECH_USE_OPENCV)
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
    SmallMatrix(const SmallMatrix<T,NROWS,NCOLS> &M);
    
    // Matrix element access:
    T&       operator() (unsigned int i, unsigned int j);
    const T& operator() (unsigned int i, unsigned int j) const;
    
    // Matrix multiplication:
    // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
    template<unsigned int KCOLS>
    SmallMatrix<T,NROWS,KCOLS> operator* (const SmallMatrix<T,NCOLS,KCOLS> &other) const;
    
    // Matrix transpose:
    SmallMatrix<T,NCOLS,NROWS> getTranspose(void) const;
    
    // TODO: Add explicit pseudo-inverse? (inv(A'A)*A')
    // SmallMatrix<T,NCOLS,NROWS> getPsuedoInverse(void) const;
    
    unsigned int numRows() const;
    unsigned int numCols() const;
    
#if defined(ANKICORETECH_USE_OPENCV)
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
    SmallMatrix<T,NROWS,NCOLS> getInverse(void) const;

  }; // class SmallMatrix
  
  // Comparisons for equalit and near-equality:
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  bool operator==(const SmallMatrix<T,NROWS,NCOLS> &M1,
                  const SmallMatrix<T,NROWS,NCOLS> &M2);
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  bool nearlyEqual(const SmallMatrix<T,NROWS,NCOLS> &M1,
                   const SmallMatrix<T,NROWS,NCOLS> &M2,
                   const T eps = std::numeric_limits<T>::epsilon());
  
  // An extension of the SmallMatrix class for square matrices
  template<typename T, unsigned int DIM>
  class SmallSquareMatrix : public SmallMatrix<T,DIM,DIM>
  {
  public:
    
    SmallSquareMatrix();
    SmallSquareMatrix(const SmallMatrix<T,DIM,DIM> &M);
    
    using SmallMatrix<T,DIM,DIM>::operator();
    
    // Matrix multiplication in place...
    // ... this = this * other;
    void operator*=(const SmallSquareMatrix<T,DIM> &other);
    // ... this = other * this;
    void preMultiplyBy(const SmallSquareMatrix<T,DIM> &other);
    
    friend Point<T,DIM> operator*(const SmallSquareMatrix<T,DIM> &M,
                                  const Point<T,DIM> &p);
    
    // Transpose: (Note that we can transpose square matrices in place)
    using SmallMatrix<T,DIM,DIM>::getTranspose;
    SmallSquareMatrix<T,DIM>& Transpose(void);
    
    // Matrix inversion:
    void Invert(void); // in place
    SmallSquareMatrix<T,DIM> getInverse(void) const;
    
  }; // class SmallSquareMatrix
  
    
  // Typedef some common small matrix sizes, like 3x3
  typedef SmallSquareMatrix<float,2> Matrix_2x2f;
  typedef SmallSquareMatrix<float,3> Matrix_3x3f;
  typedef SmallMatrix<float,3,4> Matrix_3x4f;
  
  // Common 2D matrix-point multiplication:
  template<typename T>
  Point2<T> operator*(const SmallSquareMatrix<T,2> &M,
                      const Point2<T> &p)
  {
    Point2<T> result(M(0,0)*p.x() + M(0,1)*p.y(),
                     M(1,0)*p.x() + M(1,1)*p.y());
    
    return result;
  }
  
  // Common 3D matrix-point multiplication:
  template<typename T>
  Point3<T> operator*(const SmallSquareMatrix<T,3> &M,
                      const Point3<T> &p)
  {
    Point3<T> result(M(0,0)*p.x() + M(0,1)*p.y() + M(0,2)*p.z(),
                     M(1,0)*p.x() + M(1,1)*p.y() + M(1,2)*p.z(),
                     M(2,0)*p.x() + M(2,1)*p.y() + M(2,2)*p.z());
    
    return result;
  }
  
  // Generic ND matrix-point multiplication
  template<typename T, unsigned int DIM>
  Point<T,DIM> operator*(const SmallSquareMatrix<T,DIM> &M,
                         const Point<T,DIM> &p)
  {
    Point<T,DIM> result;
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
  Matrix<T>& Matrix<T>::operator*=(const Matrix<T> &other)
  {
    // Make sure the matrices have compatible sizes for multiplication
    CORETECH_THROW_IF(this->numCols() != other.numRows());
    
    
#if defined(ANKICORETECH_USE_OPENCV)
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
    
#if defined(ANKICORETECH_USE_OPENCV)
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
    
#if defined(ANKICORETECH_USE_OPENCV)
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
    
#if defined(ANKICORETECH_USE_OPENCV)
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
  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS>::SmallMatrix()
#if defined(ANKICORETECH_USE_OPENCV)
  : cv::Matx<T,NROWS,NCOLS>()
#endif
  {
#if (!defined(ANKICORETECH_USE_OPENCV))
    assert(false);
    // TODO: Define our own opencv-free constructor?
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
    // TODO: Define our own opencv-free constructor?
#endif
  }
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  SmallMatrix<T,NROWS,NCOLS>::SmallMatrix(const SmallMatrix<T,NROWS,NCOLS> &M)
#if defined(ANKICORETECH_USE_OPENCV)
  : cv::Matx<T,NROWS,NCOLS>(M)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    assert(false);
    // TODO: Define our own opencv-free copy constructor?
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
    // TODO: Define our own opencv-free constructor?
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
    res = this->get_CvMatx_() * other.get_CvMatx_();
    return res;
#else
    assert(false);
    // TODO: Define our own opencv-free multiplication?
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
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  bool operator==(const SmallMatrix<T,NROWS,NCOLS> &M1,
                  const SmallMatrix<T,NROWS,NCOLS> &M2)
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
  
  
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  bool nearlyEqual(const SmallMatrix<T,NROWS,NCOLS> &M1,
                   const SmallMatrix<T,NROWS,NCOLS> &M2,
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
  std::ostream& operator<<(std::ostream& out, const SmallMatrix<T, NROWS, NCOLS>& m)
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
  template<typename T, unsigned int NROWS, unsigned int NCOLS>
  void SmallMatrix<T,NROWS,NCOLS>::Invert(void)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    (*this) = this->inv();
#else
    assert(false);
    // TODO: Define our own opencv-free inverse?
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
    // TODO: Define our own opencv-free inverse?
#endif
    
  }
  
  
  
#pragma mark --- SmallSquareMatrix Implementations
  
  template<typename T, unsigned int DIM>
  SmallSquareMatrix<T,DIM>::SmallSquareMatrix(void)
  : SmallMatrix<T,DIM,DIM>()
  {
    
  }

  template<typename T, unsigned int DIM>
  SmallSquareMatrix<T,DIM>::SmallSquareMatrix(const SmallMatrix<T,DIM,DIM> &M)
  : SmallMatrix<T,DIM,DIM>(M)
  {
    
  }
  
  template<typename T, unsigned int DIM>
  void SmallSquareMatrix<T,DIM>::operator*=(const SmallSquareMatrix<T,DIM> &other)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    (*this) *= other;
#else
    assert(false);
    // TODO: Define our own opencv-free in-place multiplication?
#endif
    
  }

  template<typename T, unsigned int DIM>
  void SmallSquareMatrix<T,DIM>::preMultiplyBy(const SmallSquareMatrix<T,DIM> &other)
  {
    // TODO: Come up with our own in-place, super-awesome pre-multiplcation
    // method (since opencv doesn't give us one, and this causes a copy)
    (*this) = other * (*this);    
  }
  
  template<typename T, unsigned int DIM>
  SmallSquareMatrix<T,DIM>&  SmallSquareMatrix<T,DIM>::Transpose(void)
  {
    for(unsigned int i=0; i<DIM; ++i) {
      for(unsigned int j=(i+1); j<DIM; ++j) {
        std::swap((*this)(i,j), (*this)(j,i));
      }
    }
    
    return *this;
  } // SmallSquareMatrix::Tranpose()
   
  // Matrix inversion:
  template<typename T, unsigned int DIM>
  void SmallSquareMatrix<T,DIM>::Invert(void)
  {
    this->SmallMatrix<T,DIM,DIM>::Invert();
  }
  
  template<typename T, unsigned int DIM>
  SmallSquareMatrix<T,DIM> SmallSquareMatrix<T,DIM>::getInverse(void) const
  {
    return this->SmallMatrix<T,DIM,DIM>::getInverse();
  }
  
  
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
