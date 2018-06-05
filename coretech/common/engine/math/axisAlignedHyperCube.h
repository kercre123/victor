/**
 * File: axisAlignedHyperCube.h
 *
 * Author: Michael Willett
 * Created: 2018-05-20
 *
 * Description: Defines an N-dimensional closed hypercube C defined by two points, p and q,
 *              with all faces normal to an axis:
 *                      C = { x ∈ ℝⁿ | min{pᵢ,qᵢ} ≤ xᵢ ≤ max{pᵢ,qᵢ},  ∀i ∈ n } 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_AXIS_ALIGNED_HYPER_CUBE_H__
#define __COMMON_ENGINE_MATH_AXIS_ALIGNED_HYPER_CUBE_H__

#include "coretech/common/engine/math/ball.h"
#include "coretech/common/engine/math/fastPolygon2d.h"

#include <algorithm>

namespace Anki {

template <DimType N, typename T>
class AxisAlignedHyperCube : public ConvexPointSet<N, T> {
public:

  // Constructor must enumerate all 2^N vertices for proper hyperplane Contains checks
  AxisAlignedHyperCube(const Point<N,T>& p, const Point<N,T>& q) : _vertices( std::pow(2,N) )
  {
    // we can generate a vertex by looking at the binary representation of its index.
    // for each bit position corresponding to a dimension in N, a `0` bit represents 
    // its min value, and `1` represents its max value. 
    // EG:    0b011 => {max₀, max₁, min₂}
    //
    // NOTE: this ordering ensures front/back are the min/max vertices respectively
    for (size_t i = 0; i < std::pow(2,N); ++i ) {
      for (size_t dim = 0; dim < N; ++dim) {
        _vertices[i][dim] = (i & (1 << dim)) ? std::fmax(p[dim], q[dim]) : std::fmin(p[dim], q[dim]);
      }
    }
  }
    
  // returns true if p is contained in the hypercube
  virtual bool Contains(const Point<N,T>& p) const override { 
    for (size_t i = 0; i < N; ++i) {
      if ( FLT_LT(p[i], _vertices.front()[i]) || FLT_GT(p[i], _vertices.back()[i]) ) { return false; }
    }
    return true;
  }

  // check if this hypercube is fully in the halfplane defined by H
  virtual bool InHalfPlane(const Halfplane<N,T>& H) const override {
    return std::all_of(_vertices.begin(), _vertices.end(), [&H](const Point<N,T>& p) { return H.Contains(p); }); 
  }

  // check if this hypercube is fully in the hyperplane defined by H
  bool InNegativeHalfPlane(const AffineHyperplane<N,T>& H) const {
    return std::all_of( _vertices.begin(), _vertices.end(), [&H](const Point2f& p) { return FLT_LT(H.Evaluate(p), 0.f); } ); 
  }

  // if Hypercube center is contained in the ball, return true. Otherwise find the closest point on the perimeter
  // of the ball and check if is contained by the hypercube.
  bool Intersects(const Ball<N,T>& ball) const {
    if ( ball.Contains(GetCentroid()) ) { return true; }
    
    // get the closest point in the ball to the hypercube center
    Point<N,T> vec = GetCentroid() - ball.GetCentroid();
    vec.MakeUnitLength(); // this needs to be on its own line, since it returns the original length, not a point.
    return Contains( ball.GetCentroid() + (vec * ball.GetRadius()) );
  }

  // returns true if the given set FULLY contains all vertices
  bool IsContainedBy(const ConvexPointSet<N,T>& set) const {
    return std::all_of( _vertices.begin(), _vertices.end(),[&](const Point<N,T>& p) {return set.Contains(p);} );
  }  

  // helper for intersection checks
  inline Point<N,T> GetCentroid() const { return (_vertices.front() + _vertices.back()) * .5; }

protected:
  std::vector< Point<N,T> > _vertices;
};

// 2D implementation of Hypercube with some optimizations
class AxisAlignedQuad : public AxisAlignedHyperCube<2, float> {
public:
  using AxisAlignedHyperCube::Contains;
  using AxisAlignedHyperCube::Intersects;
  
  AxisAlignedQuad(const Point2f& p, const Point2f& q) : AxisAlignedHyperCube(p,q) {}
    
  // returns true if this node FULLY contains all poly vertices
  bool Contains(const FastPolygon& poly) const { 
    return std::all_of( poly.begin(), poly.end(),[this](const Point2f& p) {return Contains(p);} );
  }

  // calculates if the polygon intersects the node
  bool Intersects(const FastPolygon& poly) const
  {
    // check if any of the bounding box edges create a separating axis
    if (FLT_LT( poly.GetMaxX(), _vertices.front().x() )) { return false; }
    if (FLT_LT( poly.GetMaxY(), _vertices.front().y() )) { return false; }
    if (FLT_GT( poly.GetMinX(), _vertices.back().x()  )) { return false; }
    if (FLT_GT( poly.GetMinY(), _vertices.back().y()  )) { return false; }

    // fastPolygon line segments define the halfplane boundary of points inside the polygon, 
    // so check negative halfplane instead
    return std::none_of(poly.GetEdgeSegments().begin(), poly.GetEdgeSegments().end(), [&](const auto& l) { return InNegativeHalfPlane(l); });
  }
};

}

#endif
