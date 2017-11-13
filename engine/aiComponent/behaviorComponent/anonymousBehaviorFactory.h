/**
* File: anonymousBehaviorFactory.h
*
* Author: Kevin M. Karol
* Created: 11/3/17
*
* Description: Component that allows anonymous behaviors to be created at runtime
* Anonymous behaviors are not stored in the behavior container and will be destroyed
* if the pointer returned from this factory leaves scope
*
* Copyright: Anki, Inc. 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_AnonymousBehaviorFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_AnonymousBehaviorFactory_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorContainer;

class AnonymousBehaviorFactory : private Util::noncopyable
{
public:
  AnonymousBehaviorFactory(const BehaviorContainer& bContainer);
  virtual ~AnonymousBehaviorFactory();

  // Pass in the behavior type and any parameters that a behavior of that type requires/accepts
  // parameters should NOT include a BehaviorID
  ICozmoBehaviorPtr CreateBehavior(BehaviorClass behaviorType, Json::Value& parameters);
  
  
private:
  const BehaviorContainer& _behaviorContainer;
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_AnonymousBehaviorFactory_H__
