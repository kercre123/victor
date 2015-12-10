/**
 * File: behaviorChooserTypesHelpers.cpp
 *
 * Author: Brad Neuman
 * Created: 2015-12-08
 *
 * Description: Helper functions for dealing with CLAD generated behavior chooser types
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorchooserTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(BehaviorChooserType);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<BehaviorChooserType> gStringToBehaviorChooserTypeMapper;

BehaviorChooserType BehaviorChooserTypeFromString(const char* inString)
{
  return gStringToBehaviorChooserTypeMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

