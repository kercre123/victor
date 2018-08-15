/**
 * File: contextWrapper.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-27
 *
 * Description: Wrapper for CozmoContext to allow it to be accessed like a component
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_ContextWrapper_H__
#define __Engine_ContextWrapper_H__

#include "engine/robotComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

namespace Anki {
namespace Vector {

class CozmoContext;

class ContextWrapper:  public IDependencyManagedComponent<RobotComponentID> {
public:
  ContextWrapper(const CozmoContext* context)
  : IDependencyManagedComponent(this, RobotComponentID::CozmoContextWrapper)
  , context(context){}
  const CozmoContext* context;

  virtual ~ContextWrapper(){}

  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override {};
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
};

}
}

#endif
