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
  
bool IsLightCube(ObjectType objType)
{
  // Note: COZMO-9856 (implement 'EnumConcept' for CLAD) would
  //  obviate the need for this function (and it would cover the
  //  case of potentially adding more light cubes in the future)
  return (objType == ObjectType::Block_LIGHTCUBE1) ||
         (objType == ObjectType::Block_LIGHTCUBE2) ||
         (objType == ObjectType::Block_LIGHTCUBE3);
};

bool IsCharger(ObjectType objType)
{
  return (objType == ObjectType::Charger_Basic);
};


} // namespace Cozmo
} // namespace Anki
