/**
 * File: transform.h
 *
 * Author: Andrew Stein
 * Created: 8/3/2017
 *
 * Description: Defines classes for storing a 2D/3D Transform, meaning a rotation and translation.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Common_Math_Transform_H__
#define __Anki_Common_Math_Transform_H__

#include "util/logging/logging.h"

#include "coretech/common/shared/math/matrix.h"
#include "coretech/common/shared/math/rotation.h"
#include "coretech/common/shared/math/radians.h"

#include "coretech/common/engine/math/quad.h"

#include <list>

namespace Anki {

class Transform2d
{
public:
  
  Transform2d(const Radians& angle = 0.f, const Vec2f& T = {0.f,0.f})
  : _angle(angle)
  , _translation(T)
  {
    
  }
  
  float GetX()              const { return _translation.x(); }
  float GetY()              const { return _translation.y(); }
  const Radians& GetAngle() const { return _angle;           }
  
  RotationMatrix2d  GetRotationMatrix() const { return RotationMatrix2d(_angle); }
  const Vec2f&      GetTranslation()    const { return _translation; }
  
  void SetRotation(const Radians& angle) { _angle = angle; }
  void SetTranslation(const Vec2f& T);
  
  Transform2d& Invert();               // in place
  Transform2d  GetInverse(void) const; // return new Transform
  
  // Composition with another Transform
  void PreComposeWith(const Transform2d& other);
  void operator*=(const Transform2d &other); // in place
  Transform2d operator*(const Transform2d& other) const;
  
  // Translate dist along transform's existing angle
  void TranslateForward(float dist);
  
  // "Apply" transformation to points, quads, etc
  Point2f operator*(const Point2f &point) const;
  
  template<typename T>
  void ApplyTo(const Quadrilateral<2,T> &quadIn,
               Quadrilateral<2,T>       &quadOut) const;

  // Check *exact* numerical equality (generally only useful for unit tests, since floating point is involved)
  bool operator==(const Transform2d& other) const;
  
private:
  Radians _angle;
  Point2f _translation;
};

// ---------------------------------------------------------------------------------------------------------------------
  
class Transform3d
{
public:
  
  Transform3d(const Rotation3d& R = Rotation3d(0.f, Z_AXIS_3D()), const Vec3f& T = {0.f,0.f,0.f})
  : _rotation(R)
  , _translation(T)
  {
    
  }
  
  const Rotation3d& GetRotation()    const { return _rotation;    }
  const Vec3f&      GetTranslation() const { return _translation; }
  
  void SetRotation(const Rotation3d& R)    { _rotation = R;        }
  void SetTranslation(const Vec3f& trans)  { _translation = trans; }
  
  // Get the rotation angle, optionally around a specific axis.
  // By default the rotation angle *around the pose's rotation axis* is
  // returned. When 'X', 'Y', or 'Z' are specified (case insensitive), the
  // rotation angle around that axis is returned.
  template<char AXIS = ' '>
  Radians GetRotationAngle() const;
  
  // Inverse transformations
  Transform3d& Invert();               // in place
  Transform3d  GetInverse(void) const; // return new Transform
  
  // Composition with another Transform
  void PreComposeWith(const Transform3d& other);
  void operator*=(const Transform3d& other); // in place
  Transform3d operator*(const Transform3d& other) const;
  
  // Pre-multiply by a given rotation: this = R*this
  void RotateBy(const Radians& angle); // around existing axis
  void RotateBy(const RotationVector3d& Rvec);
  void RotateBy(const RotationMatrix3d& Rmat);
  void RotateBy(const Rotation3d& R);

  // translate along its current angle, along the +x axis (negative means backwards)
  void TranslateForward(float dist);
  
  // "Apply" transformation to points, quads, etc
  template<typename T>
  Point<3,T> operator*(const Point<3,T> &point) const;
  
  template<typename T>
  void ApplyTo(const std::vector<Point<3,T> >  &pointsIn,
               std::vector<Point<3,T> >        &pointsOut) const;
  
  template<typename T, size_t N>
  void ApplyTo(const std::array<Point<3,T>, N> &pointsIn,
               std::array<Point<3,T>, N>       &pointsOut) const;
  
  template<typename T>
  void ApplyTo(const Quadrilateral<3,T>        &quadIn,
               Quadrilateral<3,T>              &quadOut)   const;
  
  // Check *exact* numerical equality (generally only useful for unit tests, since floating point is involved)
  bool operator==(const Transform3d& other) const;
  
private:
  Rotation3d _rotation;
  Vec3f      _translation;
};


// =====================================================================================================================
// Inlined/Templated functions
// =====================================================================================================================

// ---------------------------------------------------------------------------------------------------------------------
// Transform2d

inline bool Transform2d::operator==(const Transform2d& other) const
{
  return (_angle == other._angle) && (_translation == other._translation);
}
  
template<typename T>
void Transform2d::ApplyTo(const Quadrilateral<2,T> &quadIn,
                          Quadrilateral<2,T>       &quadOut) const
{
  using namespace Quad;
  quadOut[TopLeft]     = (*this) * quadIn[TopLeft];
  quadOut[TopRight]    = (*this) * quadIn[TopRight];
  quadOut[BottomLeft]  = (*this) * quadIn[BottomLeft];
  quadOut[BottomRight] = (*this) * quadIn[BottomRight];
}

// ---------------------------------------------------------------------------------------------------------------------
// Transform3d

inline bool Transform3d::operator==(const Transform3d& other) const
{
  return (_rotation == other._rotation) && (_translation == other._translation);
}

template<>
inline Radians Transform3d::GetRotationAngle<' '>() const
{
  // return the inherient axis of the rotation
  return GetRotation().GetAngle();
}

template<>
inline Radians Transform3d::GetRotationAngle<'X'>() const
{
  return GetRotation().GetAngleAroundXaxis();
}

template<>
inline Radians Transform3d::GetRotationAngle<'Y'>() const
{
  return GetRotation().GetAngleAroundYaxis();
}

template<>
inline Radians Transform3d::GetRotationAngle<'Z'>() const
{
  return GetRotation().GetAngleAroundZaxis();
}

template<>
inline Radians Transform3d::GetRotationAngle<'x'>() const
{
  return GetRotation().GetAngleAroundXaxis();
}

template<>
inline Radians Transform3d::GetRotationAngle<'y'>() const
{
  return GetRotation().GetAngleAroundYaxis();
}

template<>
inline Radians Transform3d::GetRotationAngle<'z'>() const
{
  return GetRotation().GetAngleAroundZaxis();
}

template<typename T>
Point<3,T> Transform3d::operator*(const Point<3,T> &pointIn) const
{
  Point3f pointOut( GetRotation() * pointIn );
  pointOut += GetTranslation();
  
  return pointOut;
}

template<typename T>
void Transform3d::ApplyTo(const Quadrilateral<3,T> &quadIn,
                          Quadrilateral<3,T>       &quadOut) const
{
  using namespace Quad;
  quadOut[TopLeft]     = (*this) * quadIn[TopLeft];
  quadOut[TopRight]    = (*this) * quadIn[TopRight];
  quadOut[BottomLeft]  = (*this) * quadIn[BottomLeft];
  quadOut[BottomRight] = (*this) * quadIn[BottomRight];
}

template<typename T>
void Transform3d::ApplyTo(const std::vector<Point<3,T> > &pointsIn,
                          std::vector<Point<3,T> >       &pointsOut) const
{
  const size_t numPoints = pointsIn.size();
  
  if(pointsOut.size() == numPoints)
  {
    // The output vector already has the right number of points
    // in it.  No need to construct a new vector full of (0,0,0)
    // points with resize; just replace what's there.
    for(size_t i=0; i<numPoints; ++i)
    {
      pointsOut[i] = (*this) * pointsIn[i];
    }
    
  } else {
    // Clear the output vector, and use push_back to add newly-
    // constructed points. Again, this avoids first creating a
    // bunch of (0,0,0) points via resize and then immediately
    // overwriting them.
    pointsOut.clear();
    
    for(const Point3f& x : pointsIn)
    {
      pointsOut.emplace_back( (*this) * x );
    }
  }
}

template<typename T, size_t N>
void Transform3d::ApplyTo(const std::array<Point<3,T>, N> &pointsIn,
                          std::array<Point<3,T>, N>       &pointsOut) const
{
  for(size_t i=0; i<N; ++i)
  {
    pointsOut[i] = (*this) * pointsIn[i];
  }
}
  
} // namespace Anki

#endif /* __Anki_Common_Math_Transform_H__ */
