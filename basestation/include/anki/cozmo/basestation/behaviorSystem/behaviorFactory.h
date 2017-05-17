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


#include "clad/types/behaviorTypes.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <map>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {


class IBehavior;
class IScoredBehavior;
class IReactionaryBehavior;
class Robot;

  
class BehaviorFactory : private Util::noncopyable
{
public:
  
  using BehaviorIDToBehaviorMap = std::map<BehaviorID, IBehavior*>;
  
  // NameCollisionRule dictates how to handle name collision on creation - reuse existing entry, replace it, or error and return null
  enum class NameCollisionRule : uint8_t
  {
    ReuseOld,
    OverwriteWithNew,
    Fail
  };
  
  BehaviorFactory(Robot& robot);
  ~BehaviorFactory();
  
  IBehavior* CreateBehavior(BehaviorClass behaviorType, Robot& robot, const Json::Value& config, NameCollisionRule nameCollisionRule = NameCollisionRule::ReuseOld);
  IBehavior* CreateBehavior(const Json::Value& behaviorJson, Robot& robot, NameCollisionRule nameCollisionRule = NameCollisionRule::ReuseOld);


  void DestroyBehavior(IBehavior* behavior);
  void SafeDestroyBehavior(IBehavior*& behaviorPtrRef); // destroy and null out the pointer
  
  IBehavior* FindBehaviorByID(BehaviorID behaviorID) const;
  IBehavior* FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
  const BehaviorIDToBehaviorMap& GetBehaviorMap() const { return _idToBehaviorMap; }

  
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
private:

  // ============================== Private Member Funcs ==============================
  
  // NOTE: can modify newBehavior (e.g. on name collision if rule is to reuse existing behavior)
  IBehavior* AddToFactory(IBehavior* newBehavior, NameCollisionRule nameCollisionRule);

  
  bool RemoveBehaviorFromMap(IBehavior* behavior);
  void DeleteBehaviorInternal(IBehavior* behavior);
  
  // ============================== Private Member Vars ==============================
  Robot& _robot;
  BehaviorIDToBehaviorMap _idToBehaviorMap;
  std::vector<Signal::SmartHandle> _signalHandles;


};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorFactory_H__

