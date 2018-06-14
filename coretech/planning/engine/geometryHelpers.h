/**
 * File: math
 *
 * Author: Michael Willett
 * Created: 2018-06-04
 *
 * Description: helper math functions for planner geometry
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Coretech_Planning_GeometryHelpers_H__
#define __Coretech_Planning_GeometryHelpers_H__

#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/ball.h"
#include "coretech/common/engine/math/lineSegment2d.h"

namespace Anki {  
  
// arc defined by three points
struct Arc {
  Point2f start;
  Point2f middle;
  Point2f end;
};

// -1 := counter clockwise, 1 := clockwise, 0 := colinear
inline int GetOrientation(const Vec2f& p, const Vec2f& q) {
  float cross = (p.y() * q.x()) - (p.x() * q.y());
  return NEAR_ZERO(cross) ? 0 : FLT_GE_ZERO(cross) ? 1 : -1 ;
}

// returns a list of unit vectors that are tangent to the circle at the origin with radius r, that also pass through
// point p. If p is inside the circle, or the circle has an invalid radius, return an empty list
inline std::vector<Vec2f> TangentsToCircle(const Point2f& p, float r) 
{
  float dx = r*r - p.x()*p.x();
  float raticand = p.y()*p.y() - dx;

  if (FLT_LT(raticand, 0.f) || FLT_LE(r, 0.f)) { return {}; }
  float a = -p.x()*p.y();
  float b = r*sqrt(raticand);

  std::vector<Vec2f> retv = {{dx, a+b}, {dx, a-b}};
  retv[0].MakeUnitLength();
  retv[1].MakeUnitLength();
  return retv;
}

// given sequence of three points {p0, p1, p2} that define the connected line segments 
// (L1 := p0->p1) and (L2 := p1->p2) and a raidus r, generate a circumscribed arc {t0, p1, t1} 
// such that:
//   - (A := t0->p1->t1) defines an Arc of radius r
//   - the modified line segment (L1' := p0->t0) is tangent to arc A at point t0
//   - the modified line segment (L2' := t1->p2) is tangent to arc A at point t1
//   - the piecewise function defined by L1'->A->L2' is continuously differentialable
//   - returns FALSE if {p0, p1, p2} are colinear, or if no tangent exists with radius r
inline bool GetCircumscribedArc(const Point2f& p0, const Point2f& p1, const Point2f& p2, float r, Arc& A) {
  // first get the center point for a circle on the angle bisector of p0-p1-p2, where p1 is on the circle
  Vec2f v0 = p0 - p1;
  Vec2f v1 = p2 - p1;
  Vec2f bisector(v1 * v0.Length() + v0 * v1.Length());
  bisector.MakeUnitLength();
  const Point2f C = p1 + bisector * r;

  // get dir from 2d cross product
  const int dir = GetOrientation(p1-p0, p2-p1);

  // exit if co-linear
  if ( dir == 0 ) { return false; }

  // calculate tangent vectors after moving the origin to C
  std::vector<Vec2f> L1vecs = TangentsToCircle(p0 - C, r);
  std::vector<Vec2f> L2vecs = TangentsToCircle(p2 - C, r);

  // exit if tangents do not exist
  if (L1vecs.empty() || L2vecs.empty()) { return false; }

  // (MRW) we might be able to get away with only calculating two of these if we can calculate 
  // orientation before getting the tangent points
  const Point2f x0 = p0 + ( L1vecs[0] * DotProduct(L1vecs[0], C - p0) );
  const Point2f x1 = p0 + ( L1vecs[1] * DotProduct(L1vecs[1], C - p0) );
  const Point2f x2 = p2 + ( L2vecs[0] * DotProduct(L2vecs[0], C - p2) );
  const Point2f x3 = p2 + ( L2vecs[1] * DotProduct(L2vecs[1], C - p2) ); 

  // of possible tangent points, figure choose those that make work with the current direction
  A.start  = ( dir == GetOrientation(x0-p0, p1-x0) ) ? x0 : x1;
  A.middle = p1;
  A.end    = ( dir == GetOrientation(x2-p1, p2-x2) ) ? x2 : x3;

  return true;
}

// given sequence of three points {p0, p1, p2} that define the connected line segments 
// (L1 := p0->p1) and (L2 := p1->p2) and a raidus r, generate an inscribed arc {t0, t1, t2} 
// such that:
//   - (A := t0->t1->t2) defines an Arc of radius r
//   - the line segment (L1' := p0->t0) is tangent to arc A at point t0
//   - the line segment (L2' := t2->p2) is tangent to arc A at point t2
//   - returns FALSE if point t0 is not a point on L1
//   - returns FALSE if point t2 is not a point on L2
//   - returns FALSE if {p0, p1, p2} are colinear
inline bool GetInscribedArc(const Point2f& p0, const Point2f& p1, const Point2f& p2, float r, Arc& A) 
{
  // exit if co-linear
  if ( GetOrientation(p1-p0, p2-p1) == 0 ) { return false; }

  // first get the center point for a circle of on the angle bisector of p0-p1-p2 
  Vec2f v0 = p0 - p1;
  Vec2f v1 = p2 - p1;
  v0.MakeUnitLength();
  v1.MakeUnitLength();

  // get angle bisector of {v0,v1}
  Vec2f bisector(v1 + v0);
  bisector.MakeUnitLength();

  // solve the system for C:
  //    perp := normal vector of v0
  //    x := length of C
  //    C = x * bisector,
  //    r = C · perp
  const Point2f C = bisector * (r / ABS( DotProduct(Vec2f(v0.y(), -v0.x()), bisector) ));

  const Point2f t0 = p1 + (v0 * DotProduct(C, v0));
  const Point2f t1 = p1 + C - (bisector * r);
  const Point2f t2 = p1 + (v1 * DotProduct(C, v1));

  // if the tangent points are not on the line segments L1 or L2, return false
  if (!LineSegment(p0, p1).Contains(t0) || !LineSegment(p1, p2).Contains(t2)) { return false; }

  A = {t0, t1, t2};

  return true;
}

// convert a circle defined by three points into a circle defined by a center point and a radius
inline Ball2f ArcToBall(const Arc& a) {
  // center point is intersection of perpendicular bisectors of two chords
  LineSegment chord1(a.start, a.middle);
  LineSegment chord2(a.start, a.end);

  Point2f center;
  GetIntersectionPoint(chord1.GetPerpendicularBisector(), chord2.GetPerpendicularBisector(), center);

  return Ball2f(center, (a.start - center).Length());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//  Helpers to convert between PathSegment types and basic Geomtry types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// PathSegment<PointTurn> -> Point2f conversion
inline Point2f CreatePoint2f(const Planning::PathSegment& p) {
  DEV_ASSERT(p.GetType() == Planning::PST_POINT_TURN, "Cannot create Point2f from non-point turn path segment");
  return {p.GetDef().turn.x, p.GetDef().turn.y};
}

// create a PathSegment<PointTurn> at point `a.middle`, coming from `a.start`, turning towards `a.end`
inline Planning::PathSegment CreatePointTurnPath(const Arc& a) {
  // calculate heading angles
  Vec2f l1 = a.middle - a.start;
  Vec2f l2 = a.end - a.middle;
  Radians heading1 = atan2(l1.y(), l1.x());
  Radians heading2 = atan2(l2.y(), l2.x());

  // build segment
  Planning::PathSegment seg;
  seg.DefinePointTurn(a.middle.x(), a.middle.y(), heading1.ToFloat(), heading2.ToFloat(), 1.f, 1.f, 1.f, .1f, true );
  return seg;
}

// create PathSegment<PointTurn> from `start`, turning towards point `to`
inline Planning::PathSegment CreatePointTurnPath(const Pose2d& start, const Point2f& to) {
  // calculate heading angles
  Point2f p = start.GetTranslation();
  Vec2f dir = to - p;
  Radians heading = atan2(dir.y(), dir.x());

  // build segment
  Planning::PathSegment seg;
  seg.DefinePointTurn(p.x(), p.y(), start.GetAngle().ToFloat(), heading.ToFloat(), 1.f, 1.f, 1.f, .1f, true );
  return seg;
}

// create PathSegment<PointTurn> turning from point `from`, ending at pose `end`
inline Planning::PathSegment CreatePointTurnPath(const Point2f& from, const Pose2d& end) {
  // calculate heading angles
  Point2f p = end.GetTranslation();
  Vec2f dir = p - from;
  Radians heading = atan2(dir.y(), dir.x());

  // build segment
  Planning::PathSegment seg;
  seg.DefinePointTurn(p.x(), p.y(), heading.ToFloat(),  end.GetAngle().ToFloat(), 1.f, 1.f, 1.f, .1f, true );
  return seg;
}

// LineSegment -> PathSegment<Line> conversion
inline Planning::PathSegment CreateLinePath(const LineSegment& l) {
  // build segment
  Planning::PathSegment seg;
  seg.DefineLine(l.GetFrom().x(), l.GetFrom().y(), l.GetTo().x(), l.GetTo().y(), 1.f, 1.f, 1.f);
  return seg;
}

// PathSegment<Line> -> LineSegment conversion
inline LineSegment CreateLineSegment(const Planning::PathSegment& p) {
  DEV_ASSERT(p.GetType() == Planning::PST_LINE, "Cannot create LineSegment from non-line path segment");
  const auto& def = p.GetDef().line;
  return LineSegment({def.startPt_x, def.startPt_y}, {def.endPt_x, def.endPt_y});
}

// Arc -> PathSegment<Arc> conversion
inline Planning::PathSegment CreateArcPath(const Arc& a) {
  // convert to Ball2f to get center and radius
  Ball2f b = ArcToBall(a);

  // calculate start and sweep angles
  Vec2f startVec   = a.start - b.GetCentroid();
  Vec2f endVec     = a.end - b.GetCentroid();
  Radians startAngle( std::atan2(startVec.y(), startVec.x()) );
  Radians sweepAngle( std::atan2(endVec.y(), endVec.x()) - startAngle );

  // build segment
  Planning::PathSegment seg;
  seg.DefineArc(b.GetCentroid().x(), b.GetCentroid().y(), b.GetRadius(), startAngle.ToFloat(), sweepAngle.ToFloat(), 1.f, 1.f, 1.f);
  return seg;
}

// PathSegment<Arc> -> Arc conversion
inline Arc CreateArc(const Planning::PathSegment& p) {
  DEV_ASSERT(p.GetType() == Planning::PST_ARC, "Cannot create Arc from non-arc path segment");
  const auto& def = p.GetDef().arc;
  Point2f c(def.centerPt_x, def.centerPt_y);

  Arc retv;
  float r0 = def.startRad;
  float r1 = def.startRad + def.sweepRad/2;
  float r2 = def.startRad + def.sweepRad;

  retv.start  = c + Point2f(std::cos(r0), std::sin(r0)) * def.radius;
  retv.middle = c + Point2f(std::cos(r1), std::sin(r1)) * def.radius;
  retv.end    = c + Point2f(std::cos(r2), std::sin(r2)) * def.radius;

  return retv;
}

} // Anki

#endif