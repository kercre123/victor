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
}
