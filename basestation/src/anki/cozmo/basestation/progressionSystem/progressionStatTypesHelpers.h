/**
 * File: progressionStatTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Helper functions for dealing with CLAD generated progressionStatTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_ProgressionSystem_ProgressionStatTypesHelpers_H__
#define __Cozmo_Basestation_ProgressionSystem_ProgressionStatTypesHelpers_H__


#include "clad/types/progressionStatTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(ProgressionStatType);

ProgressionStatType ProgressionStatTypeFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_ProgressionSystem_ProgressionStatTypesHelpers_H__

