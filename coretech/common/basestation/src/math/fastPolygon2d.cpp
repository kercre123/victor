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

FastPolygon::FastPolygon(const Poly2f& basePolygon)
  : _poly(basePolygon)
{
  CreateEdgeVectors();

  ComputeCenter();

  ComputeCircles();

  SortEdgeVectors();
}

bool FastPolygon::Contains(float x, float y) const
{
  // TEMP: 
  return false;
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
