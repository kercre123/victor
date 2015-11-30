/**
 * File: behaviorFactory
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Factory for creating behaviors from data / messages
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorFactory_H__


#include "clad/types/behaviorType.h"
#include <map>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {


class IBehavior;
class Robot;

  
class BehaviorFactory
{
public:
  
//  BehaviorFactory();
  
  static IBehavior* CreateBehavior(BehaviorType behaviorType, Robot& robot, const Json::Value& config);
  
private:
  
//  // ============================== Private Member Funcs ==============================
//  
//
//    
//  // ============================== Private Member Vars ==============================
//
//  using BehaviorHandle = uint32_t;
//  using HandleToBehaviorMap = std::map<BehaviorHandle, IBehavior*>;
//  
//  //using = std::map<std::string, IBehavior*>
//  
//  BehaviorHandle        _nextBehaviorHandle;
//  HandleToBehaviorMap   _handleToBehaviorMap;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorFactory_H__

