/**
 * File: navMemoryMapToPlanner.h
 *
 * Author: Raul
 * Date:   04/25/2017
 *
 * Description: Functions to convert memory map information into the representation needed for the motion planner.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef _Anki_Cozmo_NavMemoryMapToPlanner_h__
#define _Anki_Cozmo_NavMemoryMapToPlanner_h__

#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/common/basestation/math/polygon.h"

namespace Anki {
namespace Cozmo {

// fwd declaration
class Robot;

// Converts from a vector of regions from the INavMemoryMap into a vector of polygons that are convex hulls of
// those regions.
// minRegionArea_m2: regions with smaller area than this parameter will be discarded, an no convex hull will
// be generated for them.
void TranslateMapRegionToPolys(const INavMemoryMap::BorderRegionVector& regions, std::vector<Poly2f>& convexHulls, const float minRegionArea_m2);

// rsam: this is for Brad, since I implemented this code, but Brad will incorporate to the planner. This function, when
// called, calculates borders in the memory map, computes convex hulls for them, and renders them (without storing them.)
// TODO: Brad delete me at some point
void TestNavMemoryMapToPlanner(Robot& robot);

} // namespace Cozmo
} // namespace Anki

#endif // 
