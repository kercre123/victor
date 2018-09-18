/**
 * File: imuComponent.h
 *
 * Author: Michael Willett
 * Created: 9/18/2018
 *
 * Description: Component for managing imu data received from robot process
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_ImuComponent_H__
#define __Engine_Components_ImuComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "util/entityComponent/entity.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include <deque>

namespace Anki {
namespace Vector {

class ImuComponent :public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  ImuComponent();
  virtual ~ImuComponent() override = default;

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

  using ImuHistory = std::deque<RobotInterface::IMUDataFrame>;
  const ImuHistory& GetImuHistory() const { return _imuHistory; }

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override {};
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
public:

  // process a gyro frame and add it to history
  void AddData(RobotInterface::IMUDataFrame&& frame);

  // Gets the imu data before and after the timestamp
  bool GetImuDataBeforeAndAfter(RobotTimeStamp_t t, 
                                RobotInterface::IMUDataFrame& before, 
                                RobotInterface::IMUDataFrame& after) const;

  // Returns true if the any of the numToLookBack imu data before timestamp t have rates that are greater than
  // the given rates
  bool IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                      const int numToLookBack,
                                      const f32 rateX, const f32 rateY, const f32 rateZ) const;

private:
  ImuHistory _imuHistory;

};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
