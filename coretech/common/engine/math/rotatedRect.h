/**
 * File: rotatedRect.h
 *
 * Author: Brad Neuman
 * Created: 2014-06-03
 *
 * Description: A 2D rectangle with a rotation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef _ANKICORETECH_COMMON_ROTATEDRECT_H_
#define _ANKICORETECH_COMMON_ROTATEDRECT_H_

#include "coretech/common/engine/math/quad.h"
#include "coretech/common/engine/math/convexPolygon2d.h"
#include "coretech/common/engine/math/lineSegment2d.h"

/**
 * A class representing a rectangle in a planar (2D) environment. This
 * class provides a constructor that can create a rectangle out of the
 * parameters in a line in an environment file, or out of any convex
 * polygon. It will also automatically compute unit vectors as needed
 * and provides a checkPoint function to check if a given point is
 * inside the rectangle.
 *
 * NOTE: this class was designed for use in the planner. It uses extra
 * memory in exchange for faster collision checking. Take that into
 * consideration before creating thousands of these
 */
namespace Anki {

// could inherit from ConvexPolygon if we wanted...
class RotatedRectangle : public BoundedConvexSet<2, f32>
{
public:
  /** This constructor builds a rectangle using two points and a
   * length. The two points define one side of the rectangle, and then
   * the length defines the other side, which is perpendicular to the
   * first.
   */
  RotatedRectangle(const Point2f& p0, const Point2f& p1, float otherSideLength);
  RotatedRectangle(float x0, float y0, float x1, float y1, float otherSideLength) 
  : RotatedRectangle({x0,y0}, {x1,y1}, otherSideLength) {} 
  
  /** Returns true if the given (x,y) point is inside (or on the
   * border of) this rectangle
   */
  bool Contains(float x, float y) const { return Contains({x,y}); }
  virtual bool Contains(const Point2f& p) const override;

  // intersection with AABB
  virtual bool Intersects(const AxisAlignedQuad& quad) const override;

  // returns true if the convex polygon is fully contained by the halfplane H
  virtual bool InHalfPlane(const Halfplane2f& H) const override;

  // get the AABB for this poly
  virtual AxisAlignedQuad GetAxisAlignedBoundingBox() const override;

  /** Outputs in same format as input to the constructor, separated by
   * a space.
   */
  void Dump(std::ostream& out) const;

  float GetWidth()  const { return _lengthX; }
  float GetHeight() const { return _lengthY; }
  float GetX()      const { return _vertices[0].x(); }
  float GetY()      const { return _vertices[0].y(); }

  Quad2f GetQuad() const;

private:
  float _lengthX, _lengthY;
  Vec2f _vecX, _vecY;
  std::array<Point2f, 4> _vertices;
};

}

#endif

