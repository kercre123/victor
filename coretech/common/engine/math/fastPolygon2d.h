/**
 * File: fastPolygon2d.h
 *
 * Author: Brad Neuman
 * Created: 2014-10-14
 *
 * Description: A class that holds a 2d polygon and a bunch of other
 * data in order to do a really fast "contains" check. 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __COMMON_BASESTATION_MATH_FASTPOLYGON2D_H__
#define __COMMON_BASESTATION_MATH_FASTPOLYGON2D_H__

#include "coretech/common/engine/math/polygon.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/lineSegment2d.h"
#include "coretech/common/engine/math/convexPolygon2d.h"

#include <vector>

// NOTE: (mrw) The initial implementation of the FastPolygon class 
// assumes both that the input polygon is convex, and that the input
// polygon has all points sorted in clockwise order. Failing either
// of these criteria results in eronious contains checks.
//
// There is alternative polygon contains implementation that does
// not require these assumptions to hold, but is generally slower.
// It does, however, contain a fast line intersection check, which
// may result in greater overall speedup of the path planner code
// than the loss of time due to worse performance of the point checks.
// Further improvements could be made by filtering the line segments 
// by vertical span.
//
// To use the slower LineSegment based implementation, set
// USE_LINESEGMENT_CHECKS = 1 
#define USE_LINESEGMENT_CHECKS 0

namespace Anki {

class FastPolygon
{
public:

  // must be constructed from an existing polygon
  FastPolygon(const Poly2f& basePolygon);

  // Returns true if the point (x,y) is inside this polygon, false otherwise
  bool Contains(float x, float y) const;

  // convenience function
  bool Contains(const Point2f& pt) const;

  const Poly2f& GetSimplePolygon() const {return _poly.GetSimplePolygon();}
  
  const Point2f& operator[] (size_t idx) const {return _poly[idx];}
  
  const std::vector<LineSegment>& GetEdgeSegments() const { return _edgeSegments; }
  
  size_t Size() const {return _poly.size();}

  // getters for testing / plotting

  // Returns the center point of the circles used for quick checks
  // (useful for plotting)
  const Point2f& GetCheckCenter() const {return _circleCenter;}

  // returns radius of the circles used for checking (useful for
  // plotting)
  float GetCircumscribedRadius() const;
  float GetInscribedRadius() const;

  // This is an optional step that spends a little time to optimize
  // the ordering of the edges, so that collision checks will tend to
  // be faster
  void SortEdgeVectors();

  float GetMinX() const {return _minX;}
  float GetMaxX() const {return _maxX;}
  float GetMinY() const {return _minY;}
  float GetMaxY() const {return _maxY;}

  static void ResetCounts();
  static int _numChecks;
  static int _numDotProducts;
  
  
#if USE_LINESEGMENT_CHECKS
  // checks if line intersects polygon
  bool Intersects(const LineSegment& l) const;
#endif

private:
  // ConvexPolygon constructor enforces CW direction and convexity
  const ConvexPolygon _poly;

  // outermost bounding-box of poly
  float _minX;
  float _maxX;
  float _minY;
  float _maxY;

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
  // passes the test for this edge. The test must "pass" for every
  // single element of the list for this to be a collision. The first
  // element of the pair is the vector, and the second is the
  // corresponding index in _poly for the point defining the start of
  // the vector. The optional SortEdgeVectors will re-order this for
  // efficiency
  std::vector< std::pair< Vec2f, size_t> > _perpendicularEdgeVectors;
  std::vector< LineSegment > _edgeSegments;
  


  // helper functions

  void ComputeCenter();

  void ComputeCircles();

  // create (unsorted) edge vectors
  void CreateEdgeVectors();
  
#if USE_LINESEGMENT_CHECKS
  bool Contains_Assumptionless(const Point2f& p) const;
#endif

  // helper for sorting function. Returns number of new points hit by
  // the specified edge index. If dryrun is false, it will update the
  // test points to mark that it has hit the points
  unsigned int CheckTestPoints(std::vector< std::pair< bool, Point2f > >& testPoints,
                               size_t edgeIdx,
                               bool dryRun);
  

};

}

#endif
