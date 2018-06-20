/**
 * File: convexIntersection.h
 *
 * Author: Michael Willett
 * Created: 2018-06-15
 *
 * Description: Defines an the intersection of an arbitrary number of bounded convex sets
 *                      S₁ ⋂ S₂ ⋂ ... ⋂ Sₓ  = { p ∈ ℝⁿ | p ∈ S₁, p ∈ S₂, p ∈ Sₓ } 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_CONVEX_INTERSECTION_H__
#define __COMMON_ENGINE_MATH_CONVEX_INTERSECTION_H__


#include "coretech/common/engine/math/pointSet.h"
#include "coretech/common/engine/math/axisAlignedHyperCube.h"

namespace Anki {

template <DimType N, typename T>
class ConvexIntersection : public BoundedConvexSet<N,T> 
{
public:
  ConvexIntersection() : _sets() {}
  virtual ~ConvexIntersection() {}

  template<class Derived, typename = typename std::enable_if<std::is_base_of<BoundedConvexSet<N,T>, Derived>::value>::type>
  void AddSet(const Derived& p) {
    _sets.push_back( std::make_unique<Derived>(p) );
  }

  virtual bool InHalfPlane(const Halfplane<N,T>& H) const override {
    return std::all_of(_sets.begin(), _sets.end(), [&H] (const auto& S) { return S->InHalfPlane(H); });
  }

  virtual bool Contains(const Point<N,T>& x) const override {
    return std::all_of(_sets.begin(), _sets.end(), [&x] (const auto& S) { return S->Contains(x); });
  }

  virtual bool Intersects(const AxisAlignedHyperCube<N,T>& C) const override {
    return std::all_of(_sets.begin(), _sets.end(), [&C] (const auto& S) { return S->Intersects(C); });
  }

  virtual AxisAlignedHyperCube<N,T> GetAxisAlignedBoundingBox() const override 
  {
    Point<N,T> min(std::numeric_limits<T>::lowest());
    Point<N,T> max(std::numeric_limits<T>::max());
    for (const auto& S : _sets) {
      const auto& sBoundingBox = S->GetAxisAlignedBoundingBox();
      for (DimType dim = 0; dim < N; ++dim) {
        if(sBoundingBox.GetMinVertex()[dim] > min[dim]) { min[dim] = sBoundingBox.GetMinVertex()[dim]; }
        if(sBoundingBox.GetMaxVertex()[dim] < max[dim]) { max[dim] = sBoundingBox.GetMaxVertex()[dim]; }
      }
    }

    return AxisAlignedHyperCube<N,T>(min, max);
  }

private:
  std::vector< std::unique_ptr<BoundedConvexSet2f> > _sets;
};

using ConvexIntersection2f = ConvexIntersection<2, float>;

}

#endif
