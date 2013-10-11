#ifndef _ANKICORETECH_COMMON_ARRAY2D_H_
#define _ANKICORETECH_COMMON_ARRAY2D_H_

#include "anki/common/types.h"

#include "exceptions.h"

#include <iostream>
#include <assert.h>
#include <limits.h>

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{
/*
  namespace Embedded
  {
    template<typename T> class Array2d;
  }
*/

#pragma mark --- Array2d(Managed) Class Definition ---

  //
  // Array2d Class
  // A generic, templated 2D storage array.
  // Currently, this inherits from cv::Mat_<T> in order to provide basic
  // members and accessors as well as handy smart pointer functionality.
  //
  // Despite the name, this class is substantially different from the
  // Embedded::Array2d class.
  //
  template<typename T>
  class Array2d : private cv::Mat_<T> // Note the private inheritance!
  {
  public:

    // Constructors:
    Array2d();
    Array2d(s32 nrows, s32 ncols); // alloc/de-alloc handled for you
    Array2d(s32 nrows, s32 ncols, T *data); // you handle memory yourself
    Array2d(s32 nrows, s32 ncols, const T &data); // you handle memory yourself
    Array2d(s32 nrows, s32 ncols, std::vector<T> &data);
//    Array2d(const Embedded::Array2d<T> &other); // *copies* data from unmanaged array

    // Reference counting assignment (does not copy):
    Array2d<T>& operator= (const Array2d<T> &other);

    // Access by row, col (for isolated access):
    T  operator() (const unsigned int row, const unsigned int col) const;
    T& operator() (const unsigned int row, const unsigned int col);

#if ANKICORETECH_USE_OPENCV
    // Create an Array2d from a cv::Mat_<T> without copying any data
    Array2d(const cv::Mat_<T> &other);
    Array2d<T>& operator= (const cv::Mat_<T> &other);

    // Returns a templated cv::Mat_ that shares the same buffer with
    // this Array2d. No data is copied.
    cv::Mat_<T>& get_CvMat_();
    const cv::Mat_<T>& get_CvMat_() const;

#endif

    // Accessors:
    inline s32 numRows() const;
    inline s32 numCols() const;
    inline s32 numElements() const;

    inline bool isEmpty() const;

    // Don't let the public mess with my data, but they can see it:
    const T* getDataPointer() const;

    // Apply a function to every element, in place:
    void applyScalarFunction(T (*fcn)(T));

    // Apply a function pointer to every element, storing the
    // result in a separate, possibly differently-typed, result:
    template<class Tresult>
    void applyScalarFunction(void(*fcn)(const T&, Tresult&),
      Array2d<Tresult> &result) const;

  protected:
    // Sub-classes can get write access to the data:
    T* getDataPointer();
  }; // class Array2d(Managed)

  // Return true iff two arrays are EXACTLY equal:
  template<typename T>
  bool operator==(const Array2d<T> &array1, const Array2d<T> &array2);
  
  // Return true iff two arrays are NEARLY equal, up to some epsilon:
  template<typename T>
  bool nearlyEqual(const Array2d<T> &array1, const Array2d<T> &array2,
                   const T eps = T(10)*std::numeric_limits<T>::epsilon());
  
  
#pragma mark --- Array2d(Managed) Implementation ---
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
  const T* Array2d<T>::getDataPointer(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return this->template ptr<T>(0);
#else
    return this->data;
#endif
  }

  template<typename T>
  T* Array2d<T>::getDataPointer(void)
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
  T  Array2d<T>::operator() (const unsigned int row,
    const unsigned int col) const
  {
    CORETECH_THROW_IF(row >= numRows() || col >= numCols());
    
#if ANKICORETECH_USE_OPENCV
    // Provide thin wrapper to OpenCV's (row,col) access:
    return cv::Mat_<T>::operator()(row,col);
#endif
  }

  template<typename T>
  T& Array2d<T>::operator() (const unsigned int row, const unsigned int col)
  {
    CORETECH_THROW_IF(row >= numRows() || col >= numCols());
    
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
  s32 Array2d<T>::numRows(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return s32(this->rows);
#endif
  }

  template<typename T>
  s32 Array2d<T>::numCols(void) const
  {
#if ANKICORETECH_USE_OPENCV
    return s32(this->cols);
#endif
  }

  template<typename T>
  s32 Array2d<T>::numElements(void) const
  {
    return this->numRows()*this->numCols();
  }

  template<typename T>
  bool Array2d<T>::isEmpty() const
  {
    // Thin wrapper to OpenCV's empty() check:
    return this->empty();
  }
  
  template<typename T>
  bool operator==(const Array2d<T> &array1, const Array2d<T> &array2)
  {
    bool equal = (array1.numRows() == array2.numRows() &&
                  array1.numCols() == array2.numCols());
    
    const T* data1 = array1.getDataPointer();
    const T* data2 = array2.getDataPointer();
    
    s32 i=0;
    while(equal && i < array1.numElements())
    {
      equal = data1[i] == data2[i];
      ++i;
    }
    
    return equal;
    
  } // operator==
  
  
  template<typename T>
  bool nearlyEqual(const Array2d<T> &array1, const Array2d<T> &array2,
                   const T eps)
  {
    bool equal = (array1.numRows() == array2.numRows() &&
                  array1.numCols() == array2.numCols());
    
    const T* data1 = array1.getDataPointer();
    const T* data2 = array2.getDataPointer();
    
    s32 i=0;
    while(equal && i < array1.numElements())
    {
      equal = NEAR(data1[i], data2[i], eps);
      ++i;
    }
    
    return equal;
    
  } // nearlyEqual()
  

  template<typename T>
  void Array2d<T>::applyScalarFunction(T (*fcn)(T))
  {
    s32 nrows = this->numRows();
    s32 ncols = this->numCols();

    if (this->isContinuous()) {
      ncols *= nrows;
      nrows = 1;
    }

    for(s32 i=0; i<nrows; ++i)
    {
      T *data_i = (*this)[i];

      for (s32 j=0; j<ncols; ++j)
      {
        data_i[j] = fcn(data_i[j]);
      }
    }
  } // applyScalarFunction() in place

  template<typename T>
  template<typename Tresult>
  void Array2d<T>::applyScalarFunction(void(*fcn)(const T&, Tresult&),
    Array2d<Tresult> &result) const
  {
    s32 nrows = this->numRows();
    s32 ncols = this->numCols();

    assert(result.size() == this->size());

    if (this->isContinuous() && result.isContinuous() ) {
      ncols *= nrows;
      nrows = 1;
    }

    for(s32 i=0; i<nrows; ++i)
    {
      const T *data_i = (*this)[i];
      Tresult *result_i = result[i];

      for (s32 j=0; j<ncols; ++j)
      {
        fcn(data_i[j], result_i[j]);
      }
    }
  } // applyScalarFunction() to separate result

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


#endif // _ANKICORETECH_COMMON_ARRAY2D_H_
