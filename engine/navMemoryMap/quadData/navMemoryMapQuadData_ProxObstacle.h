/**
 * File: navMemoryMapQuadData_ProxObstacle.h
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_PROX_OBSTACLE_H
#define ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_PROX_OBSTACLE_H

#include "iNavMemoryMapQuadData.h"

#include "anki/common/basestation/math/point.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadData_ProxObstacle
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct NavMemoryMapQuadData_ProxObstacle : public INavMemoryMapQuadData
{
  // constructor
  NavMemoryMapQuadData_ProxObstacle();
  
  // create a copy of self (of appropriate subclass) and return it
  virtual INavMemoryMapQuadData* Clone() const override;
  
  // compare to INavMemoryMapQuadData and return bool if the data stored is the same
  virtual bool Equals(const INavMemoryMapQuadData* other) const override;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If you add attributes, make sure you add them to ::Equals and ::Clone (if required)
  Vec2f directionality; // direction we presume for the collision (based off robot pose when detected)
};
 
} // namespace
} // namespace

#endif //
