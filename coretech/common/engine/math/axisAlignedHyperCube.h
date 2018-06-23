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

#include "coretech/common/engine/math/affineHyperplane.h"

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

  // returns true if A is contained in the hypercube
  bool Contains(const AxisAlignedHyperCube<N,T>& A) const { 
    return Contains(A.GetMinVertex()) && Contains(A.GetMaxVertex());
  }  

  // helper for intersection checks
  inline Point<N,T> GetCentroid() const { return (_vertices.front() + _vertices.back()) * .5; }

  // get the min/max vertices
  const Point<N,T>& GetMinVertex() const { return _vertices.front(); } 
  const Point<N,T>& GetMaxVertex() const { return _vertices.back();  } 

  // get all vertices
  inline const std::vector<Point<N,T>>& GetVertices() const { return _vertices; }

protected:
  std::vector< Point<N,T> > _vertices;
};

using AxisAlignedQuad = AxisAlignedHyperCube<2, float>;


}

#endif
