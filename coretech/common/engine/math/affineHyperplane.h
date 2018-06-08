/**
 * File: affineHyperplane.h
 *
 * Author: Michael Willett
 * Created: 2018-02-07
 *
 * Description: defines an affine hyperplane in N-dimensional space:
 *                   H = { x ∈ ℝⁿ | Ax + b = 0  }
 * 
 *              defines the open upper halfplane defined by an affine hyerplane:
 *                   H = { x ∈ ℝⁿ | Ax + b > 0  }
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_AFFINE_HYPERPLANE_H__
#define __COMMON_ENGINE_MATH_AFFINE_HYPERPLANE_H__

#include "coretech/common/engine/math/pointSet.h"
#include "util/math/math.h"

namespace Anki {

template<DimType N, typename T>
class AffineHyperplane : public ConvexPointSet<N,T>
{
public:
  AffineHyperplane(Point<N,T> a_, T b_) : a(a_), b(b_) {}
  virtual ~AffineHyperplane() {}

  // solve for the result of the line equation a'x + b
  virtual T    Evaluate(const Point<N,T>& x)        const          { return ( DotProduct(a, x) + b    ); }
  
  // return a'x + b = 0;
  virtual bool Contains(const Point<N,T>& x)        const override { return ( NEAR_ZERO(Evaluate(x))  ); }
  
  // check if:    { x ∈ ℝⁿ | Ax + b = 0  } ⊂ H
  virtual bool InHalfPlane(const Halfplane<N,T>& H) const override { return ( (a == H.a) && FLT_GT(b, H.b) ); }

  const Point<N,T> a;
  const T          b;
};

template<DimType N, typename T>
class Halfplane : public AffineHyperplane<N, T>
{
public:
  Halfplane(Point<N,T> a_, T b_)     : AffineHyperplane<N, T>(a_, b_) {}
  Halfplane(AffineHyperplane<N,T> h) : AffineHyperplane<N, T>(h) {}
  virtual ~Halfplane() {}

  // return a'x + b > 0
  virtual bool Contains(const Point<N,T>& x) const override { return ( FLT_GT(this->Evaluate(x), 0.f) ); }
};

using Line2f      = AffineHyperplane<2, f32>;
using Halfplane2f = Halfplane<2, f32>;

// helper method for line intersection
inline bool GetIntersectionPoint(const Line2f& l1, const Line2f& l2, Point2f& p)
{
  auto det = [](const Point2f& a, const Point2f& b) { return a[0] * b[1] - b[0] * a[1]; };

  // if the determinant of the a of each line is 0, the lines are parallel
  float div = det(l2.a, l1.a);
  if ( NEAR_ZERO(div) ) { return false; }

  p.x() = det({l1.b, l2.b},       {l1.a[1], l2.a[1]}) / div;
  p.y() = det({l1.a[0], l2.a[0]}, {l1.b, l2.b}      ) / div;
  return true;
};


}

#endif
