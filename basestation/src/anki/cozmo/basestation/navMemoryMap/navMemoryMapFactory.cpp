/**
 * File: navMemoryMapFactory.h
 *
 * Author: Raul
 * Date:   03/11/2016
 *
 * Description: Factory to hide the specific type of memory map used by Cozmo.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_FACTORY_H
#define ANKI_COZMO_NAV_MEMORY_MAP_FACTORY_H

#include "navMemoryMapQuadTree/navMemoryMapQuadTree.h"

namespace Anki {
namespace Cozmo {
namespace NavMemoryMapFactory {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INavMemoryMap* CreateDefaultNavMemoryMap(VizManager* vizManager, Robot* robot)
{
  return new NavMemoryMapQuadTree(vizManager, robot);
}

} // namespace
} // namespace
} // namespace

#endif // ANKI_COZMO_NAV_MEMORY_MAP_H
