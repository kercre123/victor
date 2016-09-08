/** @file localization_geometry.h
* @author Daniel Casner
* Extracted geometry.h by Peter Barnum, 2013
* @date 2015
*
* Copyright Anki, Inc. 2015
* For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#include "localization_geometry.h"
#include <math.h>

namespace Anki {
  namespace Embedded {

    // ------ Point2f class --------
    
    Point2f::Point2f(): x(0.0f), y(0.0f) {}
    Point2f::Point2f(const float x, const float y): x(x), y(y) {}
    Point2f::Point2f(const Point2f& pt): x(pt.x), y(pt.y) {}

    Point2f Point2f::operator+(const Point2f& other) const
    {
      return Point2f(this->x + other.x, this->y + other.y);
    }
    
    Point2f Point2f::operator-(const Point2f& other) const
    {
      return Point2f(this->x - other.x, this->y - other.y);
    }
    
    bool Point2f::operator== (const Point2f& other) const {
      return x == other.x && y == other.y;
    }

    Point2f& Point2f::operator*= (const float value) {
      x *= value;
      y *= value;
      return *this;
    }

    Point2f& Point2f::operator/= (const float value) {
      x /= value;
      y /= value;
      return *this;
    }

    Point2f& Point2f::operator+= (const Point2f& other) {
      x += other.x;
      y += other.y;
      return *this;
    }

    Point2f& Point2f::operator-= (const Point2f& other) {
      x -= other.x;
      y -= other.y;
      return *this;
    }

    Point2f& Point2f::operator= (const Point2f& other) {
      x = other.x;
      y = other.y;
      return *this;
    }

    float Point2f::Dist(const Point2f& other) const {
      const float xdiff = x - other.x;
      const float ydiff = y - other.y;
      return sqrtf(xdiff*xdiff + ydiff*ydiff);
    }

    float Point2f::Length() const {
      return sqrtf(x*x + y*y);
    }
    
    // ------- Rotation2d class ----------
    
    Rotation2d::Rotation2d()
    : _angle(0)
    {
    }
    
    Rotation2d::Rotation2d(Radians angle_rad)
    : _angle(angle_rad)
    {
    }
    
    Rotation2d Rotation2d::operator*(const Rotation2d& other) const
    {
      return Rotation2d(this->_angle + other._angle);
    }
    
    Point2f Rotation2d::operator*(const Point2f& p) const
    {
      const float cosAngle = cosf(_angle.ToFloat());
      const float sinAngle = sinf(_angle.ToFloat());
      
      Point2f v;
      v.x = cosAngle * p.x - sinAngle * p.y;
      v.y = sinAngle * p.x + cosAngle * p.y;
      return v;
    }
    
    void Rotation2d::Invert()
    {
      _angle *= -1;
    }
    
    Rotation2d Rotation2d::GetInverse() const
    {
      Rotation2d m(*this);
      m.Invert();
      return m;
    }
    
    Radians Rotation2d::GetAngle() const
    {
      return _angle;
    }
    
    
    // ------ Pose2d class --------
    
    Pose2d Pose2d::operator*(const Pose2d& other)
    {
      Radians newAngle(GetAngle() + other.GetAngle());
      Rotation2d newRotation(newAngle);
      Point2f newTranslation(GetRotation() * other.GetTranslation());
      newTranslation += trans;
      
      return Pose2d(newTranslation, newAngle);
    }
    
    Pose2d Pose2d::GetWithRespectTo(const Pose2d& p) const
    {
      Rotation2d R_p_inv = p.GetRotation().GetInverse();
      
      return Pose2d(R_p_inv * (GetTranslation() - p.GetTranslation()),
                    R_p_inv * GetRotation());
    }

  }
}
