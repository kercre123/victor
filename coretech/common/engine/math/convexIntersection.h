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

template <DimType N, typename T, class S1, class S2, typename = typename std::enable_if<
  (std::is_base_of<PointSet<N,T>, S1>::value && std::is_base_of<PointSet<N,T>, S2>::value)
>::type>
class ConvexIntersection : public BoundedConvexSet<N,T>
{
public:
  ConvexIntersection(S1 set1, S2 set2) : _set1(set1), _set2(set2) {}
  virtual ~ConvexIntersection() {}

  virtual bool Contains(const Point<N,T>& x) const override {
    return _set1.Contains(x) && _set2.Contains(x);
  }
  
  bool ContainsHyperCube(const AxisAlignedHyperCube<N,T>& C) const {
    // NOTE: this is merely a sufficient condition, but not necessary
    return SetContains(_set1, C) && SetContains(_set2, C);
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
  // helpers for choosing the correct Contains for Hypercubes
  template<class Set, typename = typename std::enable_if<std::is_base_of<ConvexPointSet<N,T>, Set>::value>::type>
  inline static bool SetContains(const Set& S, const AxisAlignedHyperCube<N,T>& C) {
    return S.ContainsAll(C.GetVertices());
  }

  // fall through for Intersection types
  template<class M1, class M2>
  inline static bool SetContains(const ConvexIntersection<N,T,M1,M2>& S, const AxisAlignedHyperCube<N,T>& C) {
    return S.ContainsHyperCube(C);
  }

  S1 _set1;
  S2 _set2;
};

template <typename S1, typename S2>
using ConvexIntersection2f = ConvexIntersection<2, float, S1, S2>;

// helper for making intersections of multiple sets via recursion: (S₁ ⋂ (S₂ ⋂ (S₃ ⋂ (... ⋂ Sₓ))))
template<DimType N, typename T, class S>
S MakeIntersection(S v) { return v; };

template<DimType N, typename T, class S, typename... Args>
decltype(auto) MakeIntersection(S head, Args... tail) {
  auto retv = MakeIntersection<N,T,Args...>(tail...);
  return ConvexIntersection<N, T, S, decltype(retv)>(head, retv);
};

template <typename... Args>
decltype(auto) MakeIntersection2f(Args... sets) {
  return MakeIntersection<2, float, Args...>(sets...);
}

}

#endif
