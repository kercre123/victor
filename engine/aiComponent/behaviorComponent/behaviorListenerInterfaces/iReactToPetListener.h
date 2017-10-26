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

#include "anki/vision/basestation/faceIdTypes.h"

namespace Anki {
namespace Cozmo {

class IReactToPetListener {
public:
  
  // Notify listener when reaction ends
  virtual void BehaviorDidReact(const std::set<Vision::FaceID_t>& targets) = 0;
  
};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorListenerInterfaces_IReactToPetListener_H__
