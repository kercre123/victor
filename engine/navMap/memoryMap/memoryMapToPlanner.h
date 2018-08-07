/**
 * File: memoryMapToPlanner.h
 *
 * Author: Raul
 * Date:   04/25/2017
 *
 * Description: Functions to convert memory map information into the representation needed for the motion planner.
 *
 * Copyright: Anki, Inc. 2017
 **/
 
 #ifndef _ANKI_COZMO_MEMORY_MAP_TO_PLANNER_H__
 #define _ANKI_COZMO_MEMORY_MAP_TO_PLANNER_H__

#include "engine/navMap/iNavMap.h"
#include "coretech/common/engine/math/convexPolygon2d.h"
#include "coretech/common/engine/math/polygon.h"

namespace Anki {
namespace Vector {

// fwd declaration
class Robot;

// Converts from a vector of regions from the INavMemoryMap into a vector of polygons that are convex hulls of
// those regions.
// minRegionArea_m2: regions with smaller area than this parameter will be discarded, an no convex hull will
// be generated for them.
void TranslateMapRegionToPolys(const INavMap::BorderRegionVector& regions, std::vector<ConvexPolygon>& convexHulls, const float minRegionArea_m2);

// rsam: this is for Brad, since I implemented this code, but Brad will incorporate to the planner. This function, when
// called, calculates borders in the memory map, computes convex hulls for them, and renders them (without storing them.)
// TODO: Brad delete me at some point
void TestNavMemoryMapToPlanner(Robot& robot);

// Gets all convex polys by edge node type in the current memory map
void GetConvexHullsByType(INavMap* memoryMap,
                    const MemoryMapTypes::FullContentArray& outerTypes,
                    const MemoryMapTypes::EContentType innerType,
                    std::vector<ConvexPolygon>& convexHulls);
                                        
// get strict poly without CH calculation
void GetBorderPoly(INavMap* memoryMap,
                   const MemoryMapTypes::FullContentArray& outerTypes,
                   const MemoryMapTypes::EContentType innerType,
                   std::vector<Poly2f>& outPoly);

} // namespace Vector
} // namespace Anki

#endif // 
