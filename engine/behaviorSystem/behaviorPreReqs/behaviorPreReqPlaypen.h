/**
 * File: behaviorPreReqPlaypen.h
 *
 * Author: Al Chaussee
 * Created: 07/24/17
 *
 * Description: Defines prerequisites that can be passed into
 * behavior's isRunnable()
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPrePlaypen_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPrePlaypen_H__

namespace Anki {
namespace Cozmo {

class BehaviorPreReqPlaypen {
public:
  BehaviorPreReqPlaypen(FactoryTestLogger& factoryTestLogger)
  :_factoryTestLogger(factoryTestLogger){}
  
  FactoryTestLogger& GetFactoryTestLogger() const { return _factoryTestLogger;}
  
private:
  FactoryTestLogger& _factoryTestLogger;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPrePlaypen_H__


