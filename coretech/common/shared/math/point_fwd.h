/**
 * File: point_fwd.h
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

#ifndef _ANKICORETECH_COMMON_POINT_FORWARD_H_
#define _ANKICORETECH_COMMON_POINT_FORWARD_H_

#include "coretech/common/shared/types.h"
#include "opencv2/core.hpp"

namespace Anki {
  
  // Forward declaration
  class Radians;
  template<MatDimType,MatDimType,typename> class SmallMatrix;
  
  using PointDimType = size_t;
  
  // Generic N-dimensional Point class
  template<PointDimType N, typename T>
  class Point : private std::array<T,N>
  {
    static_assert(N>0, "Cannot create an empty Point.");

  public:
    using std::array<T,N>::operator[];
    
    // Constructors
    // Populate all dimensions with the same scalar value
    constexpr Point(const T scalar = T(0));
  
    // Construct a point from a higher-dimensional point by just dropping the
    // the last dimensions. For example, construct a 2D point from a 3D point
    // by just using the (x,y) dimensions and ignoring z.
    template <PointDimType M, typename = std::enable_if_t<(M > N)>>
    constexpr Point(const Point<M,T>& pt);
    
    constexpr explicit Point(const SmallMatrix<N,1,T>& M);

    template <typename... Ts, typename = std::enable_if_t<sizeof...(Ts) == N> >
    constexpr Point(Ts... ts);
    
    // Assignment operator:
    Point<N,T>& operator=(const T &value);
    
    // Assign and cast
    template<typename T_other>
    Point<N,T>& SetCast(const Point<N,T_other> &other);
    template<typename T_other>
    Point<N,T>& SetCast(const T_other &value);
    
    // Remove implicit casting
    template<typename T_other>
    Point<N,T>& operator=(const T_other &value) = delete;
    template<typename T_other>
    Point<N,T>& operator=(const Point<N,T_other> &other) = delete;
    
    // Return a cast version of this
    template<typename T_other>
    constexpr Point<N, T_other> CastTo() const;

    // extract elements in inclusive range [A,B] from the current point into a smaller point
    // e.g.:
    //    Point<5,int> a{1,2,3,4,5};
    //    Point<3,int> b = a.Slice<1,3>();  // b == {2,3,4}
    template<PointDimType A, PointDimType B, typename = std::enable_if_t< B <= N && A <= N && A<B >>
    constexpr Point<B-A+1, T> Slice() const;
    
    // Special mnemonic accessors for the first, second,
    // and third elements, available when N is large enough.
    constexpr T& x();
    constexpr T& y();
    constexpr T& z();
    constexpr const T& x() const;
    constexpr const T& y() const;
    constexpr const T& z() const;
    
    explicit Point(const cv::Point_<T>& pt);
    explicit Point(const cv::Point3_<T>& pt);
    explicit Point(const cv::Vec<T,N>& vec);
    
    cv::Point_<T> get_CvPoint_() const;
    cv::Point3_<T> get_CvPoint3_() const;
    
    // Arithmetic Operators
    // TODO: make constexpr when we get C++ 17
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
    Point<N,T>  operator-  () const;
    
    // Comparison
    // NOTE: Comparison of *all* elements must be true for the operator to return true!!!
    //       See Any/All methods below to make this more explicit.
    constexpr bool operator< (const Point<N,T>& other) const; // all elements less than
    constexpr bool operator> (const Point<N,T>& other) const; // all elements greater than
    constexpr bool operator==(const Point<N,T>& other) const; // all elements equal
    constexpr bool operator!=(const Point<N,T>& other) const; // all elements equal
    constexpr bool operator<=(const Point<N,T>& other) const; // all elements less than or equal
    constexpr bool operator>=(const Point<N,T>& other) const; // all elements greater than or equal
    
    // Explicit Any vs. All comparison for choosing whether all elements or any element
    // has to satisfy the comparison.
    // GT = Greater Than, GTE = Greater Than or Equal, LT = Less Than, etc.
    constexpr bool AllGT(const Point<N,T>& other) const;
    constexpr bool AllLT(const Point<N,T>& other) const;
    constexpr bool AllGTE(const Point<N,T>& other) const;
    constexpr bool AllLTE(const Point<N,T>& other) const;

    constexpr bool AnyGT(const Point<N,T>& other)  const;
    constexpr bool AnyLT(const Point<N,T>& other)  const;
    constexpr bool AnyGTE(const Point<N,T>& other) const;
    constexpr bool AnyLTE(const Point<N,T>& other) const;
    
    // Absolute value of each element
    constexpr Point<N,T>  GetAbs() const;
    Point<N,T>& Abs(); // in place
    
    // Return length (squared) of the vector from the origin to the point
    constexpr T Length(void) const;
    constexpr T LengthSq(void) const;
    
    // Get Min/Max element (and optionally, which dimension)
    constexpr T GetMin(PointDimType* whichDim = nullptr) const;
    constexpr T GetMax(PointDimType* whichDim = nullptr) const;
    
    // Makes the point into a unit vector from the origin, while
    // returning its original length. IMPORTANT: if the point was
    // originally the origin, it cannot be made into a unit vector
    // and will be left at the origin, and zero will be returned.
    T MakeUnitLength(void);

    // Returns "(x, y, ...)"
    std::string ToString() const;
        
  }; // class Point
  
  // Create some convenience aliases/typedefs for 2D and 3D points:
  template <typename T>
  using Point2 = Point<2, T>;
  
  template <typename T>
  using Point3 = Point<3, T>;
  
  using Point2f = Point2<f32>;
  using Point3f = Point3<f32>;
  
  using Point2i = Point2<s32>;
  using Point3i = Point3<s32>;
  
  using Vec2f = Point2f;
  using Vec3f = Point3f;
  
  const Vec2f& X_AXIS_2D();
  const Vec2f& Y_AXIS_2D();
  
  const Vec3f& X_AXIS_3D();
  const Vec3f& Y_AXIS_3D();
  const Vec3f& Z_AXIS_3D();

  enum class AxisName {
    Z_NEG = -3,
    Y_NEG = -2,
    X_NEG = -1,
    X_POS =  1,
    Y_POS =  2,
    Z_POS =  3
  };

  // returns single character axis for AxisName (ignoring sign)
  constexpr char AxisToChar(AxisName axis);

  // returns +/- N_AXIS_3D() where N = {X,Y,Z}
  constexpr Vec3f AxisToVec3f(AxisName axis);
  
  // returns two character string for AxisName (e.g. "+X" or "-Z")
  const char* AxisToCString(AxisName axis);

  // Helper for compile-time conversion from character axis ('X', 'Y', or 'Z')
  // to index (0, 1, or 2, respectively). Any other character for AXIS will fail
  // to compile.
  template<char AXIS> 
  constexpr s32 AxisToIndex();
  
  template<> inline 
  constexpr s32 AxisToIndex<'X'>() { return 0; }
  template<> inline 
  constexpr s32 AxisToIndex<'Y'>() { return 1; }
  template<> inline 
  constexpr s32 AxisToIndex<'Z'>() { return 2; }

  constexpr s32 AxisToIndex(AxisName axis)
  {
    switch(axis)
    {
      case AxisName::X_POS:
      case AxisName::X_NEG:
        return AxisToIndex<'X'>();
        
      case AxisName::Y_POS:
      case AxisName::Y_NEG:
        return AxisToIndex<'Y'>();
        
      case AxisName::Z_POS:
      case AxisName::Z_NEG:
        return AxisToIndex<'Z'>();
    }
  }
  
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
  constexpr std::ostream& operator<<(std::ostream& out, const Point<N,T>& p);
  
  // Binary mathematical operations:
  template<PointDimType N, typename T>
  constexpr bool operator== (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<PointDimType N, typename T>
  constexpr bool operator!= (const Point<N,T> &point1, const Point<N,T> &point2);

  template<PointDimType N, typename T>
  constexpr bool IsNearlyEqual(const Point<N,T> &point1, const Point<N,T> &point2,
                               const T eps = T(10)*std::numeric_limits<T>::epsilon());
  
  // TODO: make arithemetic operations constexpr once constexpr lambdas are enabled in C++17
  template<PointDimType N, typename T>
  Point<N,T> operator+ (const Point<N,T> &point1, const Point<N,T> &point2);
  
  template<PointDimType N, typename T>
  Point<N,T> operator- (const Point<N,T> &point1, const Point<N,T> &point2);

  // append point2 onto the end of point1, making a new point of size Dim(p1) + Dim(p2)
  template<PointDimType M, PointDimType N, typename T>
  constexpr Point<M+N,T> Join(const Point<M,T>& point1, const Point<N,T>& point2);
  
  template<PointDimType N, typename T>
  constexpr T DotProduct(const Point<N,T> &point1, const Point<N,T> &point2);

  template<typename T>
  Point3<T> CrossProduct(const Point3<T> &point1, const Point3<T> &point2);
  
  // TODO: should output type always be float/double?
  template<PointDimType N, typename T>
  constexpr T ComputeDistanceBetween(const Point<N,T>& point1, const Point<N,T>& point2);
  
  // Return true if the two points (vectors) are aligned within the given angle threshold
  template<PointDimType N, typename T>
  constexpr bool AreVectorsAligned(const Point<N,T>& point1, const Point<N,T>& point2, const Radians& angleThreshold);
  
  // Return true if the two unit vectors are aligned within the given angle threshold
  // NOTE: In DEBUG, the unit vectors' lengths are confirmed (i.e. asserted) to be 1.0, but not in RELEASE.
  template<PointDimType N>
  constexpr bool AreUnitVectorsAligned(const Point<N,f32>& unitVec1, const Point<N,f32>& unitVec2, const Radians& angleThreshold);
  
} // namespace Anki

#endif // _ANKICORETECH_COMMON_POINT_H_
