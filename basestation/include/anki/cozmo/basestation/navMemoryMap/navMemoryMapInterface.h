/**
 * File: navMemoryMapInterface.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Public interface for a map of the space navigated by the robot with some memory 
 * features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_INTERFACE_H
#define ANKI_COZMO_NAV_MEMORY_MAP_INTERFACE_H


#include "anki/common/basestation/math/quad.h"
#include "navMemoryMapTypes.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Class INavMemoryMap
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class INavMemoryMap
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual ~INavMemoryMap() {}

  // add a quad that is clear of obstacles
  virtual void AddClearQuad(const Quad2f& quad) = 0;

  // add a quad representing an obstacle
  virtual void AddObstacleQuad(const Quad2f& quad, NavMemoryMapTypes::EObstacleType obstacleType) = 0;

  // add a quad representing a cliff
  virtual void AddCliffQuad(const Quad2f& quad) = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Render memory map
  virtual void Draw() const = 0;
  
}; // class
  
} // namespace
} // namespace

#endif // 
