/**
 * File: activeObjectHelpers.h
 *
 * Author: Andrew Stein
 * Date:   3/2/2017
 *
 * Description: Helper methods for Active Objects (and their ObjectTypes).
 *              In a separate file for organizational reasons.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/


#ifndef __Anki_Cozmo_ActiveObjectHelpers_H__
#define __Anki_Cozmo_ActiveObjectHelpers_H__

namespace Anki {
namespace Cozmo {
  
// Forward decl.
class ActiveObject;

// Helper method to instantiate a new ActiveObject by type. Returns nullptr if the given
// ObjectType cannot be mapped to a known active object type.
ActiveObject* CreateActiveObjectByType(ObjectType objType, ActiveID activeID, FactoryID factoryID);
  
// ObjectType helpers:
bool IsLightCube(ObjectType objType);
bool IsCharger(ObjectType objType);
  
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_ActiveObjectHelpers_H__
