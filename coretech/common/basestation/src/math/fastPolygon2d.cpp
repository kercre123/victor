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

#include "anki/common/basestation/math/polygon_impl.h"
#include "anki/common/basestation/math/fastPolygon2d.h"

#include <cmath>

namespace Anki {

int FastPolygon::_numChecks = 0;
int FastPolygon::_numDotProducts = 0;


FastPolygon::FastPolygon(const Poly2f& basePolygon)
  : _poly(basePolygon)
{
  CreateEdgeVectors();

  ComputeCenter();

  ComputeCircles();

  SortEdgeVectors();
}

bool FastPolygon::Contains(const Point2f& testPoint) const
{
  // the goal here is to throw out points as quickly as
  // possible. First compute the squared distance to the center, and
  // do circle checks

  _numChecks++;

  float distSqaured =
    std::pow( testPoint.y() - _circleCenter.y(), 2) +
    std::pow( testPoint.x() - _circleCenter.x(), 2);

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

  unsigned int numChecked = 0;

  for(size_t i = 0; i < numPts; ++i) {
    // if the dot product is positive, the test point is inside of the
    // edge, so we must continue. Otherwise we can bail out early
    float dot = Anki::DotProduct( _perpendicularEdgeVectors[i], testPoint - _poly[i] );

    _numDotProducts++;

    if( dot > 0.0f ) {
      // was inside circle, but outside of an edge
      return false;
    }
  }

  // was inside of outer circle, and inside of all edges, so is inside
  return true;
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

    float distanceToEdgeSquared = std::pow(
      Anki::DotProduct( _perpendicularEdgeVectors[i], _circleCenter - _poly[i] ),
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
    _perpendicularEdgeVectors.emplace_back( Vec2f{ -edgeVector.y() * oneOverNorm, edgeVector.x() * oneOverNorm } );
  }
}

void FastPolygon::SortEdgeVectors()
{
  // TODO:(bn) implement
}


}
