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

#include "anki/common/robot/config.h"
#include "anki/common/robot/cInterfaces_c.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Point Class Declaration ---
    // 2D Point Class:
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

      cv::Point_<Type> get_CvPoint_();
#endif

      void Print() const;

      bool operator== (const Point<Type> &point2) const;

      Point<Type> operator+ (const Point<Type> &point2) const;

      Point<Type> operator- (const Point<Type> &point2) const;

      void operator*= (const Type value);

      inline Point<Type>& operator= (const Point<Type> &point2);
    }; // class Point<Type>

#pragma mark --- Rectangle Class Declaration ---

    template<typename Type> class Rectangle
    {
    public:
      Type left;
      Type right;
      Type top;
      Type bottom;

      Rectangle();

      Rectangle(const Type left, const Type right, const Type top, const Type bottom);

      Rectangle(const Rectangle<Type>& rect);

      void Print() const;

      Point<Type> ComputeCenter() const;

      bool operator== (const Rectangle<Type> &rect2) const;

      Rectangle<Type> operator+ (const Rectangle<Type> &rect2) const;

      Rectangle<Type> operator- (const Rectangle<Type> &rect2) const;

      inline Rectangle<Type>& operator= (const Rectangle<Type> &rect2);

      Type get_width() const;

      Type get_height() const;
    }; // class Rectangle<Type>

#pragma mark --- Quadrilateral Class Declaration ---

    template<typename Type> class Quadrilateral
    {
    public:
      Point<Type> corners[4];

      Quadrilateral();

      Quadrilateral(const Point<Type> &corner1, const Point<Type> &corner2, const Point<Type> &corner3, const Point<Type> &corner4);

      Quadrilateral(const Quadrilateral<Type>& quad);

      void Print() const;

      Point<Type> ComputeCenter() const;

      bool operator== (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator+ (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator- (const Quadrilateral<Type> &quad2) const;

      inline Quadrilateral<Type>& operator= (const Quadrilateral<Type> &quad2);

      inline const Point<Type>& operator[] (const s32 index) const;
      inline Point<Type>& operator[] (const s32 index);
    }; // class Quadrilateral<Type>

#pragma mark --- C Conversions ---
    C_Rectangle_s16 get_C_Rectangle_s16(const Rectangle<s16> &rect);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_DECLARATIONS_H_