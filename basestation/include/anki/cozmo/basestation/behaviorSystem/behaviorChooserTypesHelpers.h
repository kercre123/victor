/**
 * File: behaviorChooserTypesHelpers.h
 *
 * Author: Brad Neuman
 * Created: 2015-12-08
 *
 * Description: Helper functions for dealing with CLAD generated behavior chooser types
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChooserTypesHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChooserTypesHelpers_H__


#include "clad/types/behaviorChooserType.h"
#include "util/enums/enumOperators.h"
#include <string>


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorChooserType);

BehaviorChooserType BehaviorChooserTypeFromString(const char* inString);

inline BehaviorChooserType BehaviorChooserTypeFromString(const std::string& inString)
{
  return BehaviorChooserTypeFromString(inString.c_str());
}


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorChooserTypesHelpers_H__

