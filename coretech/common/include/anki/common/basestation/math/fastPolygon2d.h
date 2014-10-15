/**
 * File: fastPolygon2d.h
 *
 * Author: Brad Neuman
 * Created: 2014-10-14
 *
 * Description: A class that holds a 2d polygon and a bunch of other
 * data in order to do a really fast "contains" check
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __COMMON_BASESTATION_MATH_FASTPOLYGON2D_H__
#define __COMMON_BASESTATION_MATH_FASTPOLYGON2D_H__

#include "anki/common/basestation/math/polygon.h"
#include "anki/common/basestation/math/point.h"

#include <vector>

namespace Anki {

class FastPolygon
{
public:

  // must be constructed from an existing polygon
  FastPolygon(const Poly2f& basePolygon);

  // Returns true if the point (x,y) is inside this polygon, false otherwise
  bool Contains(const Point2f& testPoint) const;

  const Poly2f& GetSimplePolygon() const {return _poly;}

  // getters for testing / plotting

  // Returns the center point of the circles used for quick checks
  // (useful for plotting)
  const Point2f& GetCheckCenter() const {return _circleCenter;}

  // returns radius of the circles used for checking (useful for
  // plotting)
  float GetCircumscribedRadius() const;
  float GetInscribedRadius() const;

  static int _numChecks;
  static int _numDotProducts;

private:

  const Poly2f _poly;

  // keep track of two circles, centered at the same point (for
  // now). The circumscribed circle is the smallest one (given the
  // fixed center) that is outside the polygon. The inscribed circle
  // is the largest one inside the poly. This way, by doing two simple
  // circle checks, we can throw away points definitely not colliding,
  // and points definitely colliding, and only do the more expensive
  // check for the ones in between
  Point2f _circleCenter;
  float _circumscribedRadiusSquared;
  float _inscribedRadiusSquared;

  // for the points that are between the circles, we need to check
  // that the point is inside the polygon by checking if the point is
  // "to the left" of each edge. For each edge, this vector holds a
  // perpendicular unit-length vector. When the test point is
  // projected onto this vector, the sign will determine if the point
  // passes the test for this edge. This list of edges is sorted
  // according to a heuristic which tries to put the more imformative
  // edges first, so we can throw out a point as not collising as
  // quickly as posisble. The test must "pass" for every single
  // element of the list for this to be a collision
  std::vector< Vec2f > _perpendicularEdgeVectors;

  // helper functions

  void ComputeCenter();

  void ComputeCircles();

  // create (unsorted) edge vectors
  void CreateEdgeVectors();

  // sort the existing edge vectors
  void SortEdgeVectors();

};

}

#endif
