/** @file localization_geometry.h
* @author Daniel Casner
* Extracted geometry.h by Peter Barnum, 2013
* @date 2015
*
* Copyright Anki, Inc. 2015
* For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#ifndef __ROBOT_LOCALIZATION_GEOMETRY_H_
#define __ROBOT_LOCALIZATION_GEOMETRY_H_

#include "radians.h"

namespace Anki
{
  namespace Embedded
  {

    template<typename Type> class Point
    {
    public:
      Type x;
      Type y;

      Point();

      Point(const Type x, const Type y);

      Point(const Point<Type>& pt);

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

    typedef Point<float> Point2f;

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
      float   GetX()     const {return coord.x;}
      float   GetY()     const {return coord.y;}
      Point2f get_xy()    const {return coord;}
      Radians GetAngle() const {return angle;}

      float& x() {return coord.x;}
      float& y() {return coord.y;}

      void operator=(const Pose2d &other) {
        this->coord = other.coord;
        this->angle = other.angle;
      }

      Point2f coord;
      Radians angle;
    }; // class Pose2d
  } // namespace Embedded
} // namespace Anki


#endif
