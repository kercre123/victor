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

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/polygon.h"
#include "anki/common/basestation/utils/helpers/compareFcns.h"
#include "anki/common/shared/utilities_shared.h"

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
Polygon<N,T>::Polygon(const Quadrilateral<N,T>& quad)
{
  _points.emplace_back(quad[Quad::TopRight]);
  _points.emplace_back(quad[Quad::BottomRight]);
  _points.emplace_back(quad[Quad::BottomLeft]);
  _points.emplace_back(quad[Quad::TopLeft]);
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
  T maxX = std::numeric_limits<T>::min();
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
  T maxX = std::numeric_limits<T>::min();
  for(const auto& it : _points) {
    if( it.y() > maxX) {
      maxX = it.y();
    }
  }

  return maxX;
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


}


#endif
