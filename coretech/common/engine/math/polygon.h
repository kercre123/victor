/**
 * File: polygon.h
 *
 * Author: Brad Neuman
 * Created: 2014-10-13
 *
 * Description: Defines a planar (2D) polygon in N-dimensional
 *              space. This is a container of points are in clockwise
 *              order
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __COMMON_BASESTATION_MATH_POLYGON_H__
#define __COMMON_BASESTATION_MATH_POLYGON_H__

#include "coretech/common/engine/math/point.h"

#include "clad/types/cladPoint.h"

#include <vector>

namespace Anki {

using PolygonDimType = size_t;

template<PolygonDimType N, typename T>
class Quadrilateral;

template<typename T>
class Rectangle;

class RotatedRectangle;

template <PolygonDimType N, typename T>
class Polygon
{
protected:
  using PointContainer = std::vector< Point<N,T> >;

public:

  virtual ~Polygon() = default;

  Polygon();
  Polygon(const Polygon<N,T>& other);

  // Initialize polygon from list of points. Assumes points are already in clockwise order!
  Polygon(std::initializer_list< Point<N,T> > points);

  // Construct from polygon living in one dimension higher. Last
  // dimension simply gets dropped. For example, this allows
  // construction of a 2D quad from a 3D polygon, by simply using the
  // (x,y) coordinates and ignoring z.
  Polygon(const Polygon<N+1,T>& other);

  // convert from common 2d objects
  explicit Polygon(const Rectangle<T>& rect);
  explicit Polygon(const RotatedRectangle& rect);
  explicit Polygon(const Quadrilateral<2, T>& quad);

  // Initialize 2D/3D polygons from a list of clad points. Assumes points are already in clockwise order!
  Polygon(const std::vector<CladPoint2d>& cladPoints);
  Polygon(const std::vector<CladPoint3d>& cladPoints);

  // Get a vector of CladPoints
  std::vector<CladPoint2d> ToCladPoint2dVector() const;
  std::vector<CladPoint3d> ToCladPoint3dVector() const;

  // Import from a 2D quad
  void ImportQuad2d(const Quadrilateral<2, T>& quad);

  // Import from a quadrilateral > 2D
  void ImportQuad(const Quadrilateral<N, T>& quad);

  void Print() const;

  // TODO:(bn) define equality operators?

  // Get min/max coordinates (e.g. for bounding box)
  virtual T GetMinX(void) const;
  virtual T GetMinY(void) const;
  virtual T GetMaxX(void) const;
  virtual T GetMaxY(void) const;

  // TODO:(bn) implement the rest of the helper functions like in Quad (e.g. Contains point, intersects, etc...)

  // computes the angle between point at idx and point at idx + 1 (wrapping around to 0 at the end)
  T GetEdgeAngle(size_t idx) const;

  // Returns the vector pointing from the point at idx to the point at idx + 1 (wrapping around at the end)
  Point<N,T> GetEdgeVector(size_t idx) const;

  // Compute the weighted average of the points. NOTE: is currently broken for integral types!
  Point<N,T> ComputeWeightedAverage() const;

  ////////////////////////////////////////////////////////////////////////////////
  // container functions:
  ////////////////////////////////////////////////////////////////////////////////

  typename PointContainer::iterator begin() {return _points.begin();}
  typename PointContainer::iterator end() {return _points.end();}
  typename PointContainer::const_iterator begin() const {return _points.begin();}
  typename PointContainer::const_iterator end() const {return _points.end();}

  // reserves/allocates memory for n Points, according to the rules of underlying container, currently std::vector
  void reserve(size_t n);

  void push_back(const Point<N, T>& val);
  void push_back(Point<N, T>&& val);
  void emplace_back(Point<N, T>&& val);

  void pop_back();

  virtual       Point<N,T>& operator[] (size_t idx);
  virtual const Point<N,T>& operator[] (size_t idx) const;

  size_t size() const;

protected:

  PointContainer _points;

};

using Poly2f = Polygon<2, f32>;
using Poly3f = Polygon<3, f32>;
using Poly2i = Polygon<2, s32>;
using Poly3i = Polygon<3, s32>;

}

#endif
