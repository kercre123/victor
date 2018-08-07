/**
* File: visionModesHelpers
*
* Author: Lee Crippen
* Created: 06/12/16
*
* Description: Helper functions for dealing with CLAD generated visionModes
*
* Copyright: Anki, Inc. 2016
*
**/


#ifndef __Cozmo_Basestation_VisionModesHelpers_H__
#define __Cozmo_Basestation_VisionModesHelpers_H__


#include "clad/types/visionModes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Vector {

DECLARE_ENUM_INCREMENT_OPERATORS(VisionMode);

} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_VisionModesHelpers_H__

