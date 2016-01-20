/**
 * File: navMemoryMapTypes.h
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the navMemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_TYPES_H
#define ANKI_COZMO_NAV_MEMORY_MAP_TYPES_H

namespace Anki {
namespace Cozmo {
namespace NavMemoryMapTypes {

// this enum allows us to recognize the expected shape of the obstacle. At the moment I'm going to try this,
// but we may want in the future to actually set a pointer to the obstacle so that we can query more information
// or we can get notified when things move in front of Cozmo
enum class EObstacleType {
  Cube, // marked cube we recognize
  Unrecognized // object whose shape we don't recognize
};

} // namespace
} // namespace
} // namespace

#endif // 
