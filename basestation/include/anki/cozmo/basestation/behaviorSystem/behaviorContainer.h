/**
 * File: behaviorContainer
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Container which creates and stores behaviors by ID 
 * which were generated from data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__


#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior_fwd.h"

#include "clad/types/behaviorTypes.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {


class IReactionaryBehavior;
class Robot;

  
class BehaviorContainer : private Util::noncopyable
{
public:
  ~BehaviorContainer();
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  // Behavior manager should be the only clas that directly interacts with the
  // container
  friend class BehaviorManager;
  BehaviorContainer(Robot& robot);
  
  
  IBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  IBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
  // Check to ensure that the factory only includes one behavior per executable
  // type
  void VerifyExecutableBehaviors() const;
  

  
  
  using BehaviorIDToBehaviorMap = std::map<BehaviorID, IBehaviorPtr>;

  IBehaviorPtr CreateBehavior(BehaviorClass behaviorType, Robot& robot, const Json::Value& config);
  IBehaviorPtr CreateBehavior(const Json::Value& behaviorJson, Robot& robot);
  
#if ANKI_DEV_CHEATS
  const BehaviorIDToBehaviorMap& GetBehaviorMap() const { return _idToBehaviorMap; }
#endif
  
private:

  // ============================== Private Member Funcs ==============================
  
  // NOTE: can modify newBehavior (e.g. on name collision if rule is to reuse existing behavior)
  IBehaviorPtr AddToFactory(IBehaviorPtr newBehavior);
  
  bool RemoveBehaviorFromMap(IBehaviorPtr behavior);
  
  // ============================== Private Member Vars ==============================
  Robot& _robot;
  BehaviorIDToBehaviorMap _idToBehaviorMap;
  std::vector<Signal::SmartHandle> _signalHandles;


};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__

