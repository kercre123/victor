/**
 * File: convexPolygon2d.h
 *
 * Author: Michael Willett
 * Created: 2017-10-3
 *
 * Description: A class wrapper for a 2d polygon with convexity helpers and constraints.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "coretech/common/engine/math/convexPolygon2d.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "util/global/globalDefinitions.h"

#include <cmath>
#include <cfloat>
#include <algorithm>

namespace Anki {
  
ConvexPolygon::ConvexPolygon(const Poly2f& basePolygon)
: Poly2f(basePolygon)
, _currDirection(CW)
{
  // check convexity only if in dev code, otherwise assume it is good
  DEV_ASSERT(IsConvex(basePolygon), "Tried to create a convex polygon from non-convex polygon");

  // only check internal angle orientation if basePolygon is not a point or line
  if (basePolygon.size() > 2)  {
    // check first internal angle and force to [0, 2π) to get current clock direction
    f32 internalAngle = std::fmod(GetEdgeAngle(1) - GetEdgeAngle(0) + 2 * M_PI, 2 * M_PI);
    if (internalAngle < M_PI) // poly inserted is oriented counter clockwise, so reverse it to CW
    {
      std::reverse(begin(), end());
    }
  }
}

bool ConvexPolygon::IsConvex(const Poly2f& poly)
{ 
  // should only check for poly with 3 or more points
  for (int i = 2; i < poly.size(); ++i)
  {
    // get internal angles formed by last 3 line segments
    Radians angle1 = poly.GetEdgeAngle(i-2);
    Radians angle2 = poly.GetEdgeAngle(i-1);
    Radians angle3 = poly.GetEdgeAngle(i);
    Radians d_angle1 = angle2 - angle1;
    Radians d_angle2 = angle3 - angle2;

    // we may end up with straight line segments, so make sure floating point precision doesnt break anything
    if ( !d_angle1.IsNear(0) && !d_angle2.IsNear(0) ) { 
      // make sure the direction of both angles is the same
      if (std::signbit(d_angle1.ToFloat()) != std::signbit(d_angle2.ToFloat())) {
        return false;
      }
    }
  }
  
  return true;
}

void ConvexPolygon::RadialExpand(f32 d) {
  if (d >= 0) 
  {
    Point2f center = ComputeCentroid();
    
    for (Point2f& p : _points) {
      Point2f centeredPoint = p - center;
      f32 length = std::hypot(centeredPoint.x(), centeredPoint.y());      
      // (point / length) is a unit vector, therefor offset is a push scaled by `d` on this vector           
      p += centeredPoint * (d / length);
    }
  } else {
    PRINT_NAMED_WARNING("ConvexPolygon.RadialExpand", "called expand with a negative distance.");
  }
}

// Given a set of points, computes the convex hull using Graham scan algorithm:
// https://en.wikipedia.org/wiki/Graham_scan
ConvexPolygon ConvexPolygon::ConvexHull(std::vector<Point2f>&& points)
{
  Poly2f outPoly;
  outPoly.reserve(points.size());
  
  // check if there are not enough points for Graham scan
  const size_t numPoints = points.size();
  if ( numPoints <= 2 )
  {
    for (const auto pt : points) {
      outPoly.push_back(pt);
    }
    return ConvexPolygon(outPoly);
  }

  // Graham scan algorithm on points
  // make the first element in the sorted list the one with the lowest Y
  std::nth_element(points.begin(), points.begin(), points.end(), 
    [](const Point2f &p, const Point2f &q) {
      return FLT_NEAR(p.y(), q.y()) ? FLT_LT(p.x(), q.x()) : FLT_LT(p.y(), q.y());
    });
  
  // grabbing a reference since it should not change while sorting
  const Point2f& p0 = points[0]; 

  // cross product for quick checking of interior angle
  const auto crossProd = [](const Point2f& p0, const Point2f& p1, const Point2f& p2) -> float
  {   
    return (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() - p0.x()) * (p1.y() - p0.y());
  };

  // sort function for sorting all points relative to their angle with the start point
  const auto compareCounterClockwise = [&p0, &crossProd](const Point2f& p1, const Point2f& p2) -> bool
  {
    const float o = crossProd(p0,p1,p2);
    if ( NEAR_ZERO(o) ) {
      // if collinear, pick closest
      const Vec2f& originTo1 = p1 - p0;
      const Vec2f& originTo2 = p2 - p0;
      const bool p1Closer = FLT_LT(originTo1.LengthSq(),originTo2.LengthSq());
      return p1Closer;
    }
    
    return FLT_GE_ZERO(o);
  };
  
  // sort clockwise
  std::sort(points.begin()+1, points.end(), compareCounterClockwise);
  
  // add first 2 points
  outPoly.emplace_back( std::move(points[0]) );
  outPoly.emplace_back( std::move(points[1]) );
  
  // iterate all other candidate points
  for(size_t i=2; i<numPoints; ++i)
  {
    // calculate orientation we would obtain adding the point to the current convex hull
    size_t curPolyTop = outPoly.size()-1;
    float newOrientation = crossProd(outPoly[curPolyTop-1], outPoly[curPolyTop], points[i]);
    while( FLT_LE(newOrientation, 0.f) && curPolyTop > 0) {
      --curPolyTop;
      outPoly.pop_back();
      newOrientation = crossProd(outPoly[curPolyTop-1], outPoly[curPolyTop], points[i]);
    }
    
    // now this point becomes part of the convex hull
    outPoly.emplace_back( std::move(points[i]) );
  }
  
  return ConvexPolygon(outPoly);
}

} // end namespace
