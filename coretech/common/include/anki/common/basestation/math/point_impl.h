/**
 * File: point_impl.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Implements a general N-dimensional Point class and two
 *              subclasses for commonly-used 2D and 3D points.  The latter have
 *              special accessors for x, y, and z components as well. All offer
 *              math capabilities and are templated to store any type.
 *
 *              NOTE: These classes may also be useful to store 2-, 3- or
 *                    N-dimensional vectors as well. Thus there are also
 *                    aliases for Vec3f and Vec2f.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef _ANKICORETECH_COMMON_POINT_IMPL_H_
#define _ANKICORETECH_COMMON_POINT_IMPL_H_

#include "anki/common/basestation/math/point.h"

#include "anki/common/shared/radians.h"

#include "anki/common/basestation/exceptions.h"

#include <cmath>

namespace Anki {
  
  template<PointDimType N, typename T>
  Point<N,T>::Point( void )
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = T(0);
    }
  }
  
  template<PointDimType N, typename T>
  template<typename T_other>
  Point<N,T>::Point(const Point<N,T_other>& pt)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = static_cast<T>(pt[i]);
    }
  }
  /*
   template<PointDimType N, typename T>
   Point<N,T>::Point(const std::array<T,N>& array)
   {
   std::copy(this->data, this->data+N, array.begin());
   }
   */

  template<PointDimType N, typename T>
  Point<N,T>::Point(const T value)
  {
    // TODO: force this to unroll?
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = value;
    }
  }
  
  template<PointDimType N, typename T>
  Point<N,T>::Point(const Point<N+1,T>& pt)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = pt[i];
    }
  }
  
#if __cplusplus < 201103L

  template<PointDimType N, typename T>
  Point<N,T>::Point(const T x, const T y)
  {
    static_assert(N==2, "This constructor only works for 2D points.");
    this->data[0] = x;
    this->data[1] = y;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>::Point(const T x, const T y, const T z)
  {
    static_assert(N==3, "This constructor only works for 3D points.");
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
  }
#endif
  
#if ANKICORETECH_USE_OPENCV
  template<PointDimType N, typename T>
  Point<N,T>::Point(const cv::Point_<T>& pt)
  : Point(pt.x, pt.y)
  {
  }
  
  template<PointDimType N, typename T>
  Point<N,T>::Point(const cv::Point3_<T>& pt)
  : Point(pt.x, pt.y, pt.z)
  {
    
  }
  
  template<PointDimType N, typename T>
  cv::Point_<T> Point<N,T>::get_CvPoint_() const
  {
    static_assert(N==2, "N must be 2 to convert to cv::Point_<T>.");
    return cv::Point_<T>(this->x(), this->y());
  }
  
  template<PointDimType N, typename T>
  cv::Point3_<T> Point<N,T>::get_CvPoint3_() const
  {
    static_assert(N==3, "N must be 3 to convert to cv::Point3_<T>.");
    return cv::Point3_<T>(this->x(), this->y(), this->z());
  }
#endif
  
  template<PointDimType N, typename T>
  template<typename T_other>
  Point<N,T>& Point<N,T>::operator=(const Point<N,T_other> &other)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = static_cast<T>(other[i]);
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator=(const T &value)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = value;
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator*= (const T value)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] *= value;
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator+= (const T value)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] += value;
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator-= (const T value)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] -= value;
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator/= (const T value)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] /= value;
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> Point<N,T>::operator* (const T value) const
  {
    Point<N,T> res(*this);
    for(PointDimType i=0; i<N; ++i) {
      res[i] *= value;
    }
    return res;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> Point<N,T>::operator+ (const T value) const
  {
    Point<N,T> res(*this);
    for(PointDimType i=0; i<N; ++i) {
      res[i] += value;
    }
    return res;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator+= (const Point<N,T> &other)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] += other[i];
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator-= (const Point<N,T> &other)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] -= other[i];
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::operator*= (const Point<N,T> &other)
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] *= other[i];
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> Point<N,T>::operator-() const
  {
    Point<N,T> p;
    for(PointDimType i=0; i<N; ++i) {
      p[i] = -this->data[i];
    }
    return p;
  }
  
  template<PointDimType N, typename T>
  inline T& Point<N,T>::operator[] (const PointDimType i)
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<PointDimType N, typename T>
  inline const T& Point<N,T>::operator[] (const PointDimType i) const
  {
    CORETECH_ASSERT(i<N);
    return this->data[i];
  }
  
  template<PointDimType N, typename T>
  inline T& Point<N,T>::x() {
    static_assert(N>0, "Point x() accessor only available when N>0.");
    return this->data[0];
  }
  
  template<PointDimType N, typename T>
  inline T& Point<N,T>::y() {
    static_assert(N>1, "Point y() accessor only available when N>1.");
    return this->data[1];
  }
  
  template<PointDimType N, typename T>
  inline T& Point<N,T>::z() {
    static_assert(N>2, "Point z() accessor only available when N>2.");
    return this->data[2];
  }
  
  template<PointDimType N, typename T>
  inline const T& Point<N,T>::x() const {
    static_assert(N>0, "Point x() accessor only available when N>0.");
    return this->data[0];
  }
  
  template<PointDimType N, typename T>
  inline const T& Point<N,T>::y() const {
    static_assert(N>1, "Point y() accessor only available when N>1.");
    return this->data[1];
  }
  
  template<PointDimType N, typename T>
  inline const T& Point<N,T>::z() const {
    static_assert(N>2, "Point z() accessor only available when N>2.");
    return this->data[2];
  }
  
  template<PointDimType N, typename T>
  bool Point<N,T>::operator< (const Point<N,T>& other) const
  {
    CORETECH_ASSERT(N>0);
    bool retVal = this->data[0] < other[0];
    PointDimType i = 1;
    while(retVal && i<N) {
      retVal = this->data[i] < other[i];
      ++i;
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  bool Point<N,T>::operator> (const Point<N,T>& other) const
  {
    CORETECH_ASSERT(N>0);
    bool retVal = this->data[0] > other[0];
    PointDimType i = 1;
    while(retVal && i<N) {
      retVal = this->data[i] > other[i];
      ++i;
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  bool Point<N,T>::operator==(const Point<N,T>& other) const
  {
    CORETECH_ASSERT(N>0);
    bool retVal = this->data[0] == other[0];
    PointDimType i = 1;
    while(retVal && i<N) {
      retVal = this->data[i] == other[i];
      ++i;
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  bool operator== (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    CORETECH_ASSERT(N>0);
    
    // Return true if all elements of data are equal, false otherwise.
    bool retVal = point1[0] == point2[0];
    PointDimType i = 1;
    while(retVal && i<N) {
      retVal = point1[i] == point2[i];
      ++i;
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  bool       IsNearlyEqual (const Point<N,T> &point1, const Point<N,T> &point2,
                          const T eps)
  {
    // Return true if all elements of data are equal, false otherwise.
    bool retVal = NEAR(point1[0], point2[0], eps);
    PointDimType i = 1;
    while(retVal && i<N) {
      retVal = NEAR(point1[i], point2[i], eps);
      ++i;
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    Point<N,T> newPoint(point1);
    newPoint += point2;
    return newPoint;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2)
  {
    Point<N,T> newPoint(point1);
    newPoint -= point2;
    return newPoint;
  }
  
  template<PointDimType N, typename T>
  T DotProduct(const Point<N,T> &point1, const Point<N,T> &point2)
  {
    float dotProduct = point1[0]*point2[0];
    for(PointDimType i=1; i<N; ++i) {
      dotProduct += point1[i]*point2[i];
    }
    return dotProduct;
  }
  
  template<PointDimType N, typename T>
  Point<N,T> Point<N,T>::GetAbs() const
  {
    Point<N,T> copy(*this);
    return copy.Abs();
  }
  
  template<PointDimType N, typename T>
  Point<N,T>& Point<N,T>::Abs()
  {
    for(PointDimType i=0; i<N; ++i) {
      this->data[i] = std::abs(this->data[i]);
    }
    return *this;
  }
  
  template<PointDimType N, typename T>
  T Point<N,T>::LengthSq(void) const
  {
    T retVal = (*this)[0]*(*this)[0];
    for(PointDimType i=1; i<N; ++i) {
      retVal += (*this)[i]*(*this)[i];
    }
    
    return retVal;
  }
  
  template<PointDimType N, typename T>
  T Point<N,T>::Length(void) const
  {
    return std::sqrt(LengthSq());
  }
  
  template<PointDimType N, typename T>
  T Point<N,T>::MakeUnitLength(void)
  {
    const T length = this->Length();
    if(length > 0) {
      (*this) *= T(1)/length;
    }
    return length;
  }
  
  template<PointDimType N, typename T>
  std::ostream& operator<<(std::ostream& out, const Point<N,T>& p)
  {
    for (PointDimType i=0; i<N; ++i) {
      out << p[i] << " ";
    }
    return out;
  }
  
  template<typename T>
  Point3<T> CrossProduct(const Point3<T> &point1, const Point3<T> &point2)
  {
    return Point3<T>(point1.y()*point2.z() - point2.y()*point1.z(),
                     point2.x()*point1.z() - point1.x()*point2.z(),
                     point1.x()*point2.y() - point2.x()*point1.y());
  }
  
  template<PointDimType N, typename T>
  T ComputeDistanceBetween(const Point<N,T>& point1, const Point<N,T>& point2)
  {
    Point<N,T> temp(point1);
    temp -= point2;
    return temp.length();
  }

  template<PointDimType N, typename T>
  bool Point<N,T>::CompareX::operator ()  (const Point<N,T>& lhs, const Point<N,T>& rhs) const
  {
    return lhs[0] < rhs[0];
  }

  template<PointDimType N, typename T>
  bool Point<N,T>::CompareY::operator ()  (const Point<N,T>& lhs, const Point<N,T>& rhs) const
  {
    return lhs[1] < rhs[1];
  }

  template<PointDimType N, typename T>
  bool Point<N,T>::CompareZ::operator ()  (const Point<N,T>& lhs, const Point<N,T>& rhs) const
  {
    return lhs[2] < rhs[2];
  }
  
  template<PointDimType N, typename T>
  bool AreVectorsAligned(const Point<N,T>& point1, const Point<N,T>& point2, const Radians& angleThreshold)
  {
    Point<N,f32> unitVec1(point1);
    unitVec1.MakeUnitLength();
    
    Point<N,f32> unitVec2(point2);
    unitVec2.MakeUnitLength();
    
    return AreUnitVectorsAligned(unitVec1, unitVec2, angleThreshold);
  }
  
  template<PointDimType N>
  bool AreUnitVectorsAligned(const Point<N,f32>& unitVec1, const Point<N,f32>& unitVec2, const Radians& angleThreshold)
  {
    assert(NEAR(unitVec1.Length(), 1.f, 10.f*std::numeric_limits<f32>::epsilon()));
    assert(NEAR(unitVec2.Length(), 1.f, 10.f*std::numeric_limits<f32>::epsilon()));
    
    const f32 dotProduct = DotProduct(unitVec1, unitVec2);
    const f32 dotProductThreshold = 1.f - std::cos(angleThreshold.ToFloat());
    
    const bool areAligned = NEAR(std::abs(dotProduct), 1.f, dotProductThreshold);
    
    return areAligned;
  }
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_IMPL_H_
