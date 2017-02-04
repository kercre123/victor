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

#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"

namespace Anki {
namespace Cozmo {
namespace NavMemoryMapFactory {

// creates the proper nav memory map (through default, config, etc)
INavMemoryMap* CreateDefaultNavMemoryMap(VizManager* vizManager, Robot* robot);
  
} // namespace
} // namespace
} // namespace

#endif // ANKI_COZMO_NAV_MEMORY_MAP_FACTORY_H
