/**
 * File: pointSet.h
 *
 * Author: Michael Willett
 * Created: 2018-02-07
 *
 * Description: Defines two interfaces, PointSet and ConvexPointSet, so that performance
 *              optimizations of the `Contains` functions can be an instance of the PointSet
 *              rather than the consumer of the PointSet. PointSet can be continuous or discrete, 
 *              however, by definition ConvexPointSet must be continuous with the exception of 
 *              unit or null sets.
 * 
 *              ConvexPointSets should obey convexity under Euclidean space, that is, for any two
 *              points p and q contained within set S, all points on the line segment defined by 
 *              p and q are also contained in S. 
 * 
 *              Since convex sets are frequently used to test linear constraint satisfiability,
 *              a helper function `InHalfPlane` is required to guarantee all points in the set
 *              obey the constraint:     ∀ x ∈ S : a'x + b > 0
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __COMMON_ENGINE_MATH_POINT_SET_H__
#define __COMMON_ENGINE_MATH_POINT_SET_H__

#include "coretech/common/engine/math/point.h"
#include <type_traits>

namespace Anki {

using DimType = size_t;

template<DimType N, typename T> 
class Halfplane;

template<DimType N, typename T> 
class AxisAlignedHyperCube;

// any representation of a collection of points that supports Contains checks for a point
template <DimType N, typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
class PointSet
{
public:
  virtual ~PointSet() {}

  // check if:    x ∈ S
  virtual bool Contains(const Point<N,T>& x) const = 0;
  
  // convenience function for checking contains on multiple points:    (x ∈ S) ∀ (x ∈ list)
  bool ContainsAll(const std::vector<Point<N,T>>& list) const {
    return std::all_of(list.begin(), list.end(), [this] (const Point<N,T>& x) { return this->Contains(x); });
  }
};

// Convex sets are any sets that are closed under convex combinations, of arithmetic types
// in C++, only floating point types are closed under convex combintations
template <DimType N, typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
class ConvexPointSet : public PointSet<N,T>
{
public:
  virtual ~ConvexPointSet() {} 
  
  // check if:    S ⊂ H
  virtual bool InHalfPlane(const Halfplane<N,T>& H) const = 0;
};

// if the points set can be bounded on all sides, then we can generate an AABB
template <DimType N, typename T>
class BoundedConvexSet : public ConvexPointSet<N,T> {
public:
  virtual ~BoundedConvexSet() {} 

  virtual AxisAlignedHyperCube<N,T> GetAxisAlignedBoundingBox() const = 0;

  // TODO: I don't know if this belongs here, but this is easiest for now
  virtual bool Intersects(const AxisAlignedHyperCube<N,T>& C) const = 0;
};


using PointSet2f         = PointSet<2, f32>;
using ConvexPointSet2f   = ConvexPointSet<2, f32>;
using BoundedConvexSet2f = BoundedConvexSet<2, f32>;

}

#endif
