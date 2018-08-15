/**
 * File: iReactToObjectListener.h
 *
 * Author: Kevin M. Karol
 * Created: 12/14/16
 *
 * Description: Interface for receiving notifications from ReactToObject
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToObjectListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToObjectListener_H__


namespace Anki {
namespace Vector {

class IReactToObjectListener{
public:
  virtual void ReactedToID(s32 id) = 0;
  virtual void ClearDesiredTargets() = 0;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToObjectListener_H__
