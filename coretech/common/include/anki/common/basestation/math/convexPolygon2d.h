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

#ifndef __COMMON_BASESTATION_MATH_CONVEXPOLYGON2D_H__
#define __COMMON_BASESTATION_MATH_CONVEXPOLYGON2D_H__

#include "anki/common/basestation/math/polygon.h"
#include "anki/common/basestation/math/point.h"

#include <vector>

namespace Anki {

class ConvexPolygon
{
public:
  enum EClockDirection { CW, CCW };
  
  // must be constructed from an existing polygon
  ConvexPolygon(const Poly2f& basePolygon);
  
        Point2f& operator[](size_t i)       { return _poly[GetInternalIdx(i)]; }
  const Point2f& operator[](size_t i) const { return _poly[GetInternalIdx(i)]; }

  const Poly2f& GetSimplePolygon() const { return _poly; }

  void SetClockDirection(EClockDirection d) { _currDirection = d; };
  EClockDirection GetClockDirection() const { return _currDirection; }
    
  size_t size() const { return _poly.size(); }
    
  float GetMinX() const { return _poly.GetMinX(); }
  float GetMaxX() const { return _poly.GetMaxX(); }
  float GetMinY() const { return _poly.GetMinY(); }
  float GetMaxY() const { return _poly.GetMaxY(); }
  
  f32     GetEdgeAngle(size_t i)  const { return _poly.GetEdgeAngle(GetInternalIdx(i)); }
  Point2f GetEdgeVector(size_t i) const { return _poly.GetEdgeVector(GetInternalIdx(i)); }
  Point2f ComputeCentroid()       const { return _poly.ComputeCentroid(); }
    
  static bool IsConvex(const Poly2f& poly);
  
  void RadialExpand(f32 d);
  
private:
  Poly2f          _poly;
  EClockDirection _currDirection;
  size_t GetInternalIdx(size_t i) const { return (_currDirection == CW) ? i : _poly.size() - i;}
};

}

#endif
