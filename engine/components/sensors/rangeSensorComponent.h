/**
 * File: proxSensorComponent.h
 *
 * Author: Al Chaussee
 * Created: 10/19/2018
 *
 * Description: Component for managing range sensor data
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_RangeSensorComponent_H__
#define __Engine_Components_RangeSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "coretech/common/engine/math/pose.h"

#include "util/entityComponent/entity.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class RangeSensorComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:
  RangeSensorComponent();
  ~RangeSensorComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override
  {
    _robot = robot;
  }
  
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override { }  
  
  virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override { }
    virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override { }

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////

private:

  struct RangeData
  {
    Pose3d sensorPoint;
    Pose3d worldPoint;
  };
  void UpdateNavMap(const std::vector<RangeData>& data);

  Robot* _robot = nullptr;
  
};


}
}

#endif
