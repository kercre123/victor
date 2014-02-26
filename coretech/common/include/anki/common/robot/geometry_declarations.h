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
#include "anki/common/shared/radians.h"

#if ANKICORETECH_EMBEDDED_USE_OPENCV
#include "opencv2/core/core.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    // #pragma mark --- Point Class Declaration ---
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

      cv::Point_<Type> get_CvPoint_();
#endif

      void Print() const;

      bool operator== (const Point<Type> &point2) const;

      Point<Type> operator+ (const Point<Type> &point2) const;

      Point<Type> operator- (const Point<Type> &point2) const;

      Point<Type>& operator*= (const Type value);
      Point<Type>& operator-= (const Type value);
      Point<Type>& operator-= (const Point<Type> &point2);

      inline Point<Type>& operator= (const Point<Type> &point2);

      // The L2 (Euclidian) distance between this point and an input point.
      f32 Dist(const Point<Type> &point2) const;
    }; // class Point<Type>

    // #pragma mark --- Rectangle Class Declaration ---

    // A rectangle is bounded by a left, right, top, and bottom
    //
    // WARNING:
    // The width and height of a floating point Rectangle is different than that of an integer rectangle.
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

      // WARNING:
      // The width and height of a floating point Rectangle is different than that of an integer rectangle.
      Type get_width() const;

      // WARNING:
      // The width and height of a floating point Rectangle is different than that of an integer rectangle.
      Type get_height() const;
    }; // class Rectangle<Type>
    
    typedef Point<float> Point2f;

    
    // #pragma mark --- Pose2d Class Declaration ---

		class Pose2d
		{
		public:
			// Constructors:
			Pose2d() : coord(0,0), angle(0) {}
			Pose2d(const float x, const float y, const Radians angle) : coord(x,y), angle(angle) {};
			Pose2d(const Pose2d &other) {
				*this = other;
			}
      
			// Accessors:
			float   get_x()     const {return coord.x;}
			float   get_y()     const {return coord.y;}
			Point2f get_xy()    const {return coord;}
			Radians get_angle() const {return angle;}
      
			float& x() {return coord.x;}
			float& y() {return coord.y;}
      
			void operator=(const Pose2d &other) {
				this->coord = other.coord;
				this->angle = other.angle;
			}
      
			Point2f coord;
			Radians angle;
      
		}; // class Pose2d

    
    
    // #pragma mark --- Quadrilateral Class Declaration ---

    // A Quadrilateral is defined by four Point objects
    template<typename Type> class Quadrilateral
    {
    public:
      Point<Type> corners[4];

      Quadrilateral();

      Quadrilateral(const Point<Type> &corner1, const Point<Type> &corner2, const Point<Type> &corner3, const Point<Type> &corner4);

      Quadrilateral(const Quadrilateral<Type>& quad);

      Quadrilateral(const Rectangle<Type>& rect);

      void Print() const;

      Point<Type> ComputeCenter() const;

      // WARNING:
      // The width and height of a floating point Rectangle is different than that of an integer rectangle.
      Rectangle<Type> ComputeBoundingRectangle() const;

      bool operator== (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator+ (const Quadrilateral<Type> &quad2) const;

      Quadrilateral<Type> operator- (const Quadrilateral<Type> &quad2) const;

      inline Quadrilateral<Type>& operator= (const Quadrilateral<Type> &quad2);

      inline const Point<Type>& operator[] (const s32 index) const;
      inline Point<Type>& operator[] (const s32 index);
    }; // class Quadrilateral<Type>
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_POINT_DECLARATIONS_H_
