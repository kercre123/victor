/**
 * File: lineSegment2d.h
 *
 * Author: Michael Willett
 * Created: 2017-10-12
 *
 * Description: A class that holds a 2d line segment for fast intersection checks 
 *              in point-point form: 
 *                    Δy(x - x₁) - Δx(y - y₁) = 0
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __COMMON_BASESTATION_MATH_LINESEGMENT2D_H__
#define __COMMON_BASESTATION_MATH_LINESEGMENT2D_H__

#include "coretech/common/engine/math/affineHyperplane.h"

namespace Anki {

class LineSegment : public Line2f
{
public:  
  // cache useful data for minor speedup of intersection checks
  LineSegment(const Point2f& from, const Point2f& to) ;
  
  // check if the point is Co-linear with line segment and constrained by start and end points
  virtual bool Contains(const Point2f& p) const override;

  // check if this line segment is fully above the line l
  virtual bool InHalfPlane(const Halfplane2f& H) const override;

  // solve the line equation
  virtual float Evaluate(const Point2f& p) const override;

  // check if the two line segments intersect
  bool IntersectsWith(const LineSegment& l) const;

  // check if two line segments intersect and return the intersection in location.
  // returns true is there's a single point of intersection
  bool IntersectsAt(const LineSegment& l, Point2f& location) const;
  
  // Create a Line perpendicular to the line segment, intersecting at the midpoint
  Line2f GetPerpendicularBisector() const;
  
  // length of line segment
  float Length() const { return std::hypot(dX, dY); }
  
  // Get start and end points
  const Point2f& GetFrom() const { return from; }
  const Point2f& GetTo()   const { return to; } 
  
private:
  
  enum class EOrientation {COLINEAR, CW, CCW};
  
  Point2f from;
  Point2f to;
  
  float minX;
  float maxX;
  float minY;
  float maxY;
  float dX;
  float dY;
  
  // check which clock direction the points form `from->to->p`
  EOrientation Orientation(const Point2f& p) const;

  // checks if point is constrained by AABB defined by `from->to`
  bool InBoundingBox(const Point2f& p) const;
};

}

#endif
