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

#include "coretech/common/shared/math/radians.h"

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
      Pose2d() : trans(0,0), rot(0) {}
      Pose2d(const float x, const float y, const Radians angle) : trans(x,y), rot(angle) {};
      Pose2d(const Point2f translation, const Rotation2d rotation) : trans(translation), rot(rotation) {};
      Pose2d(const Pose2d &other) {
        *this = other;
      }

      // Accessors:
      float   GetX()     const {return trans.x;}
      float   GetY()     const {return trans.y;}
      Radians GetAngle() const {return rot.GetAngle();}

      const Point2f& GetTranslation() const {return trans;}
      const Rotation2d& GetRotation() const {return rot;}
      
      float& x() {return trans.x;}
      float& y() {return trans.y;}
      Radians& angle() {return rot.angle();}

      void operator=(const Pose2d &other) {
        this->trans = other.trans;
        this->rot = other.rot;
      }
      Pose2d operator*(const Pose2d& other);
      
      // Returns pose of p with respect to this pose.
      // Note: These are tree-less poses so no pose chaining occurs.
      //       This pose and pose p should be wrt the same parent.
      Pose2d GetWithRespectTo(const Pose2d& p) const;

    private:
      Point2f trans;
      Rotation2d rot;
    }; // class Pose2d
  } // namespace Embedded
} // namespace Anki


#endif
