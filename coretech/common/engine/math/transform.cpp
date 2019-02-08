/**
 * File: transform.cpp
 *
 * Author: Andrew Stein
 * Created: 8/3/2017
 *
 * Description: Defines classes for storing a 2D/3D Transform, meaning a rotation and translation.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "coretech/common/engine/math/transform.h"

#include "coretech/common/shared/math/matrix_impl.h"
#include "coretech/common/engine/math/quad_impl.h"

#include "coretech/common/shared/utilities_shared.h"

#include "util/math/math.h"

#include <cmath>

namespace Anki {

  
#pragma mark -
#pragma mark Transform2d Implementations


void Transform2d::TranslateForward(float dist)
{
  _translation.x() += dist * cosf(_angle.ToFloat());
  _translation.y() += dist * sinf(_angle.ToFloat());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform2d::operator*=(const Transform2d& other)
{
  _angle += other._angle;
  
  _translation += this->GetRotationMatrix() * other._translation;
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform2d Transform2d::operator* (const Transform2d& other) const
{
  Radians newAngle(_angle + other._angle);
  RotationMatrix2d newRotation(newAngle);
  Point2f newTranslation(GetRotationMatrix() * other._translation);
  newTranslation += _translation;
  
  return Transform2d(newAngle, newTranslation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform2d::PreComposeWith(const Transform2d& other)
{
  _angle += other._angle;

  _translation = other.GetRotationMatrix() * _translation;
  _translation += other._translation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform2d Transform2d::GetInverse(void) const
{
  Transform2d returnTform(*this);
  returnTform.Invert();
  return returnTform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform2d& Transform2d::Invert(void)
{
  _angle *= -1.f;
  RotationMatrix2d R(_angle);

  _translation *= -1.f;
  _translation = R * _translation;
  
  return *this;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point2f Transform2d::operator*(const Point2f &point) const
{
  Point2f pointOut( GetRotationMatrix() * point);
  pointOut += _translation;
  
  return pointOut;
}

#pragma mark -
#pragma mark Transform3d Implementations

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform3d::TranslateForward(float dist)
{
  SetTranslation( _translation + (_rotation * Point3f{dist, 0.f, 0.f}) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Composition: this = this*other
void Transform3d::operator*=(const Transform3d& other)
{
  // this.T = this.R*other.T + this.T;
  Vec3f thisTranslation(_translation); // temp storage
  _translation = _rotation * other._translation;
  _translation += thisTranslation;

  // this.R = this.R * other.R
  // Note: must do this _after_ the translation update, since that uses this.R
  _rotation *= other._rotation;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform3d Transform3d::operator*(const Transform3d& other) const
{
  Vec3f newTranslation = _rotation * other._translation;
  newTranslation += _translation;
  
  Rotation3d newRotation(_rotation);
  newRotation *= other._rotation;
  
  return Transform3d(newRotation, newTranslation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Composition: this = other * this
void Transform3d::PreComposeWith(const Transform3d& other)
{
  _rotation.PreMultiplyBy(other._rotation);
  _translation = other._rotation * _translation;
  _translation += other._translation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform3d Transform3d::GetInverse(void) const
{
  Transform3d returnTransform(_rotation, _translation);
  returnTransform.Invert();
  return returnTransform;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Transform3d& Transform3d::Invert(void)
{
  _rotation.Invert();
  _translation *= -1.f;
  _translation = _rotation * _translation;
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform3d::RotateBy(const Rotation3d& R)
{
  _translation = R * _translation;
  _rotation.PreMultiplyBy(R);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform3d::RotateBy(const Radians& angleIn)
{
  // Keep same rotation axis, but add the incoming angle
  RotateBy(Rotation3d(angleIn, _rotation.GetAxis()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform3d::RotateBy(const RotationVector3d& Rvec)
{
  RotateBy(Rotation3d(Rvec));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Transform3d::RotateBy(const RotationMatrix3d& Rmat)
{
  RotateBy(Rotation3d(Rmat));
}

} // namespace Anki
