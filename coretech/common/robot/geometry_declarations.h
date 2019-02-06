/**
File: geometry_declarations.h
Author: Peter Barnum
Created: 2013

Simply geometry classes for points, rectangles, etc.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_POINT_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_POINT_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/shared/math/radians.h"
#include "coretech/common/robot/memory.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    // Forward declaration of Array class (for Matrix x Point multiplication)
    template<typename Type> class Array;

#if 0
#pragma mark --- 2D Point Class Declaration ---
#endif
    // 2D Point Class
    //
    // WARNING:
    // The coordinate order of Array objects is (y,x), while Point objects are (x,y)
    template<typename Type> class Point
    {
    public:
      Type x;
      Type y;

      Point();

      Point(const Type x, const Type y);

      Point(const Point<Type>& pt);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      Point(const cv::Point_<Type>& pt);

      cv::Point_<Type> get_CvPoint_() const;
#endif

      // Read in the input, then cast it to this object's type
      //
      // WARNING:
      // This should be kept explicit, to prevent accidental casting between different datatypes.
      template<typename InType> void SetCast(const Point<InType> &in);

      void Print() const;

      bool operator== (const Point<Type> &point2) const;

      Point<Type> operator+ (const Point<Type> &point2) const;

      Point<Type> operator- (const Point<Type> &point2) const;

      Point<Type> operator- () const;

      Point<Type>& operator*= (const Type value);
      Point<Type>& operator-= (const Type value);
      Point<Type>& operator+= (const Point<Type> &point2);
      Point<Type>& operator-= (const Point<Type> &point2);

      Point<Type>& operator= (const Point<Type> &point2);

      // The L2 (Euclidian) distance between this point and an input point.
      f32 Dist(const Point<Type> &point2) const;

      f32 Length() const;
    }; // class Point<Type>
    
#if 0
#pragma mark --- 3D Point Class Declaration ---
#endif
    // 3D Point Class
    //
    template<typename Type> class Point3
    {
    public:
      Type x;
      Type y;
      Type z;

      Point3();

      Point3(const Type x, const Type y, const Type z);

      Point3(const Point3<Type>& pt);

#if ANKICORETECH_EMBEDDED_USE_OPENCV
      Point3(const cv::Point3_<Type>& pt);

      cv::Point3_<Type> get_CvPoint_() const;
#endif

      void Print() const;

      bool operator== (const Point3<Type> &point2) const;

      Point3<Type> operator+ (const Point3<Type> &point2) const;

      Point3<Type> operator- (const Point3<Type> &point2) const;

      Point3<Type> operator- () const;

      Point3<Type>& operator*= (const Type value);
      Point3<Type>& operator-= (const Type value);
      Point3<Type>& operator-= (const Point3<Type> &point2);

      Point3<Type>& operator= (const Point3<Type> &point2);

      // The L2 (Euclidian) distance between this point and an input point.
      f32 Dist(const Point3<Type> &point2) const;

      f32 Length() const;

      // Normalize to be unit length (divide each element by Length())
      // The original Length is returned and if it was 0, the Point will
      // remain unchanged.
      f32 MakeUnitLength();
    }; // class Point3<Type>

    template<typename Type>
    Type DotProduct(const Point3<Type>& point1, const Point3<Type>& point2);

    template<typename Type>
    Point3<Type> CrossProduct(const Point3<Type>& point1, const Point3<Type>& point2);

    // Matrix x Point multiplication.  Input matrix M must be 3x3!
    template<typename Type>
    Point3<Type> operator* (const Array<Type>& M, const Point3<Type>& p);

#if 0
#pragma mark --- Rectangle Class Declaration ---
#endif

    // A rectangle is bounded by a left, right, top, and bottom
    template<typename Type> class Rectangle
    {
    public:
      Type left;
      Type right;
      Type top;
      Type bottom;

      Rectangle();

      // The left, right, top, and bottom coordinates are the coordinates of the cracks between the pixels.
      // The top-left pixel's center is at (.5, .5)
      // WARNING: The right value should not be the index of the right pixel, but one more "rightPixelIndex + 1" (i.e. the value of the edge to the right of the pixel). Likewise for the bottom value.
      Rectangle(const Type left, const Type right, const Type top, const Type bottom);

      Rectangle(const Rectangle<Type>& rect);

      void Print() const;

      template<typename OutType> Point<OutType> ComputeCenter() const;

      // If scalePercent is less-than 1.0, the rectangle is shrunk around its center
      // If scalePercent is greater-than 1.0, the rectangle is expanded around its center
      template<typename OutType> Rectangle<OutType> ComputeScaledRectangle(const f32 scalePercent) const;

      bool operator== (const Rectangle<Type> &rect2) const;

      Rectangle<Type> operator+ (const Rectangle<Type> &rect2) const;

      Rectangle<Type> operator- (const Rectangle<Type> &rect2) const;

      inline Rectangle<Type>& operator= (const Rectangle<Type> &rect2);

      // right - left
      Type get_width() const;

      // bottom - top
      Type get_height() const;
    }; // class Rectangle<Type>

    // Compute the difference between two 3D poses (Rotations+Translations)
    // All R matrices should be 3x3.
    //    [R2 T2] = [R1 T1] * [Rdiff Tdiff]
    // I.e., the diff pose is the one that takes pose 1 to pose 2.
    template<typename Type>
    Result ComputePoseDiff(const Array<Type>& R1, const Point3<Type>& T1,
      const Array<Type>& R2, const Point3<Type>& T2,
      Array<Type>& Rdiff, Point3<Type>& Tdiff,
      MemoryStack scratch);

#if 0
#pragma mark --- Quadrilateral Class Declaration ---
#endif
    // A Quadrilateral is defined by four Point objects
    template<typename Type> class Quadrilateral
    {
    public:
      enum CornerName {
        FirstCorner = 0,
        TopLeft     = 0,
        BottomLeft  = 1,
        TopRight    = 2,
        BottomRight = 3,
        NumCorners  = 4
      };

      Point<Type> corners[4];

      Quadrilateral();

      Quadrilateral(const Point<Type> &corner1, const Point<Type> &corner2, const Point<Type> &corner3, const Point<Type> &corner4);

      Quadrilateral(const Quadrilateral<Type>& quad);

      Quadrilateral(const Rectangle<Type>& rect);

      void Print() const;

      template<typename OutType> Point<OutType> ComputeCenter() const;

      // WARNING:
      // The width and height of a floating point Rectangle is different than that of an integer rectangle.
      template<typename OutType> Rectangle<OutType> ComputeBoundingRectangle() const;

      // Returns a copy of this Quadrilateral with sorted corners, so they are clockwise around the centroid
      // Warning: This may give weird results for non-convex quadrilaterals
      template<typename OutType> Quadrilateral<OutType> ComputeClockwiseCorners() const;

      template<typename OutType> Quadrilateral<OutType> ComputeRotatedCorners(const f32 radians) const;

      bool IsConvex() const;

      bool operator== (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator+ (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator- (const Quadrilateral<Type> &quad2) const;

      inline Quadrilateral<Type>& operator= (const Quadrilateral<Type> &quad2);

      // Keeping this explicit to avoid accidental setting of quads of
      // different types
      template<typename InType> void SetCast(const Quadrilateral<InType> &quad2);

      inline const Point<Type>& operator[] (const s32 index) const;
      inline Point<Type>& operator[] (const s32 index);
    }; // class Quadrilateral<Type>
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_DECLARATIONS_H_
