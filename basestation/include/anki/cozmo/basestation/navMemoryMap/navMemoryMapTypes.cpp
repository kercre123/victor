/**
 * File: navMemoryMapTypes.cpp
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the navMemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMemoryMapTypes.h"

namespace Anki {
namespace Cozmo {
namespace NavMemoryMapTypes {

bool ExpectsAdditionalData(EContentType type)
{
  const bool ret = (type == EContentType::Cliff);
  return ret;
}

} // namespace
} // namespace
} // namespace
