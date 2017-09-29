/**
 * File: memoryMapData_Cliff.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Data for Cliff quads.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_DATA_CLIFF_H
#define ANKI_COZMO_MEMORY_MAP_DATA_CLIFF_H

#include "memoryMapData.h"

#include "anki/common/basestation/math/point.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadData_Cliff
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct MemoryMapData_Cliff : public MemoryMapData
{
  // constructor
  MemoryMapData_Cliff(Vec2f dir, TimeStamp_t t);
  
  // create a copy of self (of appropriate subclass) and return it
  MemoryMapData* Clone() const override;
  
  // compare to INavMemoryMapQuadData and return bool if the data stored is the same
  bool Equals(const MemoryMapData* other) const override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If you add attributes, make sure you add them to ::Equals and ::Clone (if required)
  Vec2f directionality; // direction we presume for the cliff (from detection)
};
 
} // namespace
} // namespace

#endif //
