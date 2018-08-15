/**
 * File: backpackLightComponentTypes.h
 *
 * Author: Lee Crippen
 * Created: 2/13/2017
 *
 * Description: Types related to managing various lights on Cozmo's body.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BackpackLightComponentTypes_H__
#define __Anki_Cozmo_Basestation_Components_BackpackLightComponentTypes_H__

#include <list>
#include <map>
#include <memory>

namespace Anki {
namespace Vector {

struct BackpackLightData;
class BackpackLightComponent;
 
using BackpackLightDataRef = std::shared_ptr<BackpackLightData>;
using BackpackLightDataRefWeak = std::weak_ptr<BackpackLightData>;

using BackpackLightList = std::list<BackpackLightDataRef>;

class BackpackLightDataLocator
{
public:
  bool IsValid() const { return !_dataPtr.expired(); }
  
private:
  friend class BackpackLightComponent;
  
  BackpackLightList::iterator         _listIter;
  BackpackLightDataRefWeak            _dataPtr;
  
}; // class LightDataLocator
  
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Components_BackpackLightComponentTypes_H__
