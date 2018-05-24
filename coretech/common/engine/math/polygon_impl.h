/**
 * File: polygon_impl.h
 *
 * Author: Brad Neuman
 * Created: 2014-10-13
 *
 * Description: Implements a planar (2D) polygon in N-dimensional
 *              space. This is a container of points are in clockwise
 *              order
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __COMMON_BASESTATION_MATH_POLYGON_IMPL_H__
#define __COMMON_BASESTATION_MATH_POLYGON_IMPL_H__

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/rotatedRect.h"
#include "coretech/common/engine/utils/helpers/compareFcns.h"
#include "coretech/common/shared/utilities_shared.h"

#include <utility>
#include <limits>

namespace Anki {

template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon()
{
}

template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon(const Polygon<N,T>& other)
  : _points(other._points)
{
}

template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon( std::initializer_list< Point<N,T> > points )
  : _points(points)
{
}

template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon(const Polygon<N+1,T>& other)
{
  for( const auto& point : other ) {
    // use points N+1 constructor to drop the last dimension
    _points.emplace_back(point);
  }
}

template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon(const Rectangle<T>& rect)
  : _points{rect.GetTopLeft(), rect.GetTopRight(), rect.GetBottomRight(), rect.GetBottomLeft()}
{
  static_assert(N == 2, "Must use 2D for rectangles");
}
  
template <PolygonDimType N, typename T>
Polygon<N,T>::Polygon(const RotatedRectangle& rect)
{
  static_assert(N == 2, "Must use 2D for rotated rectangles");

  ImportQuad2d(rect.GetQuad());
}


template <PolygonDimType N, typename T>
void Polygon<N,T>::ImportQuad(const Quadrilateral<N,T>& quad)
{
  static_assert(N != 2, "Must use ImportQuad2D for 2d");

  Quadrilateral<N,T> sortedQuad ( quad.SortCornersClockwise( Z_AXIS_3D() ) );

  // For some reason, these need to be backwards for things to work...

  // _points.emplace_back(sortedQuad[Quad::TopRight]);
  // _points.emplace_back(sortedQuad[Quad::BottomRight]);
  // _points.emplace_back(sortedQuad[Quad::BottomLeft]);
  // _points.emplace_back(sortedQuad[Quad::TopLeft]);

  _points.emplace_back(sortedQuad[Quad::TopLeft]);
  _points.emplace_back(sortedQuad[Quad::BottomLeft]);
  _points.emplace_back(sortedQuad[Quad::BottomRight]);
  _points.emplace_back(sortedQuad[Quad::TopRight]);

}

template <PolygonDimType N, typename T>
void Polygon<N,T>::ImportQuad2d(const Quadrilateral<2,T>& quad)
{
  static_assert(N == 2, "Must use ImportQuad for > 2d");

  Quadrilateral<2,T> sortedQuad ( quad.SortCornersClockwise(  ) );

  // For some reason, these need to be backwards for things to work...

  // _points.emplace_back(sortedQuad[Quad::TopRight]);
  // _points.emplace_back(sortedQuad[Quad::BottomRight]);
  // _points.emplace_back(sortedQuad[Quad::BottomLeft]);
  // _points.emplace_back(sortedQuad[Quad::TopLeft]);

  _points.emplace_back(sortedQuad[Quad::TopLeft]);
  _points.emplace_back(sortedQuad[Quad::BottomLeft]);
  _points.emplace_back(sortedQuad[Quad::BottomRight]);
  _points.emplace_back(sortedQuad[Quad::TopRight]);

}


template <PolygonDimType N, typename T>
void Polygon<N,T>::Print() const
{
  // BN: tried to copy the format from Quadrilateral::Print
  CoreTechPrint("Polygon: ");
  for(const auto& point : _points) {
    CoreTechPrint("(");
    for(PolygonDimType i = 0; i < N; ++i) {
      CoreTechPrint(" %f", point[i]);
    }
    CoreTechPrint(")\n");
  }
}

template <PolygonDimType N, typename T>
T Polygon<N,T>::GetMinX(void) const
{
  T minX = std::numeric_limits<T>::max();
  for(const auto& it : _points) {
    if( it.x() < minX) {
      minX = it.x();
    }
  }

  return minX;
}

template <PolygonDimType N, typename T>
T Polygon<N,T>::GetMaxX(void) const
{
  T maxX = std::numeric_limits<T>::lowest();
  for(const auto& it : _points) {
    if( it.x() > maxX) {
      maxX = it.x();
    }
  }

  return maxX;
}

template <PolygonDimType N, typename T>
T Polygon<N,T>::GetMinY(void) const
{
  T minX = std::numeric_limits<T>::max();
  for(const auto& it : _points) {
    if( it.y() < minX) {
      minX = it.y();
    }
  }

  return minX;
}

template <PolygonDimType N, typename T>
T Polygon<N,T>::GetMaxY(void) const
{
  T maxX = std::numeric_limits<T>::lowest();
  for(const auto& it : _points) {
    if( it.y() > maxX) {
      maxX = it.y();
    }
  }

  return maxX;
}

template <PolygonDimType N, typename T>
void Polygon<N,T>::reserve(size_t n)
{
  _points.reserve(n);
}

template <PolygonDimType N, typename T>
void Polygon<N,T>::push_back(const Point<N, T>& val)
{
  _points.push_back(val);
}

template <PolygonDimType N, typename T>
void Polygon<N,T>::push_back(Point<N, T>&& val)
{
  _points.push_back(std::forward< Point<N,T> >(val));
}

template <PolygonDimType N, typename T>
void Polygon<N,T>::emplace_back(Point<N, T>&& val)
{
  _points.emplace_back(std::forward< Point<N,T> >(val));
}

template <PolygonDimType N, typename T>
void Polygon<N,T>::pop_back()
{
  _points.pop_back();
}

template <PolygonDimType N, typename T>
size_t Polygon<N,T>::size() const
{
  return _points.size();
}

template <PolygonDimType N, typename T>
Point<N,T>& Polygon<N,T>::operator[] (size_t idx)
{
  return _points[idx];
}

template <PolygonDimType N, typename T>
const Point<N,T>& Polygon<N,T>::operator[] (size_t idx) const
{
  return _points[idx];
}


template <PolygonDimType N, typename T>
T Polygon<N,T>::GetEdgeAngle(size_t idx) const
{
  assert(!_points.empty());

  size_t idx2 = (idx + 1) % _points.size();
  return atan2(_points[idx2].y() - _points[idx].y(), _points[idx2].x() - _points[idx].x());
}

template <PolygonDimType N, typename T>
Point<N,T> Polygon<N,T>::GetEdgeVector(size_t idx) const
{
  assert(!_points.empty());

  size_t idx2 = (idx + 1) % _points.size();
  return _points[idx2] - _points[idx];
}

template <PolygonDimType N, typename T>
Point<N,T> Polygon<N,T>::ComputeCentroid() const
{
  Point<N, T> ret;

  for(PolygonDimType dim = 0; dim < N; ++dim) {
    ret[dim] = (T) 0;
  }

  // NOTE: this won't work for integer types!
  T oneOverNum = ((T) 1.0) / ((T) _points.size());

  for(const auto& point : _points) {
    for(PolygonDimType dim = 0; dim < N; ++dim) {
      ret[dim] += point[dim] * oneOverNum;
    }
  }

  return ret;
}


}


#endif
