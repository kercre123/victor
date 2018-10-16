/**
 * File: proxSensorComponent.h
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_RangeSensorComponent_H__
#define __Engine_Components_RangeSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "clad/types/proxMessages.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/entity.h"
#include "coretech/common/engine/math/pose.h"

namespace Anki {
namespace Vector {


class RangeSensorComponent :public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  RangeSensorComponent();
  ~RangeSensorComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override {
    InitBase(robot);
  };

  //////
  // end IDependencyManagedComponent functions
  //////

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
private:

  void UpdateNavMap();
  
};


} // Cozmo namespace
} // Anki namespace

#endif
