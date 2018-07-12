/**
 * File: memoryMapToPlanner.cpp
 *
 * Author: Raul
 * Date:   04/25/2017
 *
 * Description: Functions to convert memory map information into the representation needed for the motion planner.
 *
 * Copyright: Anki, Inc. 2017
 **/
#include "memoryMapToPlanner.h"

#include "engine/navMap/mapComponent.h"
#include "engine/cozmoContext.h" // only for debug render
#include "engine/viz/vizManager.h" // only for debug render
#include "engine/robot.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/helpers/boundedWhile.h"

#include <algorithm>

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

using NavMapGrid = std::vector<std::vector<Point2f>>;

void GroupPointsByMaxSize(const std::vector<MemoryMapTypes::BorderSegment>& segments,
                          const std::vector<Point2f>& points, 
                          NavMapGrid& bins) 
{
  if (points.empty() || segments.empty()) {
    return;
  }
  
  // get min/max x/y coordinates
  auto xRange = std::minmax_element(points.begin(), points.end(), 
                                    [](const Point2f& p1, const Point2f& p2) {return (p1.x() < p2.x());});
  auto yRange = std::minmax_element(points.begin(), points.end(), 
                                    [](const Point2f& p1, const Point2f& p2) {return (p1.y() < p2.y());});

  const float kMaxHullWidth_mm = 200;
  const float kEpsilon_mm      = .01;
  
  const float width      = fabs(xRange.second->x() - xRange.first->x()) + kEpsilon_mm / 10; 
  const float height     = fabs(yRange.second->y() - yRange.first->y()) + kEpsilon_mm / 10;
  
  if (width <= kEpsilon_mm || height <= kEpsilon_mm) {
    return;
  }
  
  const int   nCols      = ceil(width  / kMaxHullWidth_mm);
  const int   nRows      = ceil(height / kMaxHullWidth_mm);
  const float cellWidth  = width  / nCols;
  const float cellHeight = height / nRows;
    
  bins.resize(nCols * nRows);
  
  // populate vector by counting `n` times from `start` by `stride`
  auto lCountBy      = [](const float start, const float stride, const int n, std::vector<float>& out) {
                         for (float i = 0; i < n; i++) out.push_back(i*stride + start); 
                       }; 
                
  // generic line intecept formula 
  auto lIntercept    = [] (float a1, float a2, float b1, float b2, float as) -> float {
                         return (b1 + (as - a1) * (b2 - b1) / (a2 - a1)); 
                       };
                        
  // calculate x value for given y
  auto lGetXItercept = [lIntercept] (MemoryMapTypes::BorderSegment& seg, float y) -> float {
                         return lIntercept(seg.from.y(), seg.to.y(), seg.from.x(), seg.to.x(), y); 
                       };
  
  // calculate y value for given x
  auto lGetYItercept = [lIntercept] (MemoryMapTypes::BorderSegment& seg, float x) -> float {
                         return lIntercept(seg.from.x(), seg.to.x(), seg.from.y(), seg.to.y(), x); 
                       };
                        
  auto lInsertPoint  = [&bins, nCols, nRows, xRange, yRange, cellWidth, cellHeight] (float x, float y) {
                         const int i = floor((x - xRange.first->x()) / cellWidth);
                         const int j = floor((y - yRange.first->y()) / cellHeight);
                         if (i < nCols && j < nRows && i >= 0 && j >= 0) {
                           bins[i + j * nCols].emplace_back(x, y);
                         } else {
                           PRINT_NAMED_WARNING("NavMemoryMapToPlanner.GroupPointsByMaxSize.InsertPoint", 
                              "Tried to insert point in unallocated bin");
                         }
                       };                       
                       
  std::vector<float> xBorders;
  std::vector<float> yBorders;    
  lCountBy(xRange.first->x() + cellWidth,  cellWidth,  nCols - 1, xBorders);
  lCountBy(xRange.first->x() + cellHeight, cellHeight, nRows - 1, yBorders);
  
  // do not insert `from` points - we assume all regions are closed loops
  for (auto line : segments) {
    lInsertPoint(line.to.x(), line.to.y());
    
    // split at x intercepts
    if (!xBorders.empty()) {
      const float start = fmin(line.from.x(), line.to.x());
      const float end   = fmax(line.from.x(), line.to.x());
      
      int i1 = 0;     // x-bin idx where the line starts
      int i2 = 0;     // x-bin idx where the line ends
      for (int i = 0; i < xBorders.size(); ++i) {
        if (xBorders[i] < start) i1++;
        if (xBorders[i] < end)   i2++;
      }
      
      BOUNDED_WHILE (1000, i1 < xBorders.size() && i1 < i2) {
        const float y = lGetYItercept(line, xBorders[i1]);
        lInsertPoint(xBorders[i1] - kEpsilon_mm, y);
        lInsertPoint(xBorders[i1] + kEpsilon_mm, y);
        i1++;
      }
    }
    
    // split at y intercepts
    if (!yBorders.empty()) {
      const float start = fmin(line.from.y(), line.to.y());
      const float end   = fmax(line.from.y(), line.to.y());
      
      int i1 = 0;     // y-bin idx where the line starts
      int i2 = 0;     // y-bin idx where the line ends
      for (int i = 0; i < yBorders.size(); ++i) {
        if (yBorders[i] < start) i1++;
        if (yBorders[i] < end)   i2++;
      }
      
      BOUNDED_WHILE (1000, i1 < yBorders.size() && i1 < i2) {
        const float x = lGetXItercept(line, yBorders[i1]);
        lInsertPoint(x, yBorders[i1] - kEpsilon_mm);
        lInsertPoint(x, yBorders[i1] + kEpsilon_mm);
        i1++;
      }
    }
  }
}

}; // end namespace (annonymous for local access / helpers)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TranslateMapRegionToPolys(const INavMap::BorderRegionVector& regions, std::vector<ConvexPolygon>& convexHulls, const float minRegionArea_m2)
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
            
      // split points into bins to prevent one giant poly
      std::vector<std::vector<Point2f>> splits;
      GroupPointsByMaxSize(region.segments, points, splits);

      // compute convex hull
      for (auto subPoints : splits) {
        if (subPoints.size() > 2) {
          ConvexPolygon convexHullAsPoly = ConvexPolygon::ConvexHull( std::move(subPoints) );
          convexHulls.emplace_back( std::move(convexHullAsPoly) );
        } else if (!subPoints.empty()) {
          PRINT_NAMED_WARNING("NavMemoryMapToPlanner.TranslateMapRegionToPolys", 
                              "Not enough points to define 2d polygon");
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GetConvexHullsByType(INavMap* memoryMap,
                    const MemoryMapTypes::FullContentArray& outerTypes,
                    const MemoryMapTypes::EContentType innerType,
                    std::vector<ConvexPolygon>& convexHulls)
{
  if (nullptr == memoryMap) {
    PRINT_NAMED_WARNING("NavMemoryMapToPlanner.GetPolysByType", "null pointer to memory map");
    return;
  } 
  
  // calculate regions
  // rsam to Brad: this is what doesn't support N:M calculations, only 1:N
  INavMap::BorderRegionVector regions;
  memoryMap->CalculateBorders(innerType, outerTypes, regions);

  // Translate border regions into convex hull polygons
  if (!regions.empty()) {
    // define what a small region is in order to discard them as noise
    // kMinUsefulRegionUnits: number of units in the memory map (eg: quads in a quad tree) that boundaries have to have
    // in order for the region to be considered useful
    const uint32_t kMinUsefulRegionUnits = 4;
    const float memMapPrecision_mm = memoryMap->GetContentPrecisionMM();
    const float memMapPrecision_m  = MM_TO_M(memMapPrecision_mm);
    const float kMinRegionArea_m2 = kMinUsefulRegionUnits*(memMapPrecision_m*memMapPrecision_m);
    
    TranslateMapRegionToPolys(regions, convexHulls, kMinRegionArea_m2);
  }
}

void GetBorderPoly(INavMap* memoryMap,
                   const MemoryMapTypes::FullContentArray& outerTypes,
                   const MemoryMapTypes::EContentType innerType,
                   std::vector<Poly2f>& outPolys)
{
  if (nullptr == memoryMap) {
    PRINT_NAMED_WARNING("NavMemoryMapToPlanner.GetBorderPoly", "null pointer to memory map");
    return;
  } 
  
  // calculate regions
  // rsam to Brad: this is what doesn't support N:M calculations, only 1:N
  INavMap::BorderRegionVector regions;
  memoryMap->CalculateBorders(innerType, outerTypes, regions);
  
  // Translate border regions into convex hull polygons
  if (!regions.empty()) {
    // define what a small region is in order to discard them as noise
    // kMinUsefulRegionUnits: number of units in the memory map (eg: quads in a quad tree) that boundaries have to have
    // in order for the region to be considered useful
    const uint32_t kMinUsefulRegionUnits = 4;
    const float memMapPrecision_mm = memoryMap->GetContentPrecisionMM();
    const float memMapPrecision_m  = MM_TO_M(memMapPrecision_mm);
    const float kMinRegionArea_m2 = kMinUsefulRegionUnits*(memMapPrecision_m*memMapPrecision_m);
    
    for( const auto& region : regions )
    {
      // discard small areas entirely
      if ( FLT_LE(region.area_m2, kMinRegionArea_m2) ) {
        continue;
      }
    
      // the region should have segments
      if ( !region.segments.empty() )
      {  
        // add all 'to' points
        Poly2f poly;
        poly.emplace_back( std::move(region.segments[0].from) );
        for ( const auto& segment : region.segments )
        {
          poly.emplace_back( std::move(segment.to) );
        }
        outPolys.emplace_back( std::move(poly) );
      } else {
        PRINT_NAMED_WARNING("NavMemoryMapToPlanner.GetBorderPoly","cannot get bounding poly for empty region");
      }
      
      // check that the poly is closed
      if (!Util::IsFltNear(region.segments.front().from.x(), region.segments.back().to.x()) || 
          !Util::IsFltNear(region.segments.front().from.y(), region.segments.back().to.y())) 
      {
        PRINT_NAMED_WARNING("NavMemoryMapToPlanner.GetBorderPoly.CheckLoopClosed",
          "border region is not closed! (%.2f %.2f) (%.2f %.2f)",
          region.segments.front().from.x(), region.segments.front().to.y(),
          region.segments.back().from.x(), region.segments.back().to.y());
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TestNavMemoryMapToPlanner(Robot& robot)
{
  // Configuration of memory map to check for obstacles
  constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithInterestingEdges =
  {
    {MemoryMapTypes::EContentType::Unknown               , true},
    {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
    {MemoryMapTypes::EContentType::ClearOfCliff          , true},
    {MemoryMapTypes::EContentType::ObstacleObservable    , true},
    {MemoryMapTypes::EContentType::ObstacleProx          , true},
    {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
    {MemoryMapTypes::EContentType::Cliff                 , true},
    {MemoryMapTypes::EContentType::InterestingEdge       , false},
    {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
  };
  static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithInterestingEdges),
    "This array does not define all types once and only once.");
  
  constexpr MemoryMapTypes::FullContentArray typesToCalculateBordersWithNotInterestingEdges =
  {
    {MemoryMapTypes::EContentType::Unknown               , true},
    {MemoryMapTypes::EContentType::ClearOfObstacle       , true},
    {MemoryMapTypes::EContentType::ClearOfCliff          , true},
    {MemoryMapTypes::EContentType::ObstacleObservable    , true},
    {MemoryMapTypes::EContentType::ObstacleProx          , true},
    {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
    {MemoryMapTypes::EContentType::Cliff                 , true},
    {MemoryMapTypes::EContentType::InterestingEdge       , true},
    {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
  };
  static_assert(MemoryMapTypes::IsSequentialArray(typesToCalculateBordersWithNotInterestingEdges),
    "This array does not define all types once and only once.");
      
  std::vector<ConvexPolygon> cHullsInteresting;
  std::vector<ConvexPolygon> cHullsNotInteresting;
  INavMap* memoryMap = robot.GetMapComponent().GetCurrentMemoryMap();
  GetConvexHullsByType(memoryMap, typesToCalculateBordersWithInterestingEdges, MemoryMapTypes::EContentType::InterestingEdge, cHullsInteresting);
  GetConvexHullsByType(memoryMap, typesToCalculateBordersWithNotInterestingEdges, MemoryMapTypes::EContentType::NotInterestingEdge, cHullsNotInteresting);

  
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
