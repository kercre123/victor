/**
 * File: activeObjectHelpers.cpp
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

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/charger.h"

namespace Anki {
namespace Cozmo {

ActiveObject* CreateActiveObjectByType(ObjectType objType, ActiveID activeID, FactoryID factoryID)
{
  ActiveObject* retPtr = nullptr;
  
  if (IsValidLightCube(objType, false)) {
    retPtr = new ActiveCube(activeID, factoryID, objType);
  } else if (IsCharger(objType, false)) {
    retPtr = new Charger(activeID, factoryID, objType);
  } else {
    PRINT_NAMED_ERROR("CreateActiveObjectByType.UnsupportedObjectType",
                      "Cannot create active object for given '%s' object type",
                      EnumToString(objType));
  }
  
  return retPtr;
}


} // namespace Cozmo
} // namespace Anki
