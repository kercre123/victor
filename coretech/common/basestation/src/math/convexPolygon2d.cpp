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

#include "anki/common/basestation/math/convexPolygon2d.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "util/global/globalDefinitions.h"

#include <cmath>
#include <cfloat>

namespace Anki {
  
ConvexPolygon::ConvexPolygon(const Poly2f& basePolygon)
: _poly(basePolygon)
, _currDirection(CW)
{
  assert(basePolygon.size() > 2);
  // check first internal angle and force to [0, 2π) to get current clock direction
  f32 internalAngle = std::fmod(_poly.GetEdgeAngle(1) - _poly.GetEdgeAngle(0) + 2 * M_PI, 2 * M_PI);
  if (internalAngle < M_PI) // poly inserted is oriented counter clockwise, so reverse it to CW
  {
    std::reverse(_poly.begin(), _poly.end());
  } 
  
  // check convexity only if in dev code, otherwise assume it is good
  if(ANKI_DEVELOPER_CODE) 
  {
    if (!IsConvex(basePolygon))
    {
      assert("Tried to create a convex polygon from non-convex polygon");
    }
  }
}

bool ConvexPolygon::IsConvex(const Poly2f& poly)
{
  f32 lastAngle = poly.GetEdgeAngle(0);
  // force to [-π, π) and get direction
  const bool dir = std::signbit(std::fmod(poly.GetEdgeAngle(1) - lastAngle + 2 * M_PI, 2 * M_PI) - M_PI);
  bool retv = true;
  
  for (int i = 1; i < poly.size(); ++i)
  {
    f32 angle = poly.GetEdgeAngle(i);
    f32 d_angle = std::fmod(angle - lastAngle + 2 * M_PI, 2 * M_PI) - M_PI;    // force to [-π, π)
    retv &= (std::signbit(d_angle) == dir);
    lastAngle = angle;
  }
  
  return retv;
}

void ConvexPolygon::RadialExpand(f32 d) {
  if (d >= 0) 
  {
    Point2f center = _poly.ComputeCentroid();
    
    for (Point2f& p : _poly) {
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
