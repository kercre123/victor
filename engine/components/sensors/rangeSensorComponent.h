/**
 * File: rangeSensorComponent.h
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Component for managing Whiskey's head mounted ToF sensor
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_RangeSensorComponent_H__
#define __Engine_Components_RangeSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "coretech/common/engine/math/pose.h"

#include "util/entityComponent/entity.h"

#include "clad/types/tofTypes.h"

#include "util/signals/simpleSignal_fwd.h"

#include <vector>

namespace Anki {
namespace Vector {

class RangeSensorComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:
  RangeSensorComponent();
  ~RangeSensorComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override;

  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override { }  
  
  virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override { }
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override { }
  
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

  //////
  // end IDependencyManagedComponent functions
  //////

  using RangeData = std::array<Vec3f, 16>;
  const RangeData& GetLatestRangeData() const { return _latestRangeData; }
  const RangeDataRaw& GetLatestRawRangeData(bool& isNew) const { isNew = _rawDataIsNew; return _latestRawRangeData; }
  
private:

  Robot* _robot = nullptr;

  Signal::SmartHandle _signalHandle;
  
  RangeDataRaw _latestRawRangeData;
  RangeData _latestRangeData;
  bool _rawDataIsNew = false;

  bool _sendRangeData = false;
};


}
}

#endif
