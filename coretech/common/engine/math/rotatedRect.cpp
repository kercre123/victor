/**
 * File: rotatedRect.cpp
 *
 * Author: Brad Neuman
 * Created: 2014-06-03
 *
 * Description: A 2D rectangle with a rotation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/lineSegment2d.h"
#include "coretech/common/engine/math/rotatedRect.h"
#include <math.h>

namespace Anki {

RotatedRectangle::RotatedRectangle(const Point2f& p0, const Point2f& p1, float otherSideLength)
// : _aabb(p0,p1)
{
  // if the vector p0->p1 is undefined, assume unit vector along x-axis
  Vec2f v0 = IsNearlyEqual(p1,p0) ? Vec2f(1.f,0.f) : p1 - p0;
  float l0 = v0.MakeUnitLength();

  // depending the the direction of v0 and sign of `otherSideLength`,
  //   create a new refernce frame where _vecX is in the first quadrant,
  //   _vecY is in the second, and the origin is the bottom left most
  //   corner of the rotated rectangle
  if ( FLT_LT(v0.x(), 0.f) ) {
    if ( FLT_LT(v0.y(), 0.f) ) {
      // v0 is in the 3rd quadrant
      _vecX = -v0;
      _vecY = {v0.y(), -v0.x()};
      _lengthX = l0;
      _lengthY = fabs(otherSideLength);
      _vertices[0] = FLT_GE_ZERO(otherSideLength) ? p0 - (_vecX * _lengthX) - (_vecY * _lengthY) : p0 - (_vecX * _lengthX);
    } else {
      // v0 is in the 2nd quadrant
      _vecX = {v0.y(), -v0.x()};
      _vecY = v0;
      _lengthX = fabs(otherSideLength);
      _lengthY = l0;
      _vertices[0] = FLT_GE_ZERO(otherSideLength) ? p0 - (_vecX * _lengthX) : p0;
    }
  } else {
    if ( FLT_LT(v0.y(), 0.f) ) {
      // v0 is in the 4th quadrant
      _vecX = {-v0.y(), v0.x()};
      _vecY = -v0;
      _lengthX = fabs(otherSideLength);
      _lengthY = l0;
      _vertices[0] = FLT_GE_ZERO(otherSideLength) ? p0 - (_vecY * _lengthY) : p0 - (_vecX * _lengthX) - (_vecY * _lengthY);
    } else {
      // v0 is in the 1st quadrant
      _vecX = v0;
      _vecY = {-v0.y(), v0.x()};
      _lengthX = l0;
      _lengthY = fabs(otherSideLength);
      _vertices[0] = FLT_GE_ZERO(otherSideLength) ? p0 : p0 - (_vecY * _lengthY) ; 
    }
  }

  // Generate remaining vertices in global reference frame.
  //   the lowest point will be the origin for the rotated reference frame
  //   vertices should hold points in the following order: {bottom, right, left, top}
  //   if axis aligned, then it should be {BL, BR, TL, TR}
  _vertices[1] = _vertices[0] + (_vecX * _lengthX);
  _vertices[2] = _vertices[0] + (_vecY * _lengthY);
  _vertices[3] = _vertices[1] + (_vecY * _lengthY);
}


AxisAlignedQuad RotatedRectangle::GetAxisAlignedBoundingBox() const 
{ 
  return AxisAlignedQuad({_vertices[2].x(), _vertices[0].y()}, {_vertices[1].x(), _vertices[3].y()}); 
}

bool RotatedRectangle::Contains(const Point2f& p) const
{
  // given the corner point and unit vectors, we can take a dot
  // product of the vector from the corner to the given point. This
  // will give us a projection of the point onto each side of the
  // rectangle. If that value is within 0 and the length of that side
  // for both sides, then the point is inside the rectangle

  const Point2f testVec = p - _vertices[0];

  float projX = DotProduct(_vecX, testVec);
  if(FLT_LT(projX, 0.f) || FLT_GT(projX, _lengthX)) { return false; }

  float projY = DotProduct(_vecY, testVec);
  if(FLT_LT(projY, 0.f) || FLT_GT(projY, _lengthY)) { return false; }

  return true;
}

bool RotatedRectangle::Intersects(const AxisAlignedQuad& quad) const 
{ 
  // ordering for quadVertices shoud be: {BL, BR, UL, UR}
  const std::vector<Point2f>& quadVertices = quad.GetVertices();

  // check if any of the quad edges create a separating axis
  if (FLT_GT( _vertices[0].y(), quadVertices[3].y() )) { return false; }
  if (FLT_LT( _vertices[1].x(), quadVertices[0].x() )) { return false; }
  if (FLT_GT( _vertices[2].x(), quadVertices[3].x() )) { return false; }
  if (FLT_LT( _vertices[3].y(), quadVertices[0].y() )) { return false; }

  if (FLT_GT( DotProduct(_vecX, quadVertices[0] - _vertices[0]), _lengthX )) { return false; }
  if (FLT_GT( DotProduct(_vecY, quadVertices[1] - _vertices[0]), _lengthY )) { return false; }
  if (FLT_LT( DotProduct(_vecX, quadVertices[3] - _vertices[0]), 0.f ))      { return false; }
  if (FLT_LT( DotProduct(_vecY, quadVertices[2] - _vertices[0]), 0.f ))      { return false; }

  return true;
}

bool RotatedRectangle::InHalfPlane(const Halfplane2f& H) const 
{ 
  return std::all_of(_vertices.begin(), _vertices.end(), [&H](const Point2f& p) { return H.Contains(p); } );
}

Quad2f RotatedRectangle::GetQuad() const
{
  Quad2f Q({_vertices[0], _vertices[1], _vertices[2], _vertices[3]});
  return Q.SortCornersClockwise();
}

}
