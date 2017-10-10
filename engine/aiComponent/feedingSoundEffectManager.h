/**
* File: feedingSoundEffectManager.h
*
* Author: Kevin M. Karol
* Created: 09/27/17
*
* Description: Coordinates sound effect events across various feeding
* cubes/activities etc
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Engine_AIComponent_FeedingSoundEffectManager_H__
#define __Cozmo_Engine_AIComponent_FeedingSoundEffectManager_H__


#include "anki/common/basestation/objectIDs.h"


namespace Anki {
class ObjectID;
namespace Cozmo {
  
// forward declarations
class Robot;
  
class FeedingSoundEffectManager{
public:
  // As defined in messageEngineToGame.clad
  enum class ChargeStateChange{
    Charge_Start = 0,
    Charge_Up = 1,
    Charge_Down = 2,
    Charge_Complete = 3,
    Charge_Stop = 4
  };
  
  FeedingSoundEffectManager();
  
  void NotifyChargeStateChange(Robot& robot,
                               const ObjectID& objID, int currentChargeLevel,
                               ChargeStateChange changeEnumVal);

private:
  ObjectID _dominantObject;
  int _dominantChargeLevel;
  ChargeStateChange _dominantChargeChange;
  
  void ResetChargeSound(Robot& robot);
  
};
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Engine_AIComponent_FeedingSoundEffectManager_H__
