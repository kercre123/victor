/**
 * File: point.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/10/2013
 *
 *
 * Description: Defines a general N-dimensional Point class and two 
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

#include "anki/common/types.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

#include <array>

namespace Anki {
  
  // Forward declaration
  class Radians;
  
  using PointDimType = size_t;
  
  // Generic N-dimensional Point class
  template<PointDimType N, typename T>
  class Point //: public std::array<T,N>
  {
    static_assert(N>0, "Cannot create an empty Point.");
    
  public:
    
    // Constructors
    Point( void );
    
    template<typename T_other>
    Point(const Point<N,T_other>& pt);
    //Point(const std::array<T,N>& array); // Creates ambiguity with opencv Point3_ constructor below
    
    // Populate all dimensions with the same scalar value
    Point(const T scalar);
    
    // Construct a point from a higher-dimensional point by just dropping the
    // the last dimensions. For example, construct a 2D point from a 3D point
    // by just using the (x,y) dimensions and ignoring z.
    Point(const Point<N+1,T>& pt);

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
    : data{{ head, T(tail)... }} {}
#else
#warning No variadic templates.
    Point(const T x, const T y); // Only valid if N==2
    Point(const T x, const T y, const T z); // Only valid if N==3
#endif
    
    // Assignment operator:
    template<typename T_other>
    Point<N,T>& operator=(const Point<N,T_other> &other);
    Point<N,T>& operator=(const T &value);
    
    // Accessors:
    T& operator[] (const PointDimType i);
    const T& operator[] (const PointDimType i) const;
    
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
    Point<N,T>& operator/= (const T value);
    Point<N,T>  operator*  (const T value) const;
    Point<N,T>  operator+  (const T value) const;
    Point<N,T>& operator+= (const Point<N,T> &other);
    Point<N,T>& operator-= (const Point<N,T> &other);
    Point<N,T>& operator*= (const Point<N,T> &other);
    Point<N,T>& operator/= (const Point<N,T> &other);
    Point<N,T>  operator-() const;
    
    // Comparison
    bool operator< (const Point<N,T>& other) const; // all elements less than
    bool operator> (const Point<N,T>& other) const; // all elements greater than
    bool operator==(const Point<N,T>& other) const; // all elements equal
    
    // Absolute value of each element
    Point<N,T>  GetAbs() const;
    Point<N,T>& Abs(); // in place
    
    // Return length (squared) of the vector from the origin to the point
    T Length(void) const;
    T LengthSq(void) const;
    
    // Makes the point into a unit vector from the origin, while
    // returning its original length. IMPORTANT: if the point was
    // originally the origin, it cannot be made into a unit vector
    // and will be left at the origin, and zero will be returned.
    T MakeUnitLength(void);

    // comparison object to compare the a single dimension
    struct CompareX {
      bool operator() (const Point<N,T>& lhs, const Point<N,T>& rhs) const;
    };
    struct CompareY {
      bool operator() (const Point<N,T>& lhs, const Point<N,T>& rhs) const;
    };
    struct CompareZ {
      bool operator() (const Point<N,T>& lhs, const Point<N,T>& rhs) const;
    };
    
  protected:
    
    std::array<T,N> data;
    
  }; // class Point
  
  // Create some convenience aliases/typedefs for 2D and 3D points:
  template <typename T>
  using Point2 = Point<2, T>;
  
  template <typename T>
  using Point3 = Point<3, T>;
  
  using Point2f = Point2<f32>;
  using Point3f = Point3<f32>;
  
  using Vec2f = Point2f;
  using Vec3f = Point3f;
  
  const Vec2f& X_AXIS_2D();
  const Vec2f& Y_AXIS_2D();
  
  const Vec3f& X_AXIS_3D();
  const Vec3f& Y_AXIS_3D();
  const Vec3f& Z_AXIS_3D();

  
  enum Axis {
    Z_AXIS_NEG = -3,
    Y_AXIS_NEG = -2,
    X_AXIS_NEG = -1,
    X_AXIS_POS =  1,
    Y_AXIS_POS =  2,
    Z_AXIS_POS =  3
  };

  // Helper for compile-time conversion from character axis ('X', 'Y', or 'Z')
  // to index (0, 1, or 2, respectively). Any other character for AXIS will fail
  // to compile.
  template<char AXIS> s32 AxisToIndex();
  
  template<> inline s32 AxisToIndex<'X'>() { return 0; }
  template<> inline s32 AxisToIndex<'Y'>() { return 1; }
  template<> inline s32 AxisToIndex<'Z'>() { return 2; }

  /*
  template<PointDimType N, typename T>
  class UnitVector : public Point<N,T>
  {
    template <typename... Tail>
    UnitVector(typename std::enable_if<sizeof...(Tail)+1 == N, T>::type head, Tail... tail)
    : Point<N,T>(head, T(tail)...)
    {
      this->makeUnitLength();
    }
    
    // All setters call superclass set function and then renormalize?
    
    // Always returns one:
    T length(void) const { return T(1); };
    
  }; // class UnitVector
  */
  
  /*
   // TODO: Do need a separate Vec class?
  template<PointDimType N, typename T>
  class Vec : public Point<N,T>
  {
    
  }; // class Vec
  
  template<typename T>
  using Vec2 = Vec<2,T>;
  
  template<typename T>
  using Vec3 = Vec<3,T>;
  */
  
  // Display / Logging:
  template<PointDimType N, typename T>
  std::ostream& operator<<(std::ostream& out, const Point<N,T>& p);
  
  // Binary mathematical operations:
  template<PointDimType N, typename T>
  bool operator== (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<PointDimType N, typename T>
  bool IsNearlyEqual(const Point<N,T> &point1, const Point<N,T> &point2,
                   const T eps = T(10)*std::numeric_limits<T>::epsilon());
  
  template<PointDimType N, typename T>
  Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<PointDimType N, typename T>
  Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<PointDimType N, typename T>
  T DotProduct(const Point<N,T> &point1, const Point<N,T> &point2);

  template<typename T>
  Point3<T> CrossProduct(const Point3<T> &point1, const Point3<T> &point2);
  
  // TODO: should output type always be float/double?
  template<PointDimType N, typename T>
  T ComputeDistanceBetween(const Point<N,T>& point1, const Point<N,T>& point2);
  
  // Return true if the two points (vectors) are aligned within the given angle threshold
  template<PointDimType N, typename T>
  bool AreVectorsAligned(const Point<N,T>& point1, const Point<N,T>& point2, const Radians& angleThreshold);
  
  // Return true if the two unit vectors are aligned within the given angle threshold
  // NOTE: In DEBUG, the unit vectors' lengths are confirmed (i.e. asserted) to be 1.0, but not in RELEASE.
  template<PointDimType N>
  bool AreUnitVectorsAligned(const Point<N,f32>& unitVec1, const Point<N,f32>& unitVec2, const Radians& angleThreshold);
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_H_
