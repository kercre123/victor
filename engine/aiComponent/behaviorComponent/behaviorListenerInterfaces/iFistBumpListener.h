/**
 * File: iFistBumpListener.h
 *
 * Author: Kevin Yoon
 * Created: 02/14/17
 *
 * Description: Interface for receiving notifications from BehaviorFistBump
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFistBumpListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFistBumpListener_H__


namespace Anki {
namespace Vector {

class IFistBumpListener{
public:
  virtual void ResetTrigger(bool updateLastCompletionTime) = 0;
};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFistBumpListener_H__

