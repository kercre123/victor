/**
 * File: matrix_impl.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
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

#ifndef _ANKICORETECH_MATRIX_IMPL_H_
#define _ANKICORETECH_MATRIX_IMPL_H_

#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/point.h"
#include "coretech/common/shared/array2d_impl.h"

#include <ostream>
#include <cstdio>

namespace Anki {
  
#if 0
#pragma mark ---- Matrix-Point Multiplication ----
#endif
  
  // Generic ND matrix-point multiplication
  // Working (and output) precision (type) is Twork
  template<MatDimType NROWS, MatDimType NCOLS, typename Tmat, typename Tpt, typename Twork>
  Point<NROWS,Twork> operator*(const SmallMatrix<NROWS,NCOLS,Tmat> &M,
                              const Point<NCOLS,Tpt> &p)
  {
    Point<NROWS,Twork> result;
    for(MatDimType i=0; i<NROWS; ++i) {
      result[i] = static_cast<Twork>(M(i,0)) * static_cast<Twork>(p[0]);
      for(MatDimType j=1; j<NCOLS; ++j) {
        result[i] += static_cast<Twork>(M(i,j)) * static_cast<Twork>(p[j]);
      }
    }
    
    return result;
  }
  
  
  
  // Common 2D matrix-point multiplication:
  template<typename Tmat, typename Tpt, typename Twork>
  Point2<Twork> operator*(const SmallSquareMatrix<2,Tmat> &M,
                          const Point2<Tpt> &p)
  {
    const Twork px = static_cast<Twork>(p.x());
    const Twork py = static_cast<Twork>(p.y());
    
    Point2<Twork> result(static_cast<Twork>(M(0,0))*px + static_cast<Twork>(M(0,1))*py,
                         static_cast<Twork>(M(1,0))*px + static_cast<Twork>(M(1,1))*py);
    
    return result;
  }
  
  // Common 3D matrix-point multiplication:
  template<typename Tmat, typename Tpt, typename Twork>
  Point3<Twork> operator*(const SmallSquareMatrix<3,Tmat> &M,
                          const Point3<Tpt> &p)
  {
    const Twork px = static_cast<Twork>(p.x());
    const Twork py = static_cast<Twork>(p.y());
    const Twork pz = static_cast<Twork>(p.z());
    
    Point3<Twork> result(static_cast<Twork>(M(0,0))*px + static_cast<Twork>(M(0,1))*py + static_cast<Twork>(M(0,2))*pz,
                         static_cast<Twork>(M(1,0))*px + static_cast<Twork>(M(1,1))*py + static_cast<Twork>(M(1,2))*pz,
                         static_cast<Twork>(M(2,0))*px + static_cast<Twork>(M(2,1))*py + static_cast<Twork>(M(2,2))*pz);
    
    return result;
  }
  

  
#if 0
#pragma mark --- Matrix Implementations ---
#endif 
  
  template<typename T>
  Matrix<T>::Matrix()
  : Array2d<T>()
  {
    
  }
  
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols)
  : Array2d<T>(nrows, ncols)
  {
    DEV_ASSERT(nrows > 0 && ncols > 0, "Matrix.Constructor.InitializedRequiresPositiveDimensions");
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, const T &initVal)
  : Array2d<T>(nrows, ncols, initVal)
  {
    DEV_ASSERT(nrows > 0 && ncols > 0, "Matrix.Constructor.InitializedRequiresPositiveDimensions");
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, T *data)
  : Array2d<T>(nrows, ncols, data)
  {
    DEV_ASSERT(nrows > 0 && ncols > 0 && data!=NULL, "Matrix.Constructor.InitializedRequiresPositiveDimensions");
  }
  
  template<typename T>
  Matrix<T>::Matrix(s32 nrows, s32 ncols, std::vector<T> &data)
  : Array2d<T>(nrows, ncols, data)
  {
    DEV_ASSERT(nrows > 0 && ncols > 0, "Matrix.Constructor.InitializedRequiresPositiveDimensions");
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
    DEV_ASSERT(this->GetNumCols() == other.GetNumRows(), "Matrix.Multiply.DimensionMismatch");
    
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix multiplication:
    Matrix<T> result( this->get_CvMat_() * other.get_CvMat_() );
#else
    DEV_ASSERT(false, "Matrix.Multiply.RequiresOpenCV");
    Matrix<T> result;
    // TODO: Implement our own matrix multiplication.
#endif
    
    return result;
    
  } // Matrix<T>::operator*()
  
  template<typename T>
  Matrix<T>& Matrix<T>::operator*=(const Matrix<T> &other)
  {
    // Make sure the matrices have compatible sizes for multiplication
    DEV_ASSERT(this->GetNumCols() == other.GetNumRows(), "Matrix.MultiplyInPlace.DimensionMismatch");

    
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix multiplication:
    *this = this->get_CvMat_() * other.get_CvMat_();
#else
    DEV_ASSERT(false, "Matrix.MultiplyInPlace.RequiresOpenCV");
    // TODO: Implement our own matrix multiplication.
#endif
    
    return *this;
    
  } // Matrix<T>::operator*=()
  
  
  template<typename T>
  Matrix<T>& Matrix<T>::Invert(void)
  {
    DEV_ASSERT(this->GetNumRows() == this->GetNumCols(), "Matrix.Invert.NonSquareMatrix");
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix inversion:
    *this = (Matrix<T>)this->get_CvMat_().inv();
#else
    DEV_ASSERT(false, "Matrix.Invert.RequiresOpenCV");
    // TODO: Define our own opencv-free inverse?
#endif
    
    return *this;
  } // Matrix<T>::Invert()
  
  template<typename T>
  void Matrix<T>::GetInverse(Matrix<T>& outInverse) const
  {
    DEV_ASSERT(this->GetNumRows() == this->GetNumCols(), "Matrix.GetInverse.NonSquareMatrix");
    
#if ANKICORETECH_USE_OPENCV
    // For now (?), rely on OpenCV for matrix inversion:
    cv::invert(this->get_CvMat_(), outInverse.get_CvMat_());
#else
    DEV_ASSERT(false, "Matrix.GetInverse.RequiresOpenCV");
    // TODO: Define our own opencv-free transpose?
#endif
    
  } // Matrix<T>::GetInverse()
  
                   
  
  template<typename T>
  void Matrix<T>::GetTranspose(Matrix<T>& outTranspose) const
  {
    
#if ANKICORETECH_USE_OPENCV
    cv::transpose(this->get_CvMat_(), outTranspose.get_CvMat_());
#else
    DEV_ASSERT(false, "Matrix.GetTranspose.RequiresOpenCV");
    // TODO: Define our own opencv-free tranpose?
#endif
    
  } // Matrix<T>::getTranspose()
  
  
  template<typename T>
  Matrix<T>& Matrix<T>::Transpose(void)
  {
#if ANKICORETECH_USE_OPENCV
    cv::transpose(this->get_CvMat_(), this->get_CvMat_());
#else
    DEV_ASSERT(false, "Matrix.Transpose.RequiresOpenCV");
    // TODO: Define our own opencv-free tranpose?
#endif
    
    return *this;
  } // Matrix<T>::Tranpose()
  
  
  template<typename T>
  std::ostream& operator<<(std::ostream& out, const Matrix<T>& m)
  {
    for (int i=0; i<m.GetNumRows(); ++i) {
      for (int j=0; j<m.GetNumCols(); ++j) {
        out << m(i,j) << " ";
      }
      out << "\n";
    }
    return out;
  }
  
  
#pragma mark --- SmallMatrix Implementations ---
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix()
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>()
#endif
  {
#if (!defined(ANKICORETECH_USE_OPENCV))
    DEV_ASSERT(false, "SmallMatrix.Constructor.RequiresOpenCV");
    // TODO: Define our own opencv-free constructor?
#endif
  }
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const T* vals)
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>(vals)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    DEV_ASSERT(false, "SmallMatrix.Constructor.RequiresOpenCV");
    // TODO: Define our own opencv-free constructor?
#endif
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(std::initializer_list<T> valsList)
  {
    DEV_ASSERT(valsList.size() == NROWS*NCOLS, "SmallMatrix.Constructor.InitializerListDimensionMismatch");
    
    T vals[NROWS*NCOLS];
    MatDimType i=0;
    for(auto listItem = valsList.begin(); i<NROWS*NCOLS; ++listItem, ++i ) {
      vals[i] = *listItem;
    }
    *this = SmallMatrix<NROWS,NCOLS,T>(vals);
  }
 
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(std::initializer_list<Point<NROWS,T> > colsList)
  {
    DEV_ASSERT(colsList.size() == NCOLS, "SmallMatrix.Constructor.InitializerListDimensionMismatch");
    
    MatDimType j=0;
    for(auto col = colsList.begin(); j<NCOLS; ++col, ++j)
    {
      for(MatDimType i=0; i<NROWS; ++i) {
        this->operator()(i,j) = (*col)[i];
      }
    }
  }
  
  
  /*
  template<DimType NROWS, DimType NCOLS>
  SmallMatrix<NROWS,NCOLS,float>::SmallMatrix(const SmallMatrix<NROWS,NCOLS,float> &M)
  {
    // TODO: Is there a better way to do this than looping and casting each element?
    for(size_t i=0; i<NROWS; ++i) {
      for(size_t j=0; j<NCOLS; ++j) {
        this->operator()(i,j) = static_cast<float>(M(i,j));
      }
    }
  }
   */
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const SmallMatrix<NROWS,NCOLS,T> &M)
#if ANKICORETECH_USE_OPENCV
  : cv::Matx<T,NROWS,NCOLS>(M)
#endif
  {
#if !defined(ANKICORETECH_USE_OPENCV)
    DEV_ASSERT(false, "SmallMatrix.Constructor.RequiresOpenCV");
    // TODO: Define our own opencv-free copy constructor?
#endif
  }
  
  
#if ANKICORETECH_USE_OPENCV
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>::SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix)
  : cv::Matx<T,NROWS,NCOLS>(cvMatrix)
  {
//#if !defined(ANKICORETECH_USE_OPENCV)
//    CORETECH_ASSERT(false);
//    // TODO: Define our own opencv-free constructor?
//#endif
  }
#endif

  // Matrix element access:
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  T&  SmallMatrix<NROWS,NCOLS,T>::operator() (const MatDimType i, const MatDimType j)
  {
    DEV_ASSERT(i < NROWS && j < NCOLS, "SmallMatrix.ElementAccess.OutOfBounds");
#if ANKICORETECH_USE_OPENCV
    return cv::Matx<T,NROWS,NCOLS>::operator()((int) i, (int) j);
#else
    DEV_ASSERT(false, "SmallMatrix.ElementAccess.RequiresOpenCV");
    // TODO: Define our own opencv-free (i,j) accessor?
    static T t(0);
    return t;
#endif
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  const T& SmallMatrix<NROWS,NCOLS,T>::operator() (const MatDimType i, const MatDimType j) const
  {
    DEV_ASSERT(i < NROWS && j < NCOLS, "SmallMatrix.ElementAccess.OutOfBounds");
#if ANKICORETECH_USE_OPENCV
    return cv::Matx<T,NROWS,NCOLS>::operator()((int) i, (int)j);
#else
    DEV_ASSERT(false, "SmallMatrix.ElementAccess.RequiresOpenCV");
    // TODO: Define our own opencv-free const (i,j) accessor?
    static T t(0);
    return t;
#endif
  }

  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  Point<NCOLS,T> SmallMatrix<NROWS,NCOLS,T>::GetRow(const MatDimType i) const
  {
    DEV_ASSERT(i < NROWS, "SmallMatrix.GetRow.OutOfBounds");
    
    Point<NCOLS,T> row;
    for(MatDimType j=0; j<NCOLS; ++j) {
      row[j] = this->operator()(i,j);
    }
    
    return row;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  Point<NROWS,T> SmallMatrix<NROWS,NCOLS,T>::GetColumn(const MatDimType j) const
  {
    DEV_ASSERT(j < NCOLS, "SmallMatrix.GetColumn.OutOfBounds");
    
    Point<NROWS,T> column;
    for(MatDimType i=0; i<NROWS; ++i) {
      column[i] = this->operator()(i,j);
    }
    
    return column;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  void SmallMatrix<NROWS,NCOLS,T>::SetRow(const MatDimType i, const Point<NCOLS,T>& row)
  {
    DEV_ASSERT(i < NROWS, "SmallMatrix.SetRow.OutOfBounds");

    for(MatDimType j=0; j<NCOLS; ++j) {
      this->operator()(i,j) = row[j];
    }
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  void SmallMatrix<NROWS,NCOLS,T>::SetColumn(const MatDimType j, const Point<NROWS,T>& column)
  {
    DEV_ASSERT(j < NCOLS, "SmallMatrix.SetColumn.OutOfBounds");
    
    for(MatDimType i=0; i<NROWS; ++i) {
      this->operator()(i,j) = column[i];
    }
  }

  
  /*
  template<DimType NROWS, DimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::operator=(const SmallMatrix& other)
  {
    if(this != &other) {
      *this = SmallMatrix(other);
    }
    return *this;
  }
   */

  // Matrix multiplication:
  // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  template<MatDimType KCOLS, typename T_other, typename T_work>
  SmallMatrix<NROWS,KCOLS,T_work> SmallMatrix<NROWS,NCOLS,T>::operator* (const SmallMatrix<NCOLS,KCOLS,T_other> &other) const
  {
    SmallMatrix<NROWS,KCOLS,T_work> res;
#if ANKICORETECH_USE_OPENCV
    res = this->get_CvMatx_() * other.get_CvMatx_();
#else
    // TODO: This could probably be made more efficient by avoiding so many operator(i,j) calls
    for(MatDimType iThis=0; iThis<NROWS; ++iThis) {
      for(MatDimType jOther=0; jOther<KCOLS; ++jOther) {
        res(iThis,jOther) = T(0);
        for(MatDimType jThis=0; jThis<NCOLS; ++jThis) {
          res(iThis,jOther) += static_cast<T>(this->operator()(iThis,jThis)*other(jThis,jOther));
        }
      }
    }
#endif
    return res;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>  SmallMatrix<NROWS,NCOLS,T>::operator* (T value) const
  {
    SmallMatrix<NROWS,NCOLS,T> result(*this);
    result *= value;
    return result;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::operator*=(T value)
  {
    for(MatDimType i=0; i<NROWS*NCOLS; ++i) {
      this->val[i] *= value;
    }
    return *this;
  }
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>  SmallMatrix<NROWS,NCOLS,T>::operator+ (T value) const
  {
    SmallMatrix<NROWS,NCOLS,T> result(*this);
    result += value;
    return result;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::operator+=(T value)
  {
    for(MatDimType i=0; i<NROWS*NCOLS; ++i) {
      this->val[i] += value;
    }
    return *this;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T> SmallMatrix<NROWS,NCOLS,T>::operator+ (const SmallMatrix<NROWS,NCOLS,T>& other) const
  {
    SmallMatrix<NROWS,NCOLS,T> result(*this);
    result += other;
    return result;
  }  

  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::operator+=(const SmallMatrix<NROWS,NCOLS,T>& other)
  {
    for(MatDimType i=0; i<NROWS*NCOLS; ++i) {
      this->val[i] += other.val[i];
    }
    return *this;
  }
    
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T> SmallMatrix<NROWS,NCOLS,T>::operator- (const SmallMatrix<NROWS,NCOLS,T>& other) const
  {
    SmallMatrix<NROWS,NCOLS,T> result(*this);
    result -= other;
    return result;
  }  

  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::operator-=(const SmallMatrix<NROWS,NCOLS,T>& other)
  {
    for(MatDimType i=0; i<NROWS*NCOLS; ++i) {
      this->val[i] -= other.val[i];
    }
    return *this;
  }
  
  // Matrix transpose:
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NCOLS,NROWS,T> SmallMatrix<NROWS,NCOLS,T>::GetTranspose() const
  {
    SmallMatrix<NCOLS,NROWS,T> retv;
#if ANKICORETECH_USE_OPENCV
    retv = this->t();
#else
    DEV_ASSERT(false, "SmallMatrix.GetTranspose.RequiresOpenCV");
    // TODO: Define our own opencv-free tranpose?
#endif
    return retv;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::Abs()
  {
    for(MatDimType i=0; i<NROWS; ++i) {
      for(MatDimType j=0; j<NCOLS; ++j) {
        (*this)(i,j) = std::abs((*this)(i,j));
      }
    }
    return *this;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  MatDimType SmallMatrix<NROWS,NCOLS,T>::GetNumRows() const
  {
    return NROWS;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  MatDimType SmallMatrix<NROWS,NCOLS,T>::GetNumCols() const
  {
    return NCOLS;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
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
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  bool IsNearlyEqual(const SmallMatrix<NROWS,NCOLS,T> &M1,
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
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T> Abs(const SmallMatrix<NROWS,NCOLS,T>& M)
  {
    SmallMatrix<NROWS,NCOLS,T> Mcopy(M);
    return Mcopy.abs();
  } // abs()
  
#if ANKICORETECH_USE_OPENCV
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  cv::Matx<T,NROWS,NCOLS>& SmallMatrix<NROWS,NCOLS,T>::get_CvMatx_()
  {
    return (*this);
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  const cv::Matx<T,NROWS,NCOLS>& SmallMatrix<NROWS,NCOLS,T>::get_CvMatx_() const
  {
    return (*this);
  }
  
#endif
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
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
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T>& SmallMatrix<NROWS,NCOLS,T>::Invert(void)
  {
#if ANKICORETECH_USE_OPENCV
    (*this) = this->inv();
#else
    DEV_ASSERT(false, "SmallMatrix.Invert.RequiresOpenCV");
    // TODO: Define our own opencv-free inverse?
#endif
    
    return *this;
  }
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  void SmallMatrix<NROWS,NCOLS,T>::GetInverse(SmallMatrix<NROWS,NCOLS,T>& outInverse) const
  {
#if ANKICORETECH_USE_OPENCV
    outInverse = *this;
    outInverse.Invert();
#else
    DEV_ASSERT(false, "SmallMatrix.GetInverse.RequiresOpenCV");
    // TODO: Define our own opencv-free inverse?
#endif
  }
  
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  void SmallMatrix<NROWS,NCOLS,T>::FillWith(T value)
  {
    for(MatDimType i=0; i<NROWS*NCOLS; ++i) {
      this->val[i] = value;
    }
  }
  
  
  
#pragma mark --- SmallSquareMatrix Implementations
  
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(void)
  : SmallMatrix<DIM,DIM,T>()
  {
    
  }

  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(const SmallMatrix<DIM,DIM,T> &M)
  : SmallMatrix<DIM,DIM,T>(M)
  {
    
  }
  
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(std::initializer_list<T> valsList)
  : SmallMatrix<DIM,DIM,T>(valsList)
  {
    
  }
  
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>::SmallSquareMatrix(std::initializer_list<Point<DIM,T> > colsList)
  : SmallMatrix<DIM,DIM,T>(colsList)
  {
    
  }
  
  template<MatDimType DIM, typename T>
  template<typename T_other>
  SmallSquareMatrix<DIM,T>& SmallSquareMatrix<DIM,T>::operator*=(const SmallSquareMatrix<DIM,T_other> &other)
  {
    (*this) = (*this) * other;
    return *this;
  }

  template<MatDimType DIM, typename T>
  template<typename T_other>
  SmallSquareMatrix<DIM,T>& SmallSquareMatrix<DIM,T>::PreMultiplyBy(const SmallSquareMatrix<DIM,T_other> &other)
  {
    // TODO: Come up with our own in-place, super-awesome pre-multiplcation
    // method (since opencv doesn't give us one, and this causes a copy)
    SmallSquareMatrix<DIM,T> otherCopy(other);
    otherCopy *= (*this);
    *this = otherCopy;
    return *this;
  }
  
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>&  SmallSquareMatrix<DIM,T>::Transpose(void)
  {
    for(MatDimType i=0; i<DIM; ++i) {
      for(MatDimType j=(i+1); j<DIM; ++j) {
        std::swap((*this)(i,j), (*this)(j,i));
      }
    }
    
    return *this;
  } // SmallSquareMatrix::Tranpose()
   
  // Matrix inversion:
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T>& SmallSquareMatrix<DIM,T>::Invert(void)
  {
    this->SmallMatrix<DIM,DIM,T>::Invert();
    return *this;
  }
  
  template<MatDimType DIM, typename T>
  SmallSquareMatrix<DIM,T> SmallSquareMatrix<DIM,T>::GetInverse() const
  {
    SmallSquareMatrix<DIM,T> retv = *this;
    return retv.Invert();
  }
  
  
  template<MatDimType DIM, typename T>
  T SmallSquareMatrix<DIM,T>::Trace() const
  {
    T trace = (*this)(0,0);
    for(MatDimType i=1; i<DIM; ++i) {
      trace += (*this)(i,i);
    }
    return trace;
  }
  

} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_IMPL_H_
