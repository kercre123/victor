/**
 * File: navMemoryMapQuadData_Cliff.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Data for Cliff quads.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_CLIFF_H
#define ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_CLIFF_H

#include "iNavMemoryMapQuadData.h"

#include "anki/common/basestation/math/point.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadData_Cliff
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct NavMemoryMapQuadData_Cliff : public INavMemoryMapQuadData
{
  // constructor
  NavMemoryMapQuadData_Cliff();
  
  // create a copy of self (of appropriate subclass) and return it
  virtual INavMemoryMapQuadData* Clone() const override;
  
  // compare to INavMemoryMapQuadData and return bool if the data stored is the same
  virtual bool Equals(const INavMemoryMapQuadData* other) const override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If you add attributes, make sure you add them to ::Equals and ::Clone (if required)
  Vec2f directionality; // direction we presume for the cliff (from detection)
};
 
} // namespace
} // namespace

#endif //
