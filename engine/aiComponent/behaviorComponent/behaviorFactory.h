/**
 * File: behaviorFactory.h
 *
 * Author: Brad Neuman
 * Created: 2018-02-20
 *
 * Description: Factory for creating behaviors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorFactory_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorFactory_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class BehaviorFactory : private Util::noncopyable
{
public:

  // Create and return a pointer to a behavior specified by the given json. The caller must take ownership of
  // this pointer
  static ICozmoBehaviorPtr CreateBehavior(const Json::Value& config);
};

}
}

#endif
