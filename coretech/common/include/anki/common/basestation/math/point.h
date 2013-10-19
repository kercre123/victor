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

#include "anki/common/basestation/exceptions.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki {
  
  // Generic N-dimensional Point class
  template<size_t N, typename T>
  class Point //: public std::array<T,N>
  {
    static_assert(N>0, "Cannot create an empty Point.");
    
  public:
    
    // Constructors
    Point( void );
    Point(const Point<N,T>& pt);
    
    //Point(const T x, const T y); // Only valid if N==2
    //Point(const T x, const T y, const T z); // Only valid if N==3

#if __cplusplus == 201103L
    // Point(T x1, T x2, ..., T xN);
    // This is ugly variadic template syntax to get this constructor to
    // work for all N and generate a compile-time error if you try to do
    // something like: Point<2,int> p(1, 2, 3);
    //
    // See here for more:
    //  http://stackoverflow.com/questions/8158261/templates-how-to-control-number-of-constructor-args-using-template-variable
    template <typename... Tail>
    Point(typename std::enable_if<sizeof...(Tail)+1 == N, T>::type head, Tail... tail)
    : data{ head, T(tail)... } {}
#else
    Point(const T x, const T y); // Only valid if N==2
    Point(const T x, const T y, const T z); // Only valid if N==3
#endif
    
    // Assignment operator:
    Point<N,T>& operator=(const Point<N,T> &other);
    
    // Accessors:
    T& operator[] (const size_t i);
    const T& operator[] (const size_t i) const;
    
    // Special mnemonic accessors for the first, second,
    // and third elements, available when N is large enough.
    
    T& x();
    T& y();
    T& z();
    const T& x() const;
    const T& y() const;
    const T& z() const;
    
#if ANKICORETECH_USE_OPENCV
    Point(const cv::Point_<T>& pt);
    Point(const cv::Point3_<T>& pt);
    cv::Point_<T> get_CvPoint_() const;
    cv::Point3_<T> get_CvPoint3_() const;
#endif
    
    // Arithmetic Operators
    Point<N,T>& operator+= (const T value);
    Point<N,T>& operator-= (const T value);
    Point<N,T>& operator*= (const T value);
    Point<N,T>& operator+= (const Point<N,T> &other);
    Point<N,T>& operator-= (const Point<N,T> &other);
    
    // Math methods:
    
    // Return length of the vector from the origin to the point
    T length(void) const;
    
    // Makes the point into a unit vector from the origin, while
    // returning its original length. IMPORTANT: if the point was
    // originally the origin, it cannot be made into a unit vector
    // and will be left at the origin, and zero will be returned.
    T makeUnitLength(void);
    
  protected:
    T data[N];
    
  }; // class Point
  
  // Create some convenience aliases/typedefs for 2D and 3D points:
  template <typename T>
  using Point2 = Point<2, T>;
  
  template <typename T>
  using Point3 = Point<3, T>;
  
  typedef Point2<float> Point2f;
  typedef Point3<float> Point3f;
  
  // TODO: Do need a separate Vec class?
  typedef Point3f Vec3f;
  
  // Display / Logging:
  template<size_t N, typename T>
  std::ostream& operator<<(std::ostream& out, const Point<N,T>& p);
  
  // Binary mathematical operations:
  template<size_t N, typename T>
  bool operator== (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<size_t N, typename T>
  bool nearlyEqual(const Point<N,T> &point1, const Point<N,T> &point2,
                   const T eps = T(10)*std::numeric_limits<T>::epsilon());
  
  template<size_t N, typename T>
  Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<size_t N, typename T>
  Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<size_t N, typename T>
  T dot(const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<typename T>
  Point3<T> cross(const Point3<T> &point1, const Point3<T> &point2);
  
  
#pragma mark --- Point Implementations ---
  
  template<size_t N, typename T>
  Point<N,T>::Point( void )
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] = T(0);
    }
  }
  
  template<size_t N, typename T>
  Point<N,T>::Point(const Point<N,T>& pt)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] = pt.data[i];
    }
  }
  
#if __cplusplus < 201103L
  template<size_t N, typename T>
  Point<N,T>::Point(const T x, const T y)
  {
    static_assert(N==2, "This constructor only works for 2D points.");
    this->data[0] = x;
    this->data[1] = y;
  }
  
  template<size_t N, typename T>
  Point<N,T>::Point(const T x, const T y, const T z)
  {
    static_assert(N==3, "This constructor only works for 3D points.");
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
  }
#endif
  
#if ANKICORETECH_USE_OPENCV
  template<size_t N, typename T>
  Point<N,T>::Point(const cv::Point_<T>& pt)
  : Point(pt.x, pt.y)
  {
  }
  
  template<size_t N, typename T>
  Point<N,T>::Point(const cv::Point3_<T>& pt)
  : Point(pt.x, pt.y, pt.z)
  {
    
  }
  
  template<size_t N, typename T>
  cv::Point_<T> Point<N,T>::get_CvPoint_() const
  {
    static_assert(N==2, "N must be 2 to convert to cv::Point_<T>.");
    return cv::Point_<T>(this->x(), this->y());
  }
  
  template<size_t N, typename T>
  cv::Point3_<T> Point<N,T>::get_CvPoint3_() const
  {
    static_assert(N==3, "N must be 3 to convert to cv::Point3_<T>.");
    return cv::Point3_<T>(this->x(), this->y(), this->z());
  }
#endif
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator=(const Point<N,T> &other)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] = other.data[i];
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator*= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] *= value;
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator+= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] += value;
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator-= (const T value)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] -= value;
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator+= (const Point<N,T> &other)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] += other[i];
    }
    return *this;
  }
  
  template<size_t N, typename T>
  Point<N,T>& Point<N,T>::operator-= (const Point<N,T> &other)
  {
    for(size_t i=0; i<N; ++i) {
      this->data[i] -= other[i];
    }
    return *this;
  }
  
  template<size_t N, typename T>
  inline T& Point<N,T>::operator[] (const size_t i)
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<size_t N, typename T>
  inline const T& Point<N,T>::operator[] (const size_t i) const
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<size_t N, typename T>
  inline T& Point<N,T>::x() {
    static_assert(N>0, "Point x() accessor only available when N>0.");
    return this->data[0];
  }
  
  template<size_t N, typename T>
  inline T& Point<N,T>::y() {
    static_assert(N>1, "Point y() accessor only available when N>1.");
    return this->data[1];
  }
  
  template<size_t N, typename T>
  inline T& Point<N,T>::z() {
    static_assert(N>2, "Point z() accessor only available when N>2.");
    return this->data[2];
  }
  
  template<size_t N, typename T>
  inline const T& Point<N,T>::x() const {
    static_assert(N>0, "Point x() accessor only available when N>0.");
    return this->data[0];
  }
  
  template<size_t N, typename T>
  inline const T& Point<N,T>::y() const {
    static_assert(N>1, "Point y() accessor only available when N>1.");
    return this->data[1];
  }
  
  template<size_t N, typename T>
  inline const T& Point<N,T>::z() const {
    static_assert(N>2, "Point z() accessor only available when N>2.");
    return this->data[2];
  }
  
  template<size_t N, typename T>
  bool operator== (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    CORETECH_ASSERT(N>0);
    
    // Return true if all elements of data are equal, false otherwise.
    bool retVal = point1[0] == point2[0];
    size_t i = 1;
    while(retVal && i<N) {
      retVal = point1[i] == point2[i];
      ++i;
    }
    
    return retVal;
  }
  
  template<size_t N, typename T>
  bool       nearlyEqual (const Point<N,T> &point1, const Point<N,T> &point2,
                          const T eps)
  {
    CORETECH_ASSERT(N>0);
    
    // Return true if all elements of data are equal, false otherwise.
    bool retVal = NEAR(point1[0], point2[0], eps);
    size_t i = 1;
    while(retVal && i<N) {
      retVal = NEAR(point1[i], point2[i], eps);
      ++i;
    }
    
    return retVal;
  }
  
  template<size_t N, typename T>
  Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    Point<N,T> newPoint(point1);
    newPoint += point2;
    return newPoint;
  }
  
  template<size_t N, typename T>
  Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    Point<N,T> newPoint(point1);
    newPoint -= point2;
    return newPoint;
  }
  
  template<size_t N, typename T>
  T dot(const Point<N,T> &point1, const Point<N,T> &point2)
  {
    CORETECH_ASSERT(N>0);
    
    float dotProduct = point1[0]*point2[0];
    for(size_t i=1; i<N; ++i) {
      dotProduct += point1[i]*point2[i];
    }
    return dotProduct;
  }
  
  template<size_t N, typename T>
  T Point<N,T>::length(void) const
  {
    CORETECH_ASSERT(N>0);
    T retVal = (*this)[0]*(*this)[0];
    for(size_t i=1; i<N; ++i) {
      retVal += (*this)[i]*(*this)[i];
    }
    
    return std::sqrt(retVal);
  }
  
  template<size_t N, typename T>
  T Point<N,T>::makeUnitLength(void)
  {
    const T length = this->length();
    if(length > 0) {
      (*this) *= T(1)/length;
    }
    return length;
  }
  
  template<size_t N, typename T>
  std::ostream& operator<<(std::ostream& out, const Point<N,T>& p)
  {
    for (int i=0; i<N; ++i) {
      out << p[i] << " ";
    }
    return out;
  }

  template<typename T>
  Point3<T> cross(const Point3<T> &point1, const Point3<T> &point2)
  {
    return Point3<T>(point1.y()*point2.z() - point2.y()*point1.z(),
                     point2.x()*point1.z() - point1.x()*point2.z(),
                     point1.x()*point2.y() - point2.x()*point1.y());
  }
  
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_H_
