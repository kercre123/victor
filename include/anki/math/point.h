/**
 * File: point.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Implements a general N-dimensional Point class and two 
 *              subclasses for commonly-used 2D and 3D points.  The latter have
 *              special accessors for x, y, and z components as well. All offer
 *              math capabilities and are templated to store any type.
 *
 *              NOTE: These classes may also be useful to store 2-, 3- or 
 *                    N-dimensional vectors as well. Thus there are also 
 *                    typedefs for Vec3f and Vec2f.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_COMMON_POINT_H_
#define _ANKICORETECH_COMMON_POINT_H_

#include <cmath>

#include "anki/common/exceptions.h"

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/core/core.hpp"
#endif

namespace Anki {
  
  // Generic N-dimensional Point class
  template<typename T, size_t N>
  class Point
  {
  public:
    // Constructors
    Point( void );
    Point(const Point<T,N>& pt);
    
    // Accessors:
    T  operator[](const size_t i) const;
    T& operator[](const size_t i);
    
    // Arithmetic Operators
    Point<T,N>& operator+= (const T value);
    Point<T,N>& operator-= (const T value);
    Point<T,N>& operator*= (const T value);
    Point<T,N>& operator+= (const Point<T,N> &other);
    Point<T,N>& operator-= (const Point<T,N> &other);
    
    // Math methods:
    T length(void) const;
    Point<T,N>& makeUnitLength(void);
    
    // Friendly operators
    friend bool       operator== (const Point<T,N> &point1, const Point<T,N> &point2);
    friend Point<T,N> operator+  (const Point<T,N> &point1, const Point<T,N> &point2);
    friend Point<T,N> operator-  (const Point<T,N> &point1, const Point<T,N> &point2);
    
  protected:
    // Members:
    T data[N];
    
  }; // class Point
  
  
  // Special 2D Point class with elements x and y
  template<typename T>
  class Point2 : public Point<T,2>
  {
  public:
    // Constructors:
    Point2();
    Point2(const T x, const T y);
#if defined(ANKICORETECH_USE_OPENCV)
    Point2(const cv::Point_<T>& pt);
    cv::Point_<T> get_CvPoint_() const;
#endif
    
    // Accessors:
    T& x();
    T& y();
    T  x() const;
    T  y() const;
    
  }; // class Point2
  
  typedef Point2<float> Point2f;
  typedef Point2f Vec2f;
  
  
  // Special 3D Point class with elements x, y, and z
  template<typename T>
  class Point3 : public Point<T,3>
  {
  public:
    // Constructors:
    Point3();
    Point3(const T x, const T y, const T z);
#if defined(ANKICORETECH_USE_OPENCV)
    Point3(const cv::Point3_<T>& pt);
    cv::Point3_<T> get_CvPoint_() const;
#endif
    
    // Accessors:
    T& x();
    T& y();
    T& z();
    T  x() const;
    T  y() const;
    T  z() const;
    
  }; // class Point3
  
  typedef Point3<float> Point3f;
  typedef Point3f Vec3f;
  
#pragma mark --- Point Implementations ---
  
  template<typename T, size_t N>
  Point<T,N>::Point( void )
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] = T(0);
    }
  }
  
  template<typename T, size_t N>
  Point<T,N>::Point(const Point<T,N>& pt)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] = pt.data[i];
    }
  }
  
  template<typename T, size_t N>
  T Point<T,N>::operator[](const size_t i) const
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<typename T, size_t N>
  T& Point<T,N>::operator[](const size_t i)
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::operator*= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] *= value;
    }
    return *this;
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::operator+= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] += value;
    }
    return *this;
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::operator-= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] -= value;
    }
    return *this;
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::operator+= (const Point<T,N> &other)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] += other.data[i];
    }
    return *this;
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::operator-= (const Point<T,N> &other)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] -= other.data[i];
    }
    return *this;
  }
  
  template<typename T, size_t N>
  bool operator== (const Point<T,N> &point1, const Point<T,N> &point2)
  {
    CORETECH_ASSERT(N>0);
    
    // Return true if all elements of data are equal, false otherwise.
    bool retVal = point1.data[0] == point2.data[0];
    size_t i = 1;
    while(retVal && i<N) {
      retVal = point1.data[i] == point2.data[i];
    }
    
    return retVal;
  }
  
  template<typename T, size_t N>
  Point<T,N> operator+ (const Point<T,N> &point1, const Point<T,N> &point2)
  {
    Point<T,N> newPoint(point1);
    newPoint += point2;
    return newPoint;
  }
  
  template<typename T, size_t N>
  Point<T,N> operator- (const Point<T,N> &point1, const Point<T,N> &point2)
  {
    Point<T,N> newPoint(point1);
    newPoint -= point2;
    return newPoint;
  }
  
  template<typename T, size_t N>
  T Point<T,N>::length(void) const
  {
    CORETECH_ASSERT(N>0);
    T retVal = this->data[0]*this->data[0];
    for(size_t i=1; i<N; ++i) {
      retVal += this->data[i]*this->data[i];
    }
    
    return std::sqrt(retVal);
  }
  
  template<typename T, size_t N>
  Point<T,N>& Point<T,N>::makeUnitLength(void)
  {
    (*this) *= T(1)/this->length();
    return *this;
  }

  
  // Point2's constructors
  template<typename T>
  Point2<T>::Point2(void)
  : Point<T,2>()
  {
    
  }
  
  template<typename T>
  Point2<T>::Point2(const T x, const T y)
  {
    this->x() = x;
    this->y() = y;
  }
  
#if defined(ANKICORETECH_USE_OPENCV)
  template<typename T>
  Point2<T>::Point2(const cv::Point_<T>& pt)
  : Point2(pt.x, pt.y)
  {
    
  }
  
  template<typename T>
  cv::Point_<T> Point2<T>::get_CvPoint_() const
  {
    return cv::Point_<T>(this->x(), this->y());
  }
#endif
  
  // Point3's constructors
  template<typename T>
  Point3<T>::Point3(void)
  : Point<T,3>()
  {
    
  }
  
  template<typename T>
  Point3<T>::Point3(const T x, const T y, const T z)
  {
    this->x() = x;
    this->y() = y;
    this->z() = z;
  }
  
#if defined(ANKICORETECH_USE_OPENCV)
  template<typename T>
  Point3<T>::Point3(const cv::Point3_<T>& pt)
  : Point3(pt.x, pt.y, pt.z)
  {
    
  }
  
  template<typename T>
  cv::Point3_<T> Point3<T>::get_CvPoint_() const
  {
    return cv::Point3_<T>(this->x(), this->y(), this->z());
  }
#endif
  
  // Point2's x,y accessors:
  template<typename T>
  inline T& Point2<T>::x(void) {
    return this->data[0];
  }
  
  template<typename T>
  inline T& Point2<T>::y(void) {
    return this->data[1];
  }
  
  template<typename T>
  inline T Point2<T>::x(void) const
  {
    return this->data[0];
  }
  
  template<typename T>
  inline T Point2<T>::y(void) const
  {
    return this->data[1];
  }
  
  // Point3's x,y,z accessors:
  
  template<typename T>
  inline T& Point3<T>::x(void) {
    return this->data[0];
  }
  
  template<typename T>
  inline T& Point3<T>::y(void) {
    return this->data[1];
  }
  
  template<typename T>
  inline T& Point3<T>::z(void) {
    return this->data[2];
  }
  
  template<typename T>
  inline T Point3<T>::x(void) const
  {
    return this->data[0];
  }
  
  template<typename T>
  inline T Point3<T>::y(void) const
  {
    return this->data[1];
  }
  
  template<typename T>
  inline T Point3<T>::z(void) const
  {
    return this->data[2];
  }
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_H_
