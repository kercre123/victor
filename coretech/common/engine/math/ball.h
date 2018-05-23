/**
 * File: ball.h
 *
 * Author: Michael Willett
 * Created: 2018-02-07
 *
 * Description: Defines an N-dimensional closed ball B with raidus r at point p, 
 *              using L2 norm in Euclidean space:
 *                      B = { x ∈ ℝⁿ | ‖x - p‖ ≤ r } 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_BALL_H__
#define __COMMON_ENGINE_MATH_BALL_H__

#include "coretech/common/engine/math/pointSet.h"

namespace Anki {

template <DimType N, typename T>
class Ball : public ConvexPointSet<N, T>
{
public:
  // construction/destruction
  Ball(const Point<N,T>& p = Point<N,T>(T(0)), const T& r = T(1)) : _p(p), _r( ABS(r) ), _rSq(r*r)  {}
  virtual ~Ball() override {}

  // helpers
  Point<N,T> GetCentroid() const              { return _p; }
  T          GetRadius() const                { return _r; }
  void       SetCentroid(const Point<N,T>& p) { _p = p; }
  void       SetRadius(const T& r)            { _r = r;}

  inline T   GetMin(DimType idx) const        { return _p[idx] - _r; }
  inline T   GetMax(DimType idx) const        { return _p[idx] + _r; }

  // check if the ball is fully in the halfplane h
  virtual bool InHalfPlane(const Halfplane<N,T>& H) const override 
  {
    // shortest distance from point x to hyperplane: d = (a'x+b) / ‖a‖
    T eval = H.Evaluate(_p);
    T dSq = eval * eval / H.a.LengthSq();

    // check if the distance to the plane is less than or equal to the radius,
    // otherwise return which side of the hyperplane the center is on
    return ( FLT_LE(dSq, _rSq) ) ? false : ( FLT_GT(eval, 0.f) );
  }

  // check if a point is contained in the ball
  virtual bool Contains(const Point<N,T>& x) const override
  { 
    // d² = Σ (xᵢ-pᵢٖ)²
    T dSq = 0;
    for (size_t i = 0; i < N; ++i) { 
      dSq += ((x[i] - _p[i]) * (x[i] - _p[i])); 
    }

    return ( FLT_LE(dSq, _rSq) );
  }

protected:
  Point<N,T> _p;
  T          _r;
  T          _rSq;
};

class Ball2f : public Ball<2, float>
{
public:
  using Ball::GetMin;
  using Ball::GetMax;

  // construction/destruction
  Ball2f(const Point2f& p = Point2f(), const float r = 1.f) : Ball(p, r) {}
  virtual ~Ball2f() override {}

  inline float GetMinX() const { return GetMin(0); }
  inline float GetMaxX() const { return GetMax(0); }
  inline float GetMinY() const { return GetMin(1); }
  inline float GetMaxY() const { return GetMax(1); }
};

using Ball3f = Ball<3, f32>;

}

#endif
