/**
 * File: convexIntersection.h
 *
 * Author: Michael Willett
 * Created: 2018-06-15
 *
 * Description: Defines an the intersection of an arbitrary number of bounded convex sets
 *                      S₁ ⋂ S₂ ⋂ ... ⋂ Sₓ  = { p ∈ ℝⁿ | p ∈ S₁, p ∈ S₂, ..., p ∈ Sₓ } 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_CONVEX_INTERSECTION_H__
#define __COMMON_ENGINE_MATH_CONVEX_INTERSECTION_H__


#include "coretech/common/engine/math/pointSet.h"
#include "coretech/common/engine/math/axisAlignedHyperCube.h"

namespace Anki {

template <DimType N, typename T, class Set1, class Set2, typename = typename std::enable_if<
  (std::is_base_of<PointSet<N,T>, Set1>::value && std::is_base_of<PointSet<N,T>, Set2>::value)
>::type>
class ConvexIntersection : public BoundedConvexSet<N,T>
{
public:
  ConvexIntersection(Set1 set1, Set2 set2) : _set1(set1), _set2(set2) {}
  virtual ~ConvexIntersection() {}

  virtual bool Contains(const Point<N,T>& x) const override {
    return _set1.Contains(x) && _set2.Contains(x);
  }
  
  virtual bool Intersects(const AxisAlignedHyperCube<N,T>& C) const override {
    return this->_set1.Intersects(C) && this->_set2.Intersects(C);
  }

  virtual bool InHalfPlane(const Halfplane<N,T>& H) const override {
    return this->_set1.InHalfPlane(H) && this->_set2.InHalfPlane(H);
  }

  virtual AxisAlignedHyperCube<N,T> GetAxisAlignedBoundingBox() const override 
  {
    // both Bounded types, so merge results from `GetAxisAlignedBoundingBox`
    const auto& s1BoundingBox = this->_set1.GetAxisAlignedBoundingBox();
    const auto& s2BoundingBox = this->_set2.GetAxisAlignedBoundingBox();
    Point<N,T> min = s1BoundingBox.GetMinVertex();
    Point<N,T> max = s1BoundingBox.GetMaxVertex();

    for (DimType dim = 0; dim < N; ++dim) {
      if(s2BoundingBox.GetMinVertex()[dim] > min[dim]) { min[dim] = s2BoundingBox.GetMinVertex()[dim]; }
      if(s2BoundingBox.GetMaxVertex()[dim] < max[dim]) { max[dim] = s2BoundingBox.GetMaxVertex()[dim]; }
    }

    return AxisAlignedHyperCube<N,T>(min, max);
  }

protected:
  Set1 _set1;
  Set2 _set2;
};

template <typename Set1, typename Set2>
using ConvexIntersection2f = ConvexIntersection<2, float, Set1, Set2>;

// helper for making intersections of multiple sets via recursion: (S₁ ⋂ (S₂ ⋂ (S₃ ⋂ (... ⋂ Sₓ))))
template<DimType N, typename T, class Set>
Set MakeIntersection(Set v) { return v; };

template<DimType N, typename T, class Set, typename... Args>
decltype(auto) MakeIntersection(Set head, Args... tail) {
  auto retv = MakeIntersection<N,T,Args...>(tail...);
  return ConvexIntersection<N, T, Set, decltype(retv)>(head, retv);
};

template <typename... Args>
decltype(auto) MakeIntersection2f(Args... sets) {
  return MakeIntersection<2, float, Args...>(sets...);
}

}

#endif
