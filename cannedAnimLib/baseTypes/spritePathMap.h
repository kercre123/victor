/**
 * File: SpritePathMap.h
 *
 * Author: Kevin M. Karol
 * Date:   4/9/2018
 *
 * Description: Defines type of spritePathMap
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __CannedAnimLib_BaseTypes_SpritePathMap_H__
#define __CannedAnimLib_BaseTypes_SpritePathMap_H__

#include "clad/types/spriteNames.h"
#include "util/cladHelpers/cladEnumToStringMap.h"

namespace Anki {
namespace CannedAnimLib {

using SpritePathMap = Util::CladEnumToStringMap<Vision::SpriteName>;
  
} // namespace Cozmo
} // namespace Anki


#endif // __CannedAnimLib_BaseTypes_SpritePathMap_H__
