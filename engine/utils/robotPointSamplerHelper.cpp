/**
 * File: robotPointSamplerHelper.cpp
 *
 * Author: ross
 * Created: 2018 Jun 29
 *
 * Description: Helper class for rejection sampling 2D positions that abide by some constraints related to the robot
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/utils/robotPointSamplerHelper.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "engine/navMap/iNavMap.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Cliff.h"
#include "util/random/randomGenerator.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const float kMaxCliffIntersectionDist_mm = 10000.0f;
}

namespace RobotPointSamplerHelper {
  
using RandomGenerator = Util::RandomGenerator;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point2f SamplePointInCircle( Util::RandomGenerator& rng, f32 radius )
{
  // (there's another way to do this without the sqrt, but it requires three
  // uniform r.v.'s, and some quick tests show that that ends up being slower)
  Point2f ret;
  const float theta = M_TWO_PI_F * static_cast<float>( rng.RandDbl() );
  const float u = static_cast<float>( rng.RandDbl() );
  const float r = radius * std::sqrtf( u );
  ret.x() = r * cosf(theta);
  ret.y() = r * sinf(theta);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Point2f SamplePointInAnnulus( Util::RandomGenerator& rng, f32 minRadius, f32 maxRadius )
{
  Point2f ret;
  const f32 minRadiusSq = minRadius * minRadius;
  const float theta = M_TWO_PI_F * static_cast<float>( rng.RandDbl() );
  const float u = static_cast<float>( rng.RandDbl() );
  const float r = std::sqrtf( minRadiusSq + (maxRadius*maxRadius - minRadiusSq)*u );
  ret.x() = r * cosf(theta);
  ret.y() = r * sinf(theta);
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RejectIfWouldCrossCliff::RejectIfWouldCrossCliff( float minCliffDist_mm )
  : _minCliffDistSq( minCliffDist_mm * minCliffDist_mm )
{}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfWouldCrossCliff::SetAcceptanceInterpolant( float maxCliffDist_mm, RandomGenerator& rng )
{
  _rng = &rng;
  _maxCliffDistSq = maxCliffDist_mm*maxCliffDist_mm;
  DEV_ASSERT( _maxCliffDistSq > _minCliffDistSq, "RejectIfWouldCrossCliff.SetAcceptanceInterpolant.DistanceError" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfWouldCrossCliff::SetRobotPosition(const Point2f& pos)
{
  _robotPos = pos;
  _setRobotPos = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfWouldCrossCliff::UpdateCliffs( const INavMap* memoryMap )
{
  _cliffs.clear();
  if( memoryMap == nullptr ) {
    return;
  }
  MemoryMapTypes::MemoryMapDataConstList wasteList;
  memoryMap->FindContentIf([this](MemoryMapTypes::MemoryMapDataConstPtr data){
    if( data->type == MemoryMapTypes::EContentType::Cliff ) {
      const auto& cliffData = MemoryMapData::MemoryMapDataCast<MemoryMapData_Cliff>(data);
      _cliffs.insert( &cliffData->pose );
    }
    return false; // don't actually gather any data
  }, wasteList);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RejectIfWouldCrossCliff::operator()( const Point2f& sampledPos )
{
  DEV_ASSERT( _setRobotPos, "RejectIfWouldCrossCliff.CallOperator.UninitializedRobotPos" );
  LineSegment lineRobotToSample{ sampledPos, _robotPos };
  float pAccept = 1.0f; // this may be decremented for multiple cliffs
  for( const auto* cliffPose : _cliffs ) {
    const Vec3f cliffDirection = (cliffPose->GetRotation() * X_AXIS_3D());
    // do this in 2d
    const Vec2f cliffEdgeDirection = CrossProduct( Z_AXIS_3D(), cliffDirection ); // sign doesn't matter
    const Point2f cliffPos = cliffPose->GetTranslation();
    // find intersection of lineRobotToSample with cliffEdgeDirection
    LineSegment cliffLine{ cliffPos + cliffEdgeDirection * kMaxCliffIntersectionDist_mm,
                           cliffPos - cliffEdgeDirection * kMaxCliffIntersectionDist_mm };
    Point2f intersectionPoint;
    const bool intersects = lineRobotToSample.IntersectsAt( cliffLine, intersectionPoint );
    if( intersects ) {
      // confirm intersection point lies on cliff edge
      DEV_ASSERT( AreVectorsAligned( (intersectionPoint - cliffPos), cliffEdgeDirection, 0.001f ),
                  "RejectIfWouldCrossCliff.CallOperator.BadIntersection" );
      // if the intersection pos is close to the cliff pos, reject. If it's far, accept. interpolate in between.
      const float distFromCliffSq = (intersectionPoint - cliffPos).LengthSq();
      if( distFromCliffSq < _minCliffDistSq ) {
        return false;
      }
      if( _rng != nullptr ) {
        const float p = (distFromCliffSq > _maxCliffDistSq)
                        ? 0.0f
                        : 1.0f - (distFromCliffSq - _minCliffDistSq) / (_maxCliffDistSq - _minCliffDistSq);
        // multiple cliffs can contribute to the acceptance probability
        pAccept -= p;
        if( pAccept <= 0.0f ) {
          return false;
        }
      }
    }
  }
  if( (_rng != nullptr) && ((pAccept <= 0.0f) || (pAccept < _rng->RandDbl())) ) {
    // reject
    return false;
  } else {
    return true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RejectIfInRange::RejectIfInRange( float minDist_mm, float maxDist_mm )
: _minDistSq( minDist_mm * minDist_mm )
, _maxDistSq( maxDist_mm * maxDist_mm )
{
  DEV_ASSERT(!std::signbit(minDist_mm) &&
             !std::signbit(maxDist_mm) &&
             (maxDist_mm > minDist_mm),
             "RejectIfInRange.Constructor.InvalidArgs");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfInRange::SetOtherPosition(const Point2f& pos)
{
  SetOtherPositions({{pos}});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfInRange::SetOtherPositions(const std::vector<Point2f>& pos)
{
  _otherPos.clear();
  _otherPos = pos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RejectIfInRange::operator()( const Point2f& sampledPos )
{
  DEV_ASSERT(!_otherPos.empty(), "RejectIfInRange.CallOperator.OtherPosUninitialized" );
  auto inRangePred = [this, &sampledPos](const Point2f& otherPos) {
    const float distSq = (otherPos - sampledPos).LengthSq();
    return (distSq >= _minDistSq) && (distSq <= _maxDistSq);
  };
  const bool reject = std::any_of(_otherPos.begin(),
                                  _otherPos.end(),
                                  inRangePred);
  return !reject;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RejectIfNotInRange::RejectIfNotInRange( float minDist_mm, float maxDist_mm )
  : _minDistSq( minDist_mm * minDist_mm )
  , _maxDistSq( maxDist_mm * maxDist_mm )
{ }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfNotInRange::SetOtherPosition(const Point2f& pos)
{
  _otherPos = pos;
  _setOtherPos = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RejectIfNotInRange::operator()( const Point2f& sampledPos )
{
  DEV_ASSERT( _setOtherPos, "RejectIfNotInRange.CallOperator.OtherPosUninitialized" );
  const float distSq = (_otherPos - sampledPos).LengthSq();
  return (distSq >= _minDistSq) && (distSq <= _maxDistSq);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RejectIfCollidesWithMemoryMap::RejectIfCollidesWithMemoryMap( const MemoryMapTypes::FullContentArray& collisionTypes )
  : _collisionTypes( collisionTypes )
{}
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RejectIfCollidesWithMemoryMap::SetAcceptanceProbability( float p, RandomGenerator& rng )
{
  _rng = &rng;
  _pAccept = p;
  DEV_ASSERT( _pAccept >= 0.0f && _pAccept <= 1.0f, "RejectIfCollidesWithMemoryMap.SetAcceptanceProbability.InvalidP" );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RejectIfCollidesWithMemoryMap::operator()( const Poly2f& sampledPoly )
{
  if( _memoryMap == nullptr ) {
    return true;
  }
  const bool collides = _memoryMap->HasCollisionWithTypes( sampledPoly, _collisionTypes );
  if( collides && (_rng != nullptr) ) {
    const float rv = _rng->RandDbl();
    if( rv <= _pAccept ) {
      return true;
    } else {
      return false;
    }
  }
  return !collides;
}
  
} // namespace
  
} // namespace
} // namespace
