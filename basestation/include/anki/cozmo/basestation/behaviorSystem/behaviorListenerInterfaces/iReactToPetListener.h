/**
 * File: iReactToPetListener.h
 *
 * Author: Kevin M. Karol
 * Created: 12/14/16
 *
 * Description: Interface for receiving notifications from ReactToPet
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToPetListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToPetListener_H__


namespace Anki {
namespace Cozmo {

class IReactToPetListener{
public:
  virtual void RefreshReactedToIDs() = 0;
  virtual void UpdateLastReactionTime() = 0;
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToPetListener_H__