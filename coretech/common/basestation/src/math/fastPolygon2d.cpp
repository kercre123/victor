/**
 * File: fastPolygon2d.cpp
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

#include "anki/common/basestation/math/fastPolygon2d.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/utils/helpers/boundedWhile.h"

#include <cmath>
#include <cfloat>

namespace Anki {

int FastPolygon::_numChecks = 0;
int FastPolygon::_numDotProducts = 0;

void FastPolygon::ResetCounts()
{
  FastPolygon::_numChecks = 0;
  FastPolygon::_numDotProducts = 0;
}

FastPolygon::FastPolygon(const Poly2f& basePolygon)
  : _poly(basePolygon)
{
  CreateEdgeVectors();

  ComputeCenter();

  ComputeCircles();
}



/*
 * NOTE: TODO: (bn): There is another method that could be faster,
 * here:
 *
 * One you know that the point is in between the two circles (centered
 * at the same center), then you only need to do 1 dot product check!
 * You simply need to look at the angle between the center and the
 * test point. then check which edge corresponds to that angle, and
 * just check that one edge. The other edges can't matter.
 *
 * Imagine tracing a ray from the center, through the test point, and
 * out. That point is inside the polygon if and only if the ray
 * crosses the point before it crosses an edge. We can pre-compute
 * which edge it will cross by computing for each edge the start and
 * stop angle. Then we can just search through these until we find the
 * correct edge, and just do one dot product!
 *
 * This does require an atan2, but we could probably use a very rough
 * and fast approximation and just check two or three edges and
 * probably save time
 *
 */

bool FastPolygon::Contains(float x, float y) const
{
  // the goal here is to throw out points as quickly as
  // possible. First compute the squared distance to the center, and
  // do circle checks

  _numChecks++;

  float dy = y - _circleCenter.y();
  float dx = x - _circleCenter.x();

  float distSqaured = SQUARE(dx) + SQUARE(dy);

  if(distSqaured > _circumscribedRadiusSquared) {
    // definitely not inside
    return false;
  }

  if(distSqaured < _inscribedRadiusSquared) {
    // definitely is inside
    return true;
  }

  // otherwise we have to check the actual edges.

  size_t numPts = _poly.size();
  assert(_perpendicularEdgeVectors.size() == numPts);

  //unsigned int numChecked = 0;

  for(size_t i = 0; i < numPts; ++i) {
    // if the dot product is positive, the test point is inside of the
    // edge, so we must continue. Otherwise we can bail out early
    size_t pointIdx = _perpendicularEdgeVectors[i].second;

    // dot product of perpendicular vector and (x,y) - polygonPoint

    float dot = _perpendicularEdgeVectors[i].first.x() * ( x - _poly[pointIdx].x() ) +
      _perpendicularEdgeVectors[i].first.y() * ( y - _poly[pointIdx].y() ) ;

    // float dot = Anki::DotProduct( _perpendicularEdgeVectors[i].first, testPoint - _poly[pointIdx] );

    _numDotProducts++;

    if( dot > 0.0f ) {
      // was inside circle, but outside of an edge
      return false;
    }
  }

  // was inside of outer circle, and inside of all edges, so is inside
  return true;
}

bool FastPolygon::Contains(const Point2f& pt) const
{
  return Contains(pt.x(), pt.y());
}


float FastPolygon::GetCircumscribedRadius() const
{
  return std::sqrt(_circumscribedRadiusSquared);
}

float FastPolygon::GetInscribedRadius() const
{
  return std::sqrt(_inscribedRadiusSquared);
}

////////////////////////////////////////////////////////////////////////////////
// internal helpers
////////////////////////////////////////////////////////////////////////////////

void FastPolygon::ComputeCenter()
{
  // for now just use the geometric center
  // TODO:(bn) something smarter to get a better set of circles

  _circleCenter = _poly.ComputeCentroid();
}

void FastPolygon::ComputeCircles()
{
  // go through each point and compute two things. First compute the
  // distance to the point, the maximum of which will be the min
  // circumscribing radius. Also compute the distance to the closest
  // point on each line (using the perpendicular edge vectors) and the
  // minimum of those is the inscribing radius
  assert(!_perpendicularEdgeVectors.empty());

  _circumscribedRadiusSquared = 0.0f;
  _inscribedRadiusSquared = FLT_MAX;

  size_t numPts = _poly.size();
  assert(_perpendicularEdgeVectors.size() == numPts);

  for(size_t i = 0; i < numPts; ++i) {
    float distanceToPointSquared =
      std::pow( _poly[i].x() - _circleCenter.x(), 2 ) +
      std::pow( _poly[i].y() - _circleCenter.y(), 2 );

    if( distanceToPointSquared > _circumscribedRadiusSquared ) {
      _circumscribedRadiusSquared = distanceToPointSquared;
    }

    size_t pointIdx = _perpendicularEdgeVectors[i].second;

    float distanceToEdgeSquared = std::pow(
      Anki::DotProduct( _perpendicularEdgeVectors[i].first, _circleCenter - _poly[pointIdx] ),
      2);
    if(distanceToEdgeSquared < _inscribedRadiusSquared) {
      _inscribedRadiusSquared = distanceToEdgeSquared;
    }
  }

  assert(_circumscribedRadiusSquared > 0.0f);
  assert(_inscribedRadiusSquared > 0.0f);

  if( _inscribedRadiusSquared >= _circumscribedRadiusSquared ) {
    CoreTechPrint("ERROR: inscribed radius of %f is >= circumscribed radius of %f\n",
                  _inscribedRadiusSquared,
                  _circumscribedRadiusSquared);
  }
}

void FastPolygon::CreateEdgeVectors()
{
  _perpendicularEdgeVectors.clear();

  size_t numPts = _poly.size();

  for(size_t i = 0; i < numPts; ++i) {
    const Vec2f edgeVector( _poly.GetEdgeVector(i) );
    float oneOverNorm = 1.0 / edgeVector.Length();
    _perpendicularEdgeVectors.emplace_back( std::make_pair(
                                              Vec2f{ -edgeVector.y() * oneOverNorm, edgeVector.x() * oneOverNorm },
                                              i) );
  }
}

void FastPolygon::SortEdgeVectors()
{
  // Sort the edges into an order that will help quickly eliminate
  // points as not colliding.

  // Create 16 test points along the circumscribing circle, and
  // another 16 in between the two circles. These will be used to
  // determine how good each line is at discriminating points. The
  // bool is true if the point has already been eliminated by another
  // line

  std::vector< std::pair< bool, Point2f > > testPoints;

  float outerRadius = GetCircumscribedRadius();

  // inner radius is between the two, closer to the inside
  float innerRadius = 0.3*outerRadius + 0.7*GetInscribedRadius();

  float stepSize = 2 * M_PI / 16;
  for(float theta = 0; theta <= 2*M_PI + 1e-5; theta += stepSize) {
    testPoints.emplace_back( std::make_pair( false,
                                             Point2f{ outerRadius * cosf(theta) + _circleCenter.x(),
                                                      outerRadius * sinf(theta) + _circleCenter.y() }));
    testPoints.emplace_back( std::make_pair( false,
                                             Point2f{ innerRadius * cosf(theta) + _circleCenter.x(),
                                                      innerRadius * sinf(theta) + _circleCenter.y() }));
  }

  
  std::vector< std::pair< Vec2f, size_t> > sortedEdges;

  size_t numEdges = _perpendicularEdgeVectors.size();
  assert(numEdges > 0);

  std::vector<bool> usedEdge(numEdges, false);

  BOUNDED_WHILE( 1000, sortedEdges.size() < numEdges ) {
    // try each edge and check how many points it would add
    unsigned int maxHits = 0;
    size_t bestIdx = 0;

    for( size_t i=0; i < numEdges; ++i ) {
      if( usedEdge[i] )
        continue;
      unsigned int newHits = CheckTestPoints(testPoints, i, true);
      if(newHits >= maxHits) {
        maxHits = newHits;
        bestIdx = i;
      }
    }

    // CoreTechPrint("DEBUG: adding edge %d with %d new hits\n", bestIdx, maxHits);

    usedEdge[bestIdx] = true;
    sortedEdges.push_back( _perpendicularEdgeVectors[bestIdx] );

    // mark test points
    CheckTestPoints(testPoints, bestIdx, false);
  }

  _perpendicularEdgeVectors.swap(sortedEdges);
}

unsigned int FastPolygon::CheckTestPoints(std::vector< std::pair< bool, Point2f > >& testPoints,
                                          size_t edgeIdx,
                                          bool dryRun)
{

  unsigned int newHits = 0;

  for( auto& testEntry : testPoints ) {
    if( testEntry.first ) {
      // already used this point
      continue;
    }

    size_t pointIdx = _perpendicularEdgeVectors[edgeIdx].second;
    float dot = Anki::DotProduct( _perpendicularEdgeVectors[edgeIdx].first, testEntry.second - _poly[pointIdx] );

    if( dot > 0.0f ) {
      // is outside of edge, so this point is a hit! (we want to throw out as many test points as we can)
      newHits++;
      if(!dryRun) {
        testEntry.first = true;
      }
    }
  }

  return newHits;
}

}
