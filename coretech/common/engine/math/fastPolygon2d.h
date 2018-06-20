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

// TODO: ConvexPolygon should inherit from ConvexPointSet, but that requires moving Contains
//       and InHalfplane checks to ConvexPolygon class.
class FastPolygon : public ConvexPolygon, public BoundedConvexSet<2, f32>
{
public:

  // must be constructed from an existing polygon
  FastPolygon(const Poly2f& basePolygon);

  // Returns true if the point (x,y) is inside this polygon, false otherwise
  bool Contains(float x, float y) const;

  // convenience function
  virtual bool Contains(const Point2f& pt) const override;
  
  // returns true if the convex polygon is fully contained by the halfplane H
  virtual bool InHalfPlane(const Halfplane2f& H) const override;
  
  // intersection with AABB
  virtual bool Intersects(const AxisAlignedQuad& quad) const override;
  
  const std::vector<LineSegment>& GetEdgeSegments() const { return _edgeSegments; }
  
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

  virtual float GetMinX() const override {return _minX;}
  virtual float GetMaxX() const override {return _maxX;}
  virtual float GetMinY() const override {return _minY;}
  virtual float GetMaxY() const override {return _maxY;}

  // get the AABB for this poly
  inline virtual AxisAlignedQuad GetAxisAlignedBoundingBox() const override {
    return AxisAlignedQuad({GetMinX(), GetMinY()}, {GetMaxX(), GetMaxY()});
  }

  static void ResetCounts();
  static int _numChecks;
  static int _numDotProducts;
  
  
#if USE_LINESEGMENT_CHECKS
  // checks if line intersects polygon
  bool Intersects(const LineSegment& l) const;
#endif

private:
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
