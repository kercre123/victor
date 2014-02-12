/**
File: geometry.h
Author: Peter Barnum
Created: 2013

Definitions of geometry_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_POINT_H_
#define _ANKICORETECHEMBEDDED_COMMON_POINT_H_

#include "anki/common/robot/geometry_declarations.h"
#include "anki/common/robot/cInterfaces_c.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Point Definitions ---

    template<typename Type> Point<Type>::Point()
      : x(static_cast<Type>(0)), y(static_cast<Type>(0))
    {
    }

    template<typename Type> Point<Type>::Point(const Type x, const Type y)
      : x(x), y(y)
    {
    }

    template<typename Type> Point<Type>::Point(const Point<Type>& pt)
      : x(pt.x), y(pt.y)
    {
    }

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> Point<Type>::Point(const cv::Point_<Type>& pt)
      : x(pt.x), y(pt.y)
    {
    }
#endif

#if ANKICORETECH_EMBEDDED_USE_OPENCV
    template<typename Type> cv::Point_<Type> Point<Type>::get_CvPoint_()
    {
      return cv::Point_<Type>(x,y);
    }
#endif

    template<typename Type> void Point<Type>::Print() const
    {
      printf("(%d,%d) ", this->x, this->y);
    }

    template<typename Type> bool Point<Type>::operator== (const Point<Type> &point2) const
    {
      if(this->x == point2.x && this->y == point2.y)
        return true;

      return false;
    }

    template<typename Type> Point<Type> Point<Type>::operator+ (const Point<Type> &point2) const
    {
      return Point<Type>(this->x+point2.x, this->y+point2.y);
    }

    template<typename Type> Point<Type> Point<Type>::operator- (const Point<Type> &point2) const
    {
      return Point<Type>(this->x-point2.x, this->y-point2.y);
    }

    template<typename Type> Point<Type>& Point<Type>::operator*= (const Type value)
    {
      this->x *= value;
      this->y *= value;
      return *this;
    }
    
    template<typename Type> Point<Type>& Point<Type>::operator-= (const Type value)
    {
      this->x -= value;
      this->y -= value;
      return *this;
    }
    
    template<typename Type> Point<Type>& Point<Type>::operator-= (const Point<Type> &point2)
    {
      this->x -= point2.x;
      this->y -= point2.y;
      return *this;
    }

    template<typename Type> inline Point<Type>& Point<Type>::operator= (const Point<Type> &point2)
    {
      this->x = point2.x;
      this->y = point2.y;

      return *this;
    }

    template<typename Type> float Point<Type>::Dist(const Point<Type> &point2) const
    {
      return (float)sqrt((this->x - point2.x)*(this->x - point2.x) + (this->y - point2.y)*(this->y - point2.y));
    }
    
#pragma mark --- Point Specializations ---
    template<> void Point<f32>::Print() const;
    template<> void Point<f64>::Print() const;

#pragma mark --- Rectangle Definitions ---

    template<typename Type> Rectangle<Type>::Rectangle()
      : left(static_cast<Type>(0)), right(static_cast<Type>(0)), top(static_cast<Type>(0)), bottom(static_cast<Type>(0))
    {
    }

    template<typename Type> Rectangle<Type>::Rectangle(const Type left, const Type right, const Type top, const Type bottom)
      : left(left), right(right), top(top), bottom(bottom)
    {
    }

    template<typename Type> Rectangle<Type>::Rectangle(const Rectangle<Type>& rect)
      : left(rect.left), right(rect.right), top(rect.top), bottom(rect.bottom)
    {
    }

    template<typename Type> void Rectangle<Type>::Print() const
    {
      printf("(%d,%d)->(%d,%d) ", this->left, this->top, this->right, this->bottom);
    }

    template<typename Type> Point<Type> Rectangle<Type>::ComputeCenter() const
    {
      Point<Type> center((this->left+this->right)/2, (this->top+this->bottom)/2);
      return center;
    }

    template<typename Type> bool Rectangle<Type>::operator== (const Rectangle<Type> &rectangle2) const
    {
      if(this->left == rectangle2.left && this->top == rectangle2.top && this->right == rectangle2.right && this->bottom == rectangle2.bottom)
        return true;

      return false;
    }

    template<typename Type> Rectangle<Type> Rectangle<Type>::operator+ (const Rectangle<Type> &rectangle2) const
    {
      return Rectangle<Type>(this->top+rectangle2.top, this->bottom+rectangle2.bottom, this->left+rectangle2.left, this->right+rectangle2.right);
    }

    template<typename Type> Rectangle<Type> Rectangle<Type>::operator- (const Rectangle<Type> &rectangle2) const
    {
      return Rectangle<Type>(this->top-rectangle2.top, this->bottom-rectangle2.bottom, this->left-rectangle2.left, this->right-rectangle2.right);
    }

    template<typename Type> inline Rectangle<Type>& Rectangle<Type>::operator= (const Rectangle<Type> &rect2)
    {
      this->left = rect2.left;
      this->right = rect2.right;
      this->top = rect2.top;
      this->bottom = rect2.bottom;

      return *this;
    }

    template<typename Type> Type Rectangle<Type>::get_width() const
    {
      return right - left + 1;
    }

    template<typename Type> Type Rectangle<Type>::get_height() const
    {
      return bottom - top + 1;
    }

#pragma mark --- Rectangle Specializations ---
    template<> void Rectangle<f32>::Print() const;
    template<> void Rectangle<f64>::Print() const;

    template<> f32 Rectangle<f32>::get_width() const;
    template<> f64 Rectangle<f64>::get_width() const;

    template<> f32 Rectangle<f32>::get_height() const;
    template<> f64 Rectangle<f64>::get_height() const;

#pragma mark --- Quadrilateral Definitions ---

    template<typename Type> Quadrilateral<Type>::Quadrilateral()
    {
      for(s32 i=0; i<4; i++) {
        corners[i] = Point<Type>();
      }
    }

    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Point<Type> &corner1, const Point<Type> &corner2, const Point<Type> &corner3, const Point<Type> &corner4)
    {
      corners[0] = corner1;
      corners[1] = corner2;
      corners[2] = corner3;
      corners[3] = corner4;
    }

    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Quadrilateral<Type>& quad2)
    {
      for(s32 i=0; i<4; i++) {
        this->corners[i] = quad2.corners[i];
      }
    }
    
    template<typename Type> Quadrilateral<Type>::Quadrilateral(const Rectangle<Type>& rect)
    {
      this->corners[0].x = rect.left;   this->corners[0].y = rect.top;
      this->corners[1].x = rect.right;  this->corners[1].y = rect.top;
      this->corners[2].x = rect.left;   this->corners[2].y = rect.bottom;
      this->corners[3].x = rect.right;  this->corners[3].y = rect.bottom;
    }

    template<typename Type> void Quadrilateral<Type>::Print() const
    {
      printf("{(%d,%d), (%d,%d), (%d,%d), (%d,%d)} ",
        this->corners[0].x, this->corners[0].y,
        this->corners[1].x, this->corners[1].y,
        this->corners[2].x, this->corners[2].y,
        this->corners[3].x, this->corners[3].y);
    }

    template<typename Type> Point<Type> Quadrilateral<Type>::ComputeCenter() const
    {
      Point<Type> center(0, 0);

      for(s32 i=0; i<4; i++) {
        center.x += this->corners[i].x;
        center.y += this->corners[i].y;
      }

      center.x /= 4;
      center.y /= 4;

      return center;
    }
    
    template<typename Type> Rectangle<Type> Quadrilateral<Type>::ComputeBoundingRectangle() const
    {
      Rectangle<Type> boundingBox(this->corners[0].x,
                                  this->corners[0].x,
                                  this->corners[0].y,
                                  this->corners[0].y);

      // Initialize the template rectangle to the bounding box of the given
      // quadrilateral
      for(s32 i=1; i<4; ++i) {
        boundingBox.left   = MIN(boundingBox.left,   this->corners[i].x);
        boundingBox.right  = MAX(boundingBox.right,  this->corners[i].x);
        boundingBox.top    = MIN(boundingBox.top,    this->corners[i].y);
        boundingBox.bottom = MAX(boundingBox.bottom, this->corners[i].y);
      }
      
      return boundingBox;
    }

    template<typename Type> bool Quadrilateral<Type>::operator== (const Quadrilateral<Type> &quad2) const
    {
      for(s32 i=0; i<4; i++) {
        if(!(this->corners[i] == quad2.corners[i]))
          return false;
      }

      return true;
    }

    template<typename Type> Quadrilateral<Type> Quadrilateral<Type>::operator+ (const Quadrilateral<Type> &quad2) const
    {
      Quadrilateral<Type> newQuad;

      for(s32 i=0; i<4; i++) {
        newQuad.corners[i] = this->corners[i] + quad2.corners[i];
      }

      return newQuad;
    }

    template<typename Type> Quadrilateral<Type> Quadrilateral<Type>::operator- (const Quadrilateral<Type> &quad2) const
    {
      Quadrilateral<Type> newQuad;

      for(s32 i=0; i<4; i++) {
        newQuad.corners[i] = this->corners[i] - quad2.corners[i];
      }

      return newQuad;
    }

    template<typename Type> inline Quadrilateral<Type>& Quadrilateral<Type>::operator= (const Quadrilateral<Type> &quad2)
    {
      for(s32 i=0; i<4; i++) {
        this->corners[i] = quad2.corners[i];
      }

      return *this;
    }

    template<typename Type> inline const Point<Type>& Quadrilateral<Type>::operator[] (const s32 index) const
    {
      return corners[index];
    }

    template<typename Type> inline Point<Type>& Quadrilateral<Type>::operator[] (const s32 index)
    {
      return corners[index];
    }

#pragma mark --- Quadrilateral Specializations ---
    template<> void Quadrilateral<f32>::Print() const;
    template<> void Quadrilateral<f64>::Print() const;

#pragma mark --- C Conversions ---
    C_Rectangle_s16 get_C_Rectangle_s16(const Rectangle<s16> &rect);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_H_
