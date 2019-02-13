/**
 * File: matrix.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Defines a few classes for storing matrices.  All are
 *              on storage type.
 *
 // TODO: Move these classes to separate files
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

#include "coretech/common/shared/array2d.h"
#include "coretech/common/shared/math/point_fwd.h"

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
    Matrix<T>  operator* (const Matrix<T> &other) const;
    Matrix<T>& operator*=(const Matrix<T> &other);
    
    // Matrix inversion:
    // TODO: provide way to specify method
    Matrix<T>& Invert(void); // in place
    void       GetInverse(Matrix<T>& outInverse) const;
    
    // Matrix tranpose:
    void       GetTranspose(Matrix<T>& outTransposed) const;
    Matrix<T>& Transpose(void); // in place
    
    
  }; // class Matrix

  // A class for small matrices, whose size is known at compile time
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  class SmallMatrix
#if ANKICORETECH_USE_OPENCV
  : private cv::Matx<T,NROWS,NCOLS> // private inheritance from cv::Matx
#endif
  {
    static_assert(NROWS > 0 && NCOLS > 0, "SmallMatrix should not be empty.");
    
  public:
    // Constructors:
    SmallMatrix();
    SmallMatrix(const T* vals); // *assumes* vals is NROWS*NCOLS long
    
    explicit SmallMatrix(std::initializer_list<T> valsList);
    explicit SmallMatrix(std::initializer_list<Point<NROWS,T> > colsList);
    
    SmallMatrix(const SmallMatrix<NROWS,NCOLS,T> &M);
    
    //    template<typename T_other>
    //    SmallMatrix(const SmallMatrix<NROWS,NCOLS,T_other> &M);
    
    //SmallMatrix<NROWS,NCOLS,T>& operator=(const SmallMatrix& other);
    
    // Matrix element access:
    T&       operator() (MatDimType i, MatDimType j);
    const T& operator() (MatDimType i, MatDimType j) const;

    // Extract a single row or column as a point
    Point<NCOLS,T> GetRow(const MatDimType i) const;
    Point<NROWS,T> GetColumn(const MatDimType j) const;
    
    // Set a single row or column from a point
    void SetRow(const MatDimType i, const Point<NCOLS,T>& row);
    void SetColumn(const MatDimType j, const Point<NROWS,T>& col);
    
    // Matrix multiplication:
    // Matrix[MxN] * Matrix[NxK] = Matrix[MxK]
    template<MatDimType KCOLS, typename T_other, typename T_work=T>
    SmallMatrix<NROWS,KCOLS,T_work> operator* (const SmallMatrix<NCOLS,KCOLS,T_other> &other) const;
    
    SmallMatrix<NROWS,NCOLS,T>  operator* (T value) const;
    SmallMatrix<NROWS,NCOLS,T>& operator*=(T value);
    SmallMatrix<NROWS,NCOLS,T>  operator+ (T value) const;
    SmallMatrix<NROWS,NCOLS,T>& operator+=(T value);
    SmallMatrix<NROWS,NCOLS,T>  operator+ (const SmallMatrix<NROWS,NCOLS,T>& other) const;
    SmallMatrix<NROWS,NCOLS,T>& operator+=(const SmallMatrix<NROWS,NCOLS,T>& other);
    SmallMatrix<NROWS,NCOLS,T>  operator- (const SmallMatrix<NROWS,NCOLS,T>& other) const;
    SmallMatrix<NROWS,NCOLS,T>& operator-=(const SmallMatrix<NROWS,NCOLS,T>& other);
    
    // Matrix transpose:
    SmallMatrix<NCOLS,NROWS,T> GetTranspose() const;
    
    // Take absolute value of all elements (return reference to self)
    SmallMatrix<NROWS,NCOLS,T>& Abs();
    
    // TODO: Add explicit pseudo-inverse? (inv(A'A)*A')
    // SmallMatrix<T,NCOLS,NROWS> getPsuedoInverse(void) const;
    
    MatDimType GetNumRows() const;
    MatDimType GetNumCols() const;
    
    void FillWith(T value);

# if ANKICORETECH_USE_OPENCV
  public:
    SmallMatrix(const cv::Matx<T,NROWS,NCOLS> &cvMatrix);
    cv::Matx<T,NROWS,NCOLS>& get_CvMatx_();
    const cv::Matx<T,NROWS,NCOLS>& get_CvMatx_() const;
    
    using cv::Matx<T,NROWS,NCOLS>::operator=;
# endif
    
  protected:
    
    // Do we want to provide non-square matrix inversion?
    // I'm putting this here so the SmallSquareMatrix subclass can get
    // access to cv::Matx::inv() though.
    SmallMatrix<NROWS,NCOLS,T>& Invert(void); // in place
    void GetInverse(SmallMatrix<NROWS,NCOLS,T>& outInverse) const;

  }; // class SmallMatrix
  
  // Comparisons for equality and near-equality:
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  bool operator==(const SmallMatrix<NROWS,NCOLS,T> &M1,
                  const SmallMatrix<NROWS,NCOLS,T> &M2);
  
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  bool IsNearlyEqual(const SmallMatrix<NROWS,NCOLS,T> &M1,
                   const SmallMatrix<NROWS,NCOLS,T> &M2,
                   const T eps = std::numeric_limits<T>::epsilon());
  
  // Absolute value of all elements
  template<MatDimType NROWS, MatDimType NCOLS, typename T>
  SmallMatrix<NROWS,NCOLS,T> Abs(const SmallMatrix<NROWS,NCOLS,T>& M);
  
  // Generic ND matrix-point multiplication
  // Working (and output) precision (type) is Twork
  template<MatDimType NROWS, MatDimType NCOLS, typename Tmat, typename Tpt, typename Twork=Tpt>
  Point<NROWS,Twork> operator*(const SmallMatrix<NROWS,NCOLS,Tmat> &M,
                               const Point<NCOLS,Tpt> &p);
  
  
  // An extension of the SmallMatrix class for square matrices
  template<MatDimType DIM, typename T>
  class SmallSquareMatrix : public SmallMatrix<DIM,DIM,T>
  {
  public:
    SmallSquareMatrix();
    SmallSquareMatrix(const SmallMatrix<DIM,DIM,T> &M);
    explicit SmallSquareMatrix(std::initializer_list<T> valsList); // DIMxDIM list of values
    explicit SmallSquareMatrix(std::initializer_list<Point<DIM,T> > colsList); // list of columns
    
    using SmallMatrix<DIM,DIM,T>::operator();
    using SmallMatrix<DIM,DIM,T>::operator*;
    using SmallMatrix<DIM,DIM,T>::operator+;
    using SmallMatrix<DIM,DIM,T>::operator+=;
    using SmallMatrix<DIM,DIM,T>::operator-;
    using SmallMatrix<DIM,DIM,T>::operator-=;
    
    // Matrix multiplication in place...
    // ... this = this * other;
    template<typename T_other>
    SmallSquareMatrix<DIM,T>& operator*=(const SmallSquareMatrix<DIM,T_other> &other);
    // ... this = other * this;
    template<typename T_other>
    SmallSquareMatrix<DIM,T>& PreMultiplyBy(const SmallSquareMatrix<DIM,T_other> &other);
       
    // Transpose: (Note that we can transpose square matrices in place)
    using SmallMatrix<DIM,DIM,T>::GetTranspose;
    SmallSquareMatrix<DIM,T>& Transpose(void);
    
    // Matrix inversion:
    SmallSquareMatrix<DIM,T>& Invert(void); // in place
    SmallSquareMatrix<DIM,T> GetInverse() const;
    
    // Compute the trace (sum of diagonal elements)
    T Trace(void) const;
    
  }; // class SmallSquareMatrix
  
    
  // Alias some common small matrix sizes, like 3x3
  using Matrix_2x2f = SmallSquareMatrix<2,float>;
  using Matrix_3x3f = SmallSquareMatrix<3,float>;
  using Matrix_3x4f = SmallMatrix<3,4,float>;
  
  // Common 2D matrix-point multiplication:
  template<typename Tmat, typename Tpt, typename Twork=Tpt>
  Point2<Twork> operator*(const SmallSquareMatrix<2,Tmat> &M,
                          const Point2<Tpt> &p);
  
  // Common 3D matrix-point multiplication:
  template<typename Tmat, typename Tpt, typename Twork=Tpt>
  Point3<Twork> operator*(const SmallSquareMatrix<3,Tmat> &M,
                          const Point3<Tpt> &p);
  
} // namespace Anki
  
#endif // _ANKICORETECH_MATRIX_H_
