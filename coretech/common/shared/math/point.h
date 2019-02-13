/**
 * File: point.h
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

#ifndef _ANKICORETECH_COMMON_POINT_H_
#define _ANKICORETECH_COMMON_POINT_H_

#include "coretech/common/shared/math/point_fwd.h"
#include "coretech/common/shared/math/radians.h"

#include "util/math/math.h"

#include <cmath>
#include <functional>
#include <numeric>

namespace Anki {
  // Returns true if all elements of p1 match p2 according to compareFcn
  template<PointDimType N, typename T, class Compare>
  inline constexpr bool AllHelper(const Point<N,T>& p1, const Point<N,T>& p2, Compare&& compareFcn)
  {
    for(PointDimType i=0; i<N; ++i)
    {
      // Return false as soon as any member fails
      if(!compareFcn(p1[i], p2[i]))
      {
        return false;
      }
    }
    
    // Nothing failed check, so all must be true
    return true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// STL specializations for common template comparison types 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace std {
  
// if type T is 32 bit or less, then we can hash with a bitshift
template <typename T> 
struct std::hash<Anki::Point2<T>> {
  std::enable_if_t<sizeof(T) <= 4, s64> 
  constexpr operator()(const Anki::Point2<T>& p) const
  { 
    return ((s64) p.x()) << 32 | ((s64) p.y()); 
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::equal_to<Anki::Point<N,T>>  {
  template <typename T1, typename = std::enable_if_t<std::is_floating_point<T1>::value> >
  constexpr bool operator()(const Anki::Point<N,T1>& p, const Anki::Point<N,T1>& q) const 
  { 
    return AllHelper(p, q, [] (T1 a, T1 b) { return FLT_NEAR(a, b); });
  }

  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return AllHelper(p, q, std::equal_to<T>());
  }
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::not_equal_to<Anki::Point<N,T>>  {
  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return !std::equal_to<Anki::Point<N,T>>()(p,q);
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::less<Anki::Point<N,T>>  {
  template <typename T1, typename = std::enable_if_t<std::is_floating_point<T1>::value> >
  constexpr bool operator()(const Anki::Point<N,T1>& p, const Anki::Point<N,T1>& q) const 
  { 
    return AllHelper(p, q, [] (T1 a, T1 b) { return FLT_LT(a, b); });
  }

  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return AllHelper(p, q, std::less<T>());
  }
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::less_equal<Anki::Point<N,T>>  {
  template <typename T1, typename = std::enable_if_t<std::is_floating_point<T1>::value> >
  constexpr bool operator()(const Anki::Point<N,T1>& p, const Anki::Point<N,T1>& q) const 
  { 
    return AllHelper(p, q, [] (T1 a, T1 b) { return FLT_LTE(a, b); });
  }

  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return AllHelper(p, q, std::less_equal<T>());
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::greater<Anki::Point<N,T>>  {
  template <typename T1, typename = std::enable_if_t<std::is_floating_point<T1>::value> >
  constexpr bool operator()(const Anki::Point<N,T1>& p, const Anki::Point<N,T1>& q) const 
  { 
    return AllHelper(p, q, [] (T1 a, T1 b) { return FLT_GT(a, b); });
  }

  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return AllHelper(p, q, std::greater<T>());
  }
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <Anki::PointDimType N, typename T>
struct std::greater_equal<Anki::Point<N,T>>  {
  template <typename T1, typename = std::enable_if_t<std::is_floating_point<T1>::value> >
  constexpr bool operator()(const Anki::Point<N,T1>& p, const Anki::Point<N,T1>& q) const 
  { 
    return AllHelper(p, q, [] (T1 a, T1 b) { return FLT_GTE(a, b); });
  }

  constexpr bool operator()(const Anki::Point<N,T>& p, const Anki::Point<N,T>& q) const 
  { 
    return AllHelper(p, q, std::greater_equal<T>());
  }
};

}

namespace {
  // NOTE: since `std::fill` is not constexpr until C++20, use an index_sequence to create 
  //       pack expansion for an initializer list
  template <Anki::PointDimType N, typename T, size_t... Is>
  constexpr Anki::Point<N,T> fill_point(const T& init, std::index_sequence<Is...> const&) {
    // use `Is` to duplicate `init`, but cast `Is` to void to ignore it
    return Anki::Point<N,T>{ ((void)Is, init)...}; 
  }
    
  template <Anki::PointDimType N, typename T>
  constexpr Anki::Point<N,T> fill_point(const T& init) {
    return fill_point<N,T>( init, std::make_index_sequence<N>{} );
  }

  template <Anki::PointDimType N, typename T, typename Generator, size_t... Is>
  constexpr Anki::Point<N,T> generate_point(const Generator& gen, std::index_sequence<Is...> const&) {
    return Anki::Point<N,T>{ gen[Is]... };
  }
    
  template <Anki::PointDimType N, typename T, typename Generator>
  constexpr Anki::Point<N,T> generate_point(const Generator& gen) {
    return generate_point<N,T>( gen, std::make_index_sequence<N>{} );
  }
}

namespace Anki {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Constructors
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr Point<N,T>::Point(const T scalar) 
: Point<N,T>( fill_point<N,T>(scalar) ) {} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <PointDimType N, typename T>
template <PointDimType M, typename>
constexpr Point<N,T>::Point(const Point<M,T>& pt) 
: Point<N,T>( generate_point<N,T>(pt) ) {} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <PointDimType N, typename T>
template <typename... Ts, typename>
constexpr  Point<N,T>::Point(Ts... ts)
: std::array<T,N>{{ T(ts)... }} {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr Point<N,T>::Point(const SmallMatrix<N,1,T>& M)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] = M((s32)i,0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>::Point(const cv::Point_<T>& pt)
: Point(pt.x, pt.y) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>::Point(const cv::Point3_<T>& pt)
: Point(pt.x, pt.y, pt.z) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>::Point(const cv::Vec<T, N>& vec)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] = vec[(s32)i];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Type Converters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
cv::Point_<T> Point<N,T>::get_CvPoint_() const
{
  static_assert(N==2, "N must be 2 to convert to cv::Point_<T>.");
  return cv::Point_<T>(this->x(), this->y());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
cv::Point3_<T> Point<N,T>::get_CvPoint3_() const
{
  static_assert(N==3, "N must be 3 to convert to cv::Point3_<T>.");
  return cv::Point3_<T>(this->x(), this->y(), this->z());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
template<typename T_other>
Point<N,T>& Point<N,T>::SetCast(const Point<N,T_other> &other)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] = static_cast<T>(other[i]);
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
template<typename T_other>
Point<N,T>& Point<N,T>::SetCast(const T_other &value)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] = static_cast<T>(value);
  }
  return *this;
}// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
template<typename T_other>
constexpr Point<N, T_other> Point<N,T>::CastTo() const
{
  Point<N, T_other> to_cast;
  to_cast.SetCast(*this);
  return to_cast;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
template<PointDimType A, PointDimType B, typename>
constexpr Point<B-A+1,T> Point<N,T>::Slice() const
{
  struct {    // use a struct because lambdas are not constexpr in C++14
    const Point<N,T>& p;
    constexpr const T& operator[](size_t i) const { return p[i+A]; }
  } slicer{*this};

  return generate_point<B-A+1,T>(slicer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType M, PointDimType N, typename T>
constexpr Point<M+N,T> Join(const Point<M,T>& point1, const Point<N,T>& point2)
{
  struct {    // use a struct because lambdas are not constexpr in C++14
    const Point<M,T> &p1;
    const Point<N,T> &p2;
    constexpr const T& operator[](size_t i) const { return (i<M) ? p1[i] : p2[i-M]; }
  } joinedArr{point1, point2};

  return generate_point<M+N,T>(joinedArr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Convenience Getters
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T& Point<N,T>::x()             
{ 
  return std::get<0>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T& Point<N,T>::y()             
{ 
  return std::get<1>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T& Point<N,T>::z()             
{ 
  return std::get<2>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr const T& Point<N,T>::x() const 
{ 
  return std::get<0>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr const T& Point<N,T>::y() const 
{ 
  return std::get<1>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr const T& Point<N,T>::z() const 
{ 
  return std::get<2>(*this); 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Arithmetic Operations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator=(const T &value)
{
  std::fill(this->begin(), this->end(), value);
  return (*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator*= (const T value)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] *= value;
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator+= (const T value)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] += value;
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator-= (const T value)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] -= value;
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator/= (const T value)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] /= value;
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T> Point<N,T>::operator* (const T value) const
{
  Point<N,T> res(*this);
  for(PointDimType i=0; i<N; ++i) {
    res[i] *= value;
  }
  return res;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T> Point<N,T>::operator+ (const T value) const
{
  Point<N,T> res(*this);
  for(PointDimType i=0; i<N; ++i) {
    res[i] += value;
  }
  return res;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator+= (const Point<N,T> &other)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] += other[i];
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator-= (const Point<N,T> &other)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] -= other[i];
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator*= (const Point<N,T> &other)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] *= other[i];
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::operator/= (const Point<N,T> &other)
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] /= other[i];
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T> Point<N,T>::operator-() const
{
  Point<N,T> p;
  for(PointDimType i=0; i<N; ++i) {
    p[i] = -(*this)[i];
  }
  return p;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2)
{
  Point<N,T> newPoint(point1);
  newPoint += point2;
  return newPoint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2)
{
  Point<N,T> newPoint(point1);
  newPoint -= point2;
  return newPoint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T DotProduct(const Point<N,T> &point1, const Point<N,T> &point2)
{
  float dotProduct = point1[0]*point2[0];
  for(PointDimType i=1; i<N; ++i) {
    dotProduct += point1[i]*point2[i];
  }
  return dotProduct;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
Point3<T> CrossProduct(const Point3<T> &point1, const Point3<T> &point2)
{
  return Point3<T>(point1.y()*point2.z() - point2.y()*point1.z(),
                   point2.x()*point1.z() - point1.x()*point2.z(),
                   point1.x()*point2.y() - point2.x()*point1.y());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T ComputeDistanceBetween(const Point<N,T>& point1, const Point<N,T>& point2)
{
  return (point1 - point2).Length();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
Point<N,T>& Point<N,T>::Abs()
{
  for(PointDimType i=0; i<N; ++i) {
    (*this)[i] = std::abs((*this)[i]);
  }
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
T Point<N,T>::MakeUnitLength(void)
{
  const T lengthSq = this->LengthSq();
  if(lengthSq > 0) {
    const T length = std::sqrt(lengthSq);
    (*this) *= T(1)/length;
    return length;
  } else {
    return T(0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Comparison Operators
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator< (const Point<N,T>& other) const
{
  return std::less<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator> (const Point<N,T>& other) const
{
  return std::greater<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator<= (const Point<N,T>& other) const
{
  return other;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator>= (const Point<N,T>& other) const
{
  return std::greater_equal<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator==(const Point<N,T>& other) const
{
  return std::equal_to<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool operator== (const Point<N,T> &point1, const Point<N,T> &point2)
{
  return std::equal_to<Point<N,T>>()(point1, point2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::operator!=(const Point<N,T>& other) const
{
  return std::not_equal_to<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool operator!= (const Point<N,T> &point1, const Point<N,T> &point2)
{
  return std::not_equal_to<Point<N,T>>()(point1, point2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AnyGT(const Point<N,T>& other) const
{
  return !AllLTE(other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AnyLT(const Point<N,T>& other) const
{
  return !AllGTE(other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AnyLTE(const Point<N,T>& other) const
{
  return !AllGT(other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AnyGTE(const Point<N,T>& other) const
{
  return !AllLT(other);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AllGT(const Point<N,T>& other) const
{
  return std::greater<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AllLT(const Point<N,T>& other) const
{
  return std::less<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AllGTE(const Point<N,T>& other) const
{
  return std::greater_equal<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool Point<N,T>::AllLTE(const Point<N,T>& other) const
{
  return std::less_equal<Point<N,T>>()(*this, other);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool IsNearlyEqual (const Point<N,T> &point1, const Point<N,T> &point2, const T eps)
{
  auto near = [eps](T value1, T value2) -> bool
  {
    return NEAR(value1, value2, eps);
  };
  
  return AllHelper(point1, point2, near);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Query
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T Point<N,T>::LengthSq(void) const
{
  T retVal = (*this)[0]*(*this)[0];
  for(PointDimType i=1; i<N; ++i) {
    retVal += (*this)[i]*(*this)[i];
  }
  
  return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T Point<N,T>::Length(void) const
{
  return std::sqrt(LengthSq());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr Point<N,T> Point<N,T>::GetAbs() const
{
  Point<N,T> copy(*this);
  return copy.Abs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T Point<N,T>::GetMin(PointDimType* whichDim) const
{
  PointDimType minDim = 0;
  T retVal = (*this)[minDim];
  for(PointDimType i=1 ; i<N; ++i) {
    if((*this)[i] < retVal) {
      retVal = (*this)[i];
      minDim = i;
    }
  }
  if(nullptr != whichDim) {
    *whichDim = minDim;
  }
  return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr T Point<N,T>::GetMax(PointDimType* whichDim) const
{
  PointDimType maxDim = 0;
  T retVal = t(*this)[maxDim];
  for(PointDimType i=1 ; i<N; ++i) {
    if((*this)[i] > retVal) {
      retVal = (*this)[i];
      maxDim = i;
    }
  }
  if(nullptr != whichDim) {
    *whichDim = maxDim;
  }
  return retVal;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr bool AreVectorsAligned(const Point<N,T>& point1, const Point<N,T>& point2, const Radians& angleThreshold)
{
  Point<N,f32> unitVec1(point1);
  unitVec1.MakeUnitLength();
  
  Point<N,f32> unitVec2(point2);
  unitVec2.MakeUnitLength();
  
  return AreUnitVectorsAligned(unitVec1, unitVec2, angleThreshold);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N>
constexpr bool AreUnitVectorsAligned(const Point<N,f32>& unitVec1, const Point<N,f32>& unitVec2, const Radians& angleThreshold)
{
  assert(NEAR(unitVec1.Length(), 1.f, 10.f*std::numeric_limits<f32>::epsilon()));
  assert(NEAR(unitVec2.Length(), 1.f, 10.f*std::numeric_limits<f32>::epsilon()));
  
  const f32 dotProduct = DotProduct(unitVec1, unitVec2);
  const f32 dotProductThreshold = 1.f - std::cos(angleThreshold.ToFloat());
  
  const bool areAligned = NEAR(std::abs(dotProduct), 1.f, dotProductThreshold);
  
  return areAligned;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// String Operations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
std::string Point<N,T>::ToString() const
{
  // Convert all but the last element to avoid a trailing ","
  static_assert(N>0, "Point must not be empty");
  std::ostringstream oss;
  oss << "(";
  std::copy(this->begin(), this->end()-1, std::ostream_iterator<T>(oss, ", "));
  
  // Now add the last element with no delimiter
  oss << this->back() << ")";
  
  return oss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<PointDimType N, typename T>
constexpr std::ostream& operator<<(std::ostream& out, const Point<N,T>& p)
{
  for (PointDimType i=0; i<N; ++i) {
    out << p[i] << " ";
  }
  return out;
}
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_IMPL_H_
