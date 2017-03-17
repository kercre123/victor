/**
 * File: activeObjectTypeHelpers.h
 *
 * Author: Andrew Stein
 * Date:   3/2/2017
 *
 * Description: Helper methods for Active Objects (and their ActiveObjectType). 
 *              In a separate file for organizational reasons, especially avoid propagating 
 *              the firmware ActiveObjectType enum all over the place where it isn't actually needed.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "clad/types/activeObjectTypes.h"

#ifndef __Anki_Cozmo_ActiveObjectHelpers_H__
#define __Anki_Cozmo_ActiveObjectHelpers_H__

namespace Anki {
namespace Cozmo {
  
// Forward decl.
class ActiveObject;

// Helper method to instantiate a new ActiveObject by type. Returns nullptr if the given
// ObjectType cannot be mapped to a known active object type.
ActiveObject* CreateActiveObjectByType(ObjectType objType, ActiveID activeID, FactoryID factoryID);

bool IsValid(ActiveObjectType activeObjectType);
  
bool IsLightCube(ActiveObjectType activeObjectType);
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ActiveObjectHelpers_H__
