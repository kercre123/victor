/**
 * File: pointSetUnion.h
 *
 * Author: Michael Willett
 * Created: 2018-07-15
 *
 * Description: Defines an the union of two point sets sets
 *                      S₁ ∪ S₂ = { p ∈ ℝⁿ | p ∈ S₁ or p ∈ S₂ } 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_POINTSET_UNION_H__
#define __COMMON_ENGINE_MATH_POINTSET_UNION_H__


#include "coretech/common/engine/math/pointSet.h"
#include "coretech/common/engine/math/axisAlignedHyperCube.h"

namespace Anki {

// TODO: probably should add some typetrait checks to make sure GetAABB and intersects are all
//       valid for arbitrary instances, but this should be fine for now

template <DimType N, typename T, class S1, class S2, typename = typename std::enable_if<
  (std::is_base_of<PointSet<N,T>, S1>::value && std::is_base_of<PointSet<N,T>, S2>::value)
>::type>
class PointSetUnion : public PointSet<N,T>
{
public:
  PointSetUnion(S1 set1, S2 set2) : _set1(set1), _set2(set2) {}
  virtual ~PointSetUnion() {}

  virtual bool Contains(const Point<N,T>& x) const override {
    return _set1.Contains(x) || _set2.Contains(x);
  }
  
  bool ContainsHyperCube(const AxisAlignedHyperCube<N,T>& C) const {
    // NOTE: this is merely a sufficient condition, but not necessary
    return SetContains(_set1, C) || SetContains(_set2, C);
  }

  bool Intersects(const AxisAlignedHyperCube<N,T>& C) const {
    return this->_set1.Intersects(C) || this->_set2.Intersects(C);
  }

  AxisAlignedHyperCube<N,T> GetAxisAlignedBoundingBox() const
  {
    // both Bounded types, so merge results from `GetAxisAlignedBoundingBox`
    const auto& s1BoundingBox = this->_set1.GetAxisAlignedBoundingBox();
    const auto& s2BoundingBox = this->_set2.GetAxisAlignedBoundingBox();
    Point<N,T> min = s1BoundingBox.GetMinVertex();
    Point<N,T> max = s1BoundingBox.GetMaxVertex();

    for (DimType dim = 0; dim < N; ++dim) {
      if(s2BoundingBox.GetMinVertex()[dim] < min[dim]) { min[dim] = s2BoundingBox.GetMinVertex()[dim]; }
      if(s2BoundingBox.GetMaxVertex()[dim] > max[dim]) { max[dim] = s2BoundingBox.GetMaxVertex()[dim]; }
    }

    return AxisAlignedHyperCube<N,T>(min, max);
  }

protected:
  // helpers for choosing the correct Contains for Hypercubes
  template<class Set, typename = typename std::enable_if<std::is_base_of<ConvexPointSet<N,T>, Set>::value>::type>
  inline static bool SetContains(const Set& S, const AxisAlignedHyperCube<N,T>& C) {
    return S.ContainsAll(C.GetVertices());
  }

  // fall through for Union types
  template<class M1, class M2>
  inline static bool SetContains(const PointSetUnion<N,T,M1,M2>& S, const AxisAlignedHyperCube<N,T>& C) {
    return S.ContainsHyperCube(C);
  }

  S1 _set1;
  S2 _set2;
};

template <typename S1, typename S2>
using PointSetUnion2f = PointSetUnion<2, float, S1, S2>;

// helper for making unions of multiple sets via recursion: (S₁ ∪ (S₂ ∪ (S₃ ∪ (... ∪ Sₓ))))
template<DimType N, typename T, class S>
S MakeUnion(S v) { return v; };

template<DimType N, typename T, class S, typename... Args>
decltype(auto) MakeUnion(S head, Args... tail) {
  auto retv = MakeUnion<N,T,Args...>(tail...);
  return PointSetUnion<N, T, S, decltype(retv)>(head, retv);
};

template <typename... Args>
decltype(auto) MakeUnion2f(Args... sets) {
  return MakeUnion<2, float, Args...>(sets...);
}

}

#endif
