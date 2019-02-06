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


#ifndef _ANKICORETECH_COMMON_ARRAY2D_H_
#define _ANKICORETECH_COMMON_ARRAY2D_H_

#include "coretech/common/shared/types.h"
#include "coretech/common/shared/math/rect.h"

#include <vector>
#include <functional>
#include <limits.h>

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{

  template<typename T>
  class Array2d
#if ANKICORETECH_USE_OPENCV
  : private cv::Mat_<T> // Note the private inheritance!
#endif
  {
  public:

    // Constructors:
    Array2d();
    Array2d(s32 nrows, s32 ncols); // alloc/de-alloc handled for you
    Array2d(s32 nrows, s32 ncols, T *data); // you handle memory yourself
    Array2d(s32 nrows, s32 ncols, const T &data); // you handle memory yourself
    Array2d(s32 nrows, s32 ncols, std::vector<T> &data);
//    Array2d(const Embedded::Array2d<T> &other); // *copies* data from unmanaged array

    // Allocate to a given size (does NOT initialize the data).
    // Only reallocates its memory if the new size will not fit into the old one.
    // Otherwise just changes the size of the array and reuses memory. (Like cv::Mat.)
    void Allocate(s32 nrows, s32 ncols);
    
    // "Empties" an image to make it size zero, but does not actually release memory.
    void Clear() { Allocate(0,0); }
    
    // Reference counting assignment (does not copy):
    Array2d<T>& operator= (const Array2d<T> &other);
    
    // Copies to another Array2d
    void CopyTo(Array2d<T> &other) const;

    // GetROI() will crop input rectangle to image bounds if needed.
    // NOTE: Returned array could be empty!
    template<typename T_rect>
    Array2d<T> GetROI(Rectangle<T_rect>& roiRect);
    
    template<typename T_rect>
    const Array2d<T> GetROI(Rectangle<T_rect>& roiRect) const;
    
    // Access by row, col (for isolated access):
    T  operator() (const int row, const int col) const;
    T& operator() (const int row, const int col);
    
    // Get a pointer to the beginning of a row
    T*       GetRow(s32 row);
    const T* GetRow(s32 row) const;
    
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
    s32 GetNumRows() const;
    s32 GetNumCols() const;
    s32 GetNumElements() const;

    bool IsEmpty() const;

    // Don't let the public mess with my data, but they can see it:
    const T* GetDataPointer() const;

    // Apply a function to every element, in place:
    void ApplyScalarFunction(std::function<T(T)>fcn);
    
    // Apply a function pointer to every element, storing the
    // result in a separate, possibly differently-typed, result:
    template<class Tresult>
    void ApplyScalarFunction(std::function<Tresult(const T&)>fcn, Array2d<Tresult> &result) const;
    
    // Apply scalar function that takes each pixel of this array and another array
    // of the same size and stores in a given output array
    template<class Tother, class Tresult>
    void ApplyScalarFunction(std::function<Tresult(const T& thisElem, const Tother& otherElem)>fcn,
                             const Array2d<Tother>& otherArray,
                             Array2d<Tresult>& result) const;
    
    Array2d<T>& operator+=(const Array2d<T>& other);
    Array2d<T>& operator-=(const Array2d<T>& other);
    Array2d<T>& operator*=(const Array2d<T>& other);
    Array2d<T>& operator/=(const Array2d<T>& other);

    Array2d<T>& operator+=(const T& scalarValue);
    Array2d<T>& operator-=(const T& scalarValue);
    Array2d<T>& operator*=(const T& scalarValue);
    Array2d<T>& operator/=(const T& scalarValue);
    
    // Absolute value, in place
    Array2d<T>& Abs();
    
    void FillWith(T value);
    void SetMaskTo(const Array2d<u8>& mask, T value);
    
#if ANKICORETECH_USE_OPENCV
    bool IsContinuous() const { return cv::Mat_<T>::isContinuous(); }
#else
    bool IsContinuous() const { return true; }
#endif
    
  protected:
    // Sub-classes can get write access to the data:
    T* GetDataPointer();
    
  }; // class Array2d(Managed)

  
  // Return true iff two arrays are EXACTLY equal:
  template<typename T>
  bool operator==(const Array2d<T> &array1, const Array2d<T> &array2);
  
  // Return true iff two arrays are NEARLY equal, up to some epsilon:
  template<typename T>
  bool IsNearlyEqual(const Array2d<T> &array1, const Array2d<T> &array2,
                     const T eps = T(10)*std::numeric_limits<T>::epsilon());
  
  
} //namespace Anki


#endif // _ANKICORETECH_COMMON_ARRAY2D_H_
