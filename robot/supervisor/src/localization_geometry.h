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

    class Point2f
    {
    public:
      float x, y;

      Point2f();

      Point2f(const float x, const float y);

      Point2f(const Point2f& pt);


      bool operator== (const Point2f &point2) const;
      Point2f operator+(const Point2f& other) const;
      Point2f operator-(const Point2f& other) const;
      Point2f& operator*= (const float value);
      Point2f& operator/= (const float value);
      Point2f& operator+= (const Point2f &point2);
      Point2f& operator-= (const Point2f &point2);

      Point2f& operator= (const Point2f &point2);

      // The L2 (Euclidian) distance between this point and an input point.
      float Dist(const Point2f &point2) const;

      // The L2 (Euclidian) distance between this point and the origin
      float Length() const;
    }; // class Point2f

    
    class Rotation2d
    {
    public:
      Rotation2d();
      Rotation2d(Radians angle_rad);
      
      Rotation2d operator*(const Rotation2d& other) const;
      Point2f operator*(const Point2f& p) const;
      
      Radians& angle() {return _angle;}
      
      Radians GetAngle() const;
      Rotation2d GetInverse() const;
      void Invert();
      
    private:
      Radians _angle;
    };

    
    class Pose2d
    {
    public:
      // Constructors:
      Pose2d() : coord(0,0), rot(0) {}
      Pose2d(const float x, const float y, const Radians angle) : coord(x,y), rot(angle) {};
      Pose2d(const Point2f translation, const Rotation2d rotation) : coord(translation), rot(rotation) {};
      Pose2d(const Pose2d &other) {
        *this = other;
      }

      // Accessors:
      float   GetX()     const {return coord.x;}
      float   GetY()     const {return coord.y;}
      Radians GetAngle() const {return rot.GetAngle();}

      const Point2f& GetTranslation() const {return coord;}
      const Rotation2d& GetRotation() const {return rot;}
      
      float& x() {return coord.x;}
      float& y() {return coord.y;}
      Radians& angle() {return rot.angle();}

      void operator=(const Pose2d &other) {
        this->coord = other.coord;
        this->rot = other.rot;
      }
      Pose2d operator*(const Pose2d& other);
      Pose2d GetWithRespectTo(const Pose2d& p) const;

    private:
      Point2f coord;
      Rotation2d rot;
    }; // class Pose2d
  } // namespace Embedded
} // namespace Anki


#endif
