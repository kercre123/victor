/**
 * File: memoryMapData_ProxObstacle.h
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_DATA_PROX_OBSTACLE_H
#define ANKI_COZMO_MEMORY_MAP_DATA_PROX_OBSTACLE_H

#include "memoryMapData.h"

#include "anki/common/basestation/math/point.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// MemoryMapData_ProxObstacle
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct MemoryMapData_ProxObstacle : public MemoryMapData
{
  // constructor
  MemoryMapData_ProxObstacle(Vec2f dir);
  
  // create a copy of self (of appropriate subclass) and return it
  MemoryMapData* Clone() const;
  
  // compare to IMemoryMapData and return bool if the data stored is the same
  bool Equals(const MemoryMapData* other) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If you add attributes, make sure you add them to ::Equals and ::Clone (if required)
  Vec2f directionality; // direction we presume for the collision (based off robot pose when detected)
};
 
} // namespace
} // namespace

#endif //
