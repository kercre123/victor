/**
 * File: activeObjectTypeHelpers.cpp
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

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/charger.h"

namespace Anki {
namespace Cozmo {

ActiveObject* CreateActiveObjectByType(ObjectType objType, ActiveID activeID, FactoryID factoryID)
{
  ActiveObject* retPtr = nullptr;
  
  switch(objType)
  {
    case ObjectType::Block_LIGHTCUBE1:
    case ObjectType::Block_LIGHTCUBE2:
    case ObjectType::Block_LIGHTCUBE3:
    {
      retPtr = new ActiveCube(activeID, factoryID, objType);
      break;
    }
    case ObjectType::Charger_Basic:
    {
      retPtr = new Charger(activeID, factoryID, objType);
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("CreateActiveObjectByType.UnsupportedObjectType",
                        "Cannot create active object for given '%s' object type",
                        EnumToString(objType));
    }
  }
  return retPtr;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IsValid(ActiveObjectType activeObjectType)
{
 switch(activeObjectType)
  {
    case ActiveObjectType::OBJECT_UNKNOWN:
    case ActiveObjectType::OBJECT_OTA_FAIL:
      return false;
      
    case ActiveObjectType::OBJECT_CUBE1:
    case ActiveObjectType::OBJECT_CUBE2:
    case ActiveObjectType::OBJECT_CUBE3:
    case ActiveObjectType::OBJECT_CHARGER:
      return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IsLightCube(ActiveObjectType activeObjectType)
{
  switch(activeObjectType)
  {
    case ActiveObjectType::OBJECT_UNKNOWN:
    case ActiveObjectType::OBJECT_OTA_FAIL:
    case ActiveObjectType::OBJECT_CHARGER:
      return false;
      
    case ActiveObjectType::OBJECT_CUBE1:
    case ActiveObjectType::OBJECT_CUBE2:
    case ActiveObjectType::OBJECT_CUBE3:
      return true;
  }
  return false;
}

} // namespace Cozmo
} // namespace Anki

