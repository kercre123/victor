/**
 * File: iFeedingListener.h
 *
 * Author: Kevin M. Karol
 * Created: 5/24/17
 *
 * Description: Interface for receiving notification when the feeding behavior
 * starts eating a cube, and ends eating a cube
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFeedingListener_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFeedingListener_H__


namespace Anki {
namespace Vector {
  
class IFeedingListener{
public:
  virtual void StartedEating(const int duration_s) = 0;
  virtual void EatingComplete() = 0;
  virtual void EatingInterrupted() = 0;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IFeedingListener_H__
