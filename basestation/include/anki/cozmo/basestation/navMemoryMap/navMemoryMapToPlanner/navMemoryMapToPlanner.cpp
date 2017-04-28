/**
 * File: navMemoryMapToPlanner.cpp
 *
 * Author: Raul
 * Date:   04/25/2017
 *
 * Description: Functions to convert memory map information into the representation needed for the motion planner.
 *
 * Copyright: Anki, Inc. 2017
 **/
#include "navMemoryMapToPlanner.h"

#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h" // only for debug render
#include "anki/cozmo/basestation/viz/vizManager.h" // only for debug render
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// Orientation of a set of 3 vertices. What is the turn from from p1 to p2 with respect to p0 to p1 vector
enum Orientation { RIGHT_TURN, LEFT_TURN, COLLINEAR };
Orientation ComputeOrientation(const Point2f& p0, const Point2f& p1, const Point2f& p2)
{
  // calculate cross product Z = a1*b2 - a2*b1
  const Vec2f& originTo1 = p1 - p0;
  const Vec2f& originTo2 = p2 - p0;
  float crossProductZ = originTo1.x() * originTo2.y() - originTo2.x() * originTo1.y();
  
  // compute orientation based on sign of crossProduct
  const Orientation o = NEAR_ZERO(crossProductZ) ?
                          Orientation::COLLINEAR :
                          ( FLT_GE_ZERO(crossProductZ) ?
                              Orientation::LEFT_TURN :
                              Orientation::RIGHT_TURN );
  return o;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ComputeConvexHull_GrahamScan: Given a set of points, computes the convex hull using Graham scan algorithm
// https://en.wikipedia.org/wiki/Graham_scan
// Note the vector passed is not const, and in fact will be changed by the algorithm (will be sorted). Doing this now
// because the API is internal to this file, but if we need a const parameter, we need to copy internally. In that
// case, consider a list if necessary
void ComputeConvexHull_GrahamScan(std::vector<Point2f>& points, Poly2f& outConvexPoly)
{
  DEV_ASSERT(outConvexPoly.size() == 0, "ComputeConvexHull_GrahamScan.PolyNotEmpty"); // there's no clear in Poly
  outConvexPoly.reserve(points.size());
  
  // check if there are not enough points for Graham scan
  const size_t numPoints = points.size();
  if ( numPoints <= 2 )
  {
    // should not call without points
    DEV_ASSERT( numPoints != 0, "ComputeConvexHull_GrahamScan.EmptyPointVector");
    if ( numPoints > 0 )
    {
      // add one point
      outConvexPoly.push_back(points[0]);
      
      // and the next one if available and not overlapping with the first
      if ( numPoints > 1 ) {
        const bool overlapping = NEAR_ZERO((points[0] - points[1]).LengthSq());
        if ( !overlapping ) {
          outConvexPoly.push_back(points[1]);
        }
      }
    }
    else
    {
      // not expected to receive 0 points
      PRINT_NAMED_WARNING("ComputeConvexHull_GrahamScan.EmptyPointVector", "No points provided");
    }
  }
  else
  {
    // generate polygon for this region
    // - - - - - - - - - - - - - - - - - - - - -
    // Graham scan algorithm on points
    
    // Find the bottommost point
    float minY = points[0].y();
    size_t minIndex = 0;
    for (int i = 1; i < numPoints; ++i)
    {
      const float curY = points[i].y();

      // Smallest Y. Use smallest X as tiebreaker
      if( FLT_LT(curY, minY) || (FLT_NEAR(curY, minY) && FLT_LT(points[i].x(), points[minIndex].x())))
      {
        minY = points[i].y();
        minIndex = i;
      }
    }
    
    // swap 0 and minIndex
    std::iter_swap(points.begin(), points.begin()+minIndex);
    
    // comparator to sort points clockwise with respect to p0
    const Point2f& p0 = points[0]; // grabbing a reference since it should not change while sorting
    const auto compareCounterClockwise = [&p0](const Point2f& p1, const Point2f& p2) -> bool
    {
      const Orientation o = ComputeOrientation(p0,p1,p2);
      if ( o == Orientation::COLLINEAR ) {
        // if collinear, pick closest
        const Vec2f& originTo1 = p1 - p0;
        const Vec2f& originTo2 = p2 - p0;
        const bool p1Closer = FLT_LT(originTo1.LengthSq(),originTo2.LengthSq());
        return p1Closer ? true : false;
      }
      
      return (o == Orientation::LEFT_TURN) ? true : false;
    };
    
    // sort clockwise
    std::sort(points.begin()+1, points.end(), compareCounterClockwise);
    
    // add first 2 points
    outConvexPoly.push_back( points[0] );
    outConvexPoly.push_back( points[1] );
    
    // find the first point that is not collinear to 0-1
    size_t curIndex = 2;
    for(; curIndex<numPoints; ++curIndex)
    {
      const Orientation orientation = ComputeOrientation(outConvexPoly[0], outConvexPoly[1], points[curIndex]);
      if ( orientation == Orientation::COLLINEAR ) {
        outConvexPoly.pop_back();
        outConvexPoly.push_back(points[curIndex]);
      } else {
        outConvexPoly.push_back( points[curIndex] );
        break;
      }
    }
    
    // iterate all other candidate points
    for(size_t i=curIndex; i<numPoints; ++i)
    {
      // calculate orientation we would obtain adding the point to the current convex hull
      Orientation newOrientation = Orientation::COLLINEAR; // default doesn't matter
      do {
        size_t curPolyTop = outConvexPoly.size()-1; // -1 to point at last
        DEV_ASSERT(curPolyTop>=1, "AlgorithmFailure.CantPopLast2Elements");
        newOrientation = ComputeOrientation(outConvexPoly[curPolyTop-1], outConvexPoly[curPolyTop], points[i]);
      
        // if adding it would cause a right turn or to be collinear, we would pop the top of the current hull, since
        // adding point[i] would make it concave otherwise
        if ( newOrientation != Orientation::LEFT_TURN ) {
          outConvexPoly.pop_back();
        }
      } while( newOrientation != Orientation::LEFT_TURN );
      
      // now this point becomes part of the convex hull
      outConvexPoly.push_back(points[i]);
    }
  }
}

}; // end namespace (annonymous for local access / helpers)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TranslateMapRegionToPolys(const INavMemoryMap::BorderRegionVector& regions, std::vector<Poly2f>& convexHulls, const float minRegionArea_m2)
{
  ANKI_CPU_PROFILE("TranslateMapRegionToPolys");

  // BorderRegion is a vector of Segments with from and to, which we assume are connected (ie: one segments's 'to', is
  // another segment's from)
  for( const auto& region : regions )
  {
    // discard small areas entirely
    if ( FLT_LE(region.area_m2, minRegionArea_m2) ) {
      continue;
    }
  
    // the region should have segments
    if ( !region.segments.empty() )
    {
      std::vector<Point2f> points;
      points.reserve(region.segments.size()+1);
      
      // add first 'from'
      points.emplace_back(region.segments[0].from.x(), region.segments[0].from.y());

      // add all 'to' points
      for ( const auto& segment : region.segments )
      {
        // raul: I believe the 'to' from the last segment can be equal to the 'from' from the first one. This is
        // not a problem for the algorithm, but it could be discarded here if necessary
        points.emplace_back(segment.to.x(), segment.to.y());
      }

      // compute convex hull
      Poly2f convexHullAsPoly;
      ComputeConvexHull_GrahamScan(points, convexHullAsPoly);
      
      // add convex hull
      convexHulls.emplace_back( std::move(convexHullAsPoly) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestNavMemoryMapToPlanner(Robot& robot)
{
  // define what a small region is in order to discard them as noise
  // kMinUsefulRegionUnits: number of units in the memory map (eg: quads in a quad tree) that boundaries have to have
  // in order for the region to be considered useful
  const uint32_t kMinUsefulRegionUnits = 4;
  const float memMapPrecision_mm = robot.GetBlockWorld().GetNavMemoryMap()->GetContentPrecisionMM();
  const float memMapPrecision_m  = MM_TO_M(memMapPrecision_mm);
  const float kMinRegionArea_m2 = kMinUsefulRegionUnits*(memMapPrecision_m*memMapPrecision_m);

  // Configuration of memory map to check for obstacles
  constexpr NavMemoryMapTypes::FullContentArray typesToCalculateBordersWithInterestingEdges =
  {
    {NavMemoryMapTypes::EContentType::Unknown               , true},
    {NavMemoryMapTypes::EContentType::ClearOfObstacle       , true},
    {NavMemoryMapTypes::EContentType::ClearOfCliff          , true},
    {NavMemoryMapTypes::EContentType::ObstacleCube          , true},
    {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , true},
    {NavMemoryMapTypes::EContentType::ObstacleCharger       , true},
    {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, true},
    {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
    {NavMemoryMapTypes::EContentType::Cliff                 , true},
    {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
    {NavMemoryMapTypes::EContentType::NotInterestingEdge    , true}
  };
  static_assert(NavMemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithInterestingEdges),
    "This array does not define all types once and only once.");
  
  constexpr NavMemoryMapTypes::FullContentArray typesToCalculateBordersWithNotInterestingEdges =
  {
    {NavMemoryMapTypes::EContentType::Unknown               , true},
    {NavMemoryMapTypes::EContentType::ClearOfObstacle       , true},
    {NavMemoryMapTypes::EContentType::ClearOfCliff          , true},
    {NavMemoryMapTypes::EContentType::ObstacleCube          , true},
    {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , true},
    {NavMemoryMapTypes::EContentType::ObstacleCharger       , true},
    {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, true},
    {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
    {NavMemoryMapTypes::EContentType::Cliff                 , true},
    {NavMemoryMapTypes::EContentType::InterestingEdge       , true},
    {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
  };
  static_assert(NavMemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithNotInterestingEdges),
    "This array does not define all types once and only once.");

  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();

  // calculate regions
  // rsam to Brad: this is what doesn't support N:M calculations, only 1:N
  INavMemoryMap::BorderRegionVector interestingRegions;
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::InterestingEdge, typesToCalculateBordersWithInterestingEdges, interestingRegions);
  INavMemoryMap::BorderRegionVector notInterestingRegions;
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::NotInterestingEdge, typesToCalculateBordersWithNotInterestingEdges, notInterestingRegions);

  // Translate border regions into convex hull polygons
  std::vector<Poly2f> cHullsInteresting;
  TranslateMapRegionToPolys(interestingRegions, cHullsInteresting, kMinRegionArea_m2);
  std::vector<Poly2f> cHullsNotInteresting;
  TranslateMapRegionToPolys(notInterestingRegions, cHullsNotInteresting, kMinRegionArea_m2);
  
  // Draw all polys
  {
    VizManager* vizMgr = robot.GetContext()->GetVizManager();
    static u32 prevPolyIDLimit = 0;   // this is to clear a previous call
    u32 polyID = 666;                 // initial number because of how IDs work in Viz (should port to string identifiers)
    for( const auto& p : cHullsInteresting ) {
      vizMgr->DrawPoly(polyID, p, NamedColors::CYAN);
      ++polyID;
    }
    for( const auto& p : cHullsNotInteresting ) {
      vizMgr->DrawPoly(polyID, p, NamedColors::RED);
      ++polyID;
    }
    
    // erase all IDs we previously rendered and have not been overridden in this tick
    for(u32 oldID=polyID;oldID<prevPolyIDLimit; ++oldID) {
      vizMgr->ErasePoly(oldID);
    }
    prevPolyIDLimit = polyID;
  }
}

} // namespace
} // namespace
