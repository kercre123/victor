/**
 * File: array2d_impl.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Implements a generic, templated Array2d storage class.
 *
 *   Currently, this inherits from cv::Mat_<T> in order to provide basic
 *   members and accessors as well as handy smart pointer functionality.
 *
 *  NOTE:
 *   Despite the common name, this class is substantially different from the
 *   Embedded::Array2d class.
 *
 *
 * Copyright: Anki, Inc. 2013
 *
 **/



#ifndef _ANKICORETECH_COMMON_ARRAY2D_IMPL_H_
#define _ANKICORETECH_COMMON_ARRAY2D_IMPL_H_

#include "anki/common/basestation/array2d.h"

#include "anki/common/types.h"

#include "exceptions.h"

#include <iostream>
#include <assert.h>
#include <limits.h>


namespace Anki
{

  template<typename T>
  Array2d<T>::Array2d(void)
#if ANKICORETECH_USE_OPENCV
    : cv::Mat_<T>()
#endif
  {
  } // Constructor: Array2d()

  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols)
#if ANKICORETECH_USE_OPENCV
    : cv::Mat_<T>(numRows, numCols)
#endif
  {
  } // Constructor: Array2d(rows,cols)

  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols, T *data)
#if ANKICORETECH_USE_OPENCV
    : cv::Mat_<T>(numRows, numCols, data)
#endif
  {
  } // Constructor: Array2d(rows, cols, *data)
  
  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols, std::vector<T> &data)
#if ANKICORETECH_USE_OPENCV
  : cv::Mat_<T>(numRows, numCols, &(data[0]))
#endif
  {
  } // Constructor: Array2d(rows, cols, *data)

  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols, const T &data)
#if ANKICORETECH_USE_OPENCV
  : cv::Mat_<T>(numRows, numCols, data)
#endif
  {
  } // Constructor: Array2d(rows, cols, &data)
  
  
  template<typename T>
  void Array2d<T>::CopyTo(Array2d<T> &other) const
  {
#if ANKICORETECH_USE_OPENCV
    this->copyTo(other);
#endif
  }
  
  
#if ANKICORETECH_USE_OPENCV
  template<typename T>
  Array2d<T>::Array2d(const cv::Mat_<T> &other)
    : cv::Mat_<T>(other)
  {
  } // Constructor: Array2d( cv::Mat_<T> )

  template<typename T>
  Array2d<T>& Array2d<T>::operator= (const cv::Mat_<T> &other)
  {
    cv::Mat_<T>::operator=(other);
    return *this;
  }
#endif

/*
  template<typename T>
  Array2d<T>::Array2d(const Embedded::Array2d<T> &other)
#if ANKICORETECH_USE_OPENCV
    : Array2d<T>(other.get_cvMat_())
#endif
  {
  } // Constructor: Array2d( Embedded::Array2d )
*/

  template<typename T>
  T* Array2d<T>::GetRow(s32 row)
  {
#if ANKICORETECH_USE_OPENCV
    return this->operator[](row);
#endif
  }
  
  
  template<typename T>
  const T* Array2d<T>::GetRow(s32 row) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->operator[](row);
#endif
  }
  
  
  template<typename T>
  const T* Array2d<T>::GetDataPointer(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->template ptr<T>(0);
#else
    return this->data;
#endif
  }

  template<typename T>
  T* Array2d<T>::GetDataPointer(void)
  {
#if ANKICORETECH_USE_OPENCV
    return this->template ptr<T>(0);
#else
    return this->data;
#endif
  }

  template<typename T>
  Array2d<T>& Array2d<T>::operator=(const Array2d<T> &other)
  {
#if ANKICORETECH_USE_OPENCV
    // Provide thin wrapper to OpenCV's handy reference-counting assignment:
    cv::Mat_<T>::operator=(other);
    return *this;
#endif
  }

  template<typename T>
  T  Array2d<T>::operator() (const int row, const int col) const
  {
    CORETECH_THROW_IF(row >= GetNumRows() || col >= GetNumCols());
    
#if ANKICORETECH_USE_OPENCV
    // Provide thin wrapper to OpenCV's (row,col) access:
    return cv::Mat_<T>::operator()(row,col);
#endif
  }

  template<typename T>
  T& Array2d<T>::operator() (const int row, const int col)
  {
    CORETECH_THROW_IF(row >= GetNumRows() || col >= GetNumCols());
    
#if ANKICORETECH_USE_OPENCV    
    // Provide thin wrapper to OpenCV's (row,col) access:
    return cv::Mat_<T>::operator()(row,col);
#endif
  }

#if ANKICORETECH_USE_OPENCV

  template<typename T>
  cv::Mat_<T>& Array2d<T>::get_CvMat_()
  {
    return *this; //static_cast<cv::Mat_<T> >(*this);
  }

  template<typename T>
  const cv::Mat_<T>& Array2d<T>::get_CvMat_() const
  {
    return *this; // static_cast<cv::Mat_<T> >(*this);
  }

#endif // #if ANKICORETECH_USE_OPENCV

  template<typename T>
  s32 Array2d<T>::GetNumRows(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return s32(this->rows);
#endif
  }

  template<typename T>
  s32 Array2d<T>::GetNumCols(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return s32(this->cols);
#endif
  }

  template<typename T>
  s32 Array2d<T>::GetNumElements(void) const
  {
    return this->GetNumRows()*this->GetNumCols();
  }

  template<typename T>
  bool Array2d<T>::IsEmpty() const
  {
#if ANKICORETECH_USE_OPENCV
    // Thin wrapper to OpenCV's empty() check:
    return this->empty();
#endif
  }
  
  template<typename T>
  bool operator==(const Array2d<T> &array1, const Array2d<T> &array2)
  {
    bool equal = (array1.GetNumRows() == array2.GetNumRows() &&
                  array1.GetNumCols() == array2.GetNumCols());
    
    const T* data1 = array1.getDataPointer();
    const T* data2 = array2.getDataPointer();
    
    s32 i=0;
    while(equal && i < array1.GetNumElements())
    {
      equal = data1[i] == data2[i];
      ++i;
    }
    
    return equal;
    
  } // operator==
  
  
  template<typename T>
  bool IsNearlyEqual(const Array2d<T> &array1, const Array2d<T> &array2,
                   const T eps)
  {
    bool equal = (array1.GetNumRows() == array2.GetNumRows() &&
                  array1.GetNumCols() == array2.GetNumCols());
    
    const T* data1 = array1.GetDataPointer();
    const T* data2 = array2.GetDataPointer();
    
    s32 i=0;
    while(equal && i < array1.GetNumElements())
    {
      equal = NEAR(data1[i], data2[i], eps);
      ++i;
    }
    
    return equal;
    
  } // nearlyEqual()
  

  template<typename T>
  //void Array2d<T>::ApplyScalarFunction(T (*fcn)(T))
  void Array2d<T>::ApplyScalarFunction(std::function<T(T)>fcn)
  {
    s32 nrows = this->GetNumRows();
    s32 ncols = this->GetNumCols();

    if (this->IsContinuous()) {
      ncols *= nrows;
      nrows = 1;
    }

    for(s32 i=0; i<nrows; ++i)
    {
      T *data_i = this->GetRow(i);
      
      for (s32 j=0; j<ncols; ++j)
      {
        data_i[j] = fcn(data_i[j]);
      }
    }
  } // applyScalarFunction() in place

  template<typename T>
  template<typename Tresult>
  //void Array2d<T>::ApplyScalarFunction(void(*fcn)(const T&, Tresult&),
  void Array2d<T>::ApplyScalarFunction(std::function<void(const T&, Tresult&)>fcn,
                                       Array2d<Tresult> &result) const
  {
    s32 nrows = this->GetNumRows();
    s32 ncols = this->GetNumCols();

    assert(result.GetNumRows() == this->GetNumRows() &&
           result.GetNumCols() == this->GetNumCols());

    if (this->IsContinuous() && result.IsContinuous() ) {
      ncols *= nrows;
      nrows = 1;
    }

    for(s32 i=0; i<nrows; ++i)
    {
      const T *data_i = this->GetRow(i);
      Tresult *result_i = result.GetRow(i);

      for (s32 j=0; j<ncols; ++j)
      {
        fcn(data_i[j], result_i[j]);
      }
    }
  } // applyScalarFunction() to separate result
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator+=(const Array2d<T>& other)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() += other.get_CvMat_();
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator-=(const Array2d<T>& other)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() -= other.get_CvMat_();
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator*=(const Array2d<T>& other)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() = this->mul(other.get_CvMat_());
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator/=(const Array2d<T>& other)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() /= other.get_CvMat_();
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator+=(const T& scalarValue)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() += scalarValue;
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator-=(const T& scalarValue)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() -= scalarValue;
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator*=(const T& scalarValue)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() *= scalarValue;
#   endif
    return *this;
  }
  
  template<typename T>
  Array2d<T>& Array2d<T>::operator/=(const T& scalarValue)
  {
#   if ANKICORETECH_USE_OPENCV
    this->get_CvMat_() /= scalarValue;
#   endif
    return *this;
  }
  
  
  template<typename T>
  Array2d<T>& Array2d<T>::Abs()
  {
#   if ANKICORETECH_USE_OPENCV
    *this = cv::abs(*this);
#   endif
    return *this;
  }
  
  /* OLD: Inherit from unmanaged

  // This constructor uses the OpenCv managedData matrix to allocate the data,
  // and then points the inherited (unmanaged) data pointer to that.
  template<typename T>
  Array2d<T>::Array2d(s32 numRows, s32 numCols, bool useBoundaryFillPatterns)
  {
  // Allocate a memory-aligned array the way we'd expect a user of the
  // UNmanaged parent class to do it:
  this->stride = Embedded::Array2d<T>::ComputeRequiredStride(numCols, useBoundaryFillPatterns);
  s32 dataLength = Embedded::Array2d<T>::ComputeMinimumRequiredMemory(numRows, numCols, useBoundaryFillPatterns);
  void *rawData = cv::fastMalloc(dataLength);

  initialize(numRows, numCols, rawData, dataLength, useBoundaryFillPatterns);

  // Create an OpenCv header around our data array
  this->dataManager = cv::Mat_<T>(numRows, numCols, (T *) this->data);

  // "Trick" OpenCv into doing reference counting, even though we passed in
  // our own data array.  So now when this class destructs and dataManager
  // destructs, it will free this->data for us.
  this->refCount = 1;
  this->dataManager.refcount = &(this->refCount);
  } // Constructor: Array2d(rows,cols)

  */
} //namespace Anki


#endif // _ANKICORETECH_COMMON_ARRAY2D_IMPL_H_
