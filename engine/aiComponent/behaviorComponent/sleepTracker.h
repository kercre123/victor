/**
 * File: sleepTracker.h
 *
 * Author: Brad Neuman
 * Created: 2018-08-15
 *
 * Description: Behavior component to track sleep and sleepiness
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_SleepTracker_H__
#define __Engine_Components_SleepTracker_H__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "json/json-forwards.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

class SleepTracker : public IDependencyManagedComponent<BCComponentID>
                   , public Anki::Util::noncopyable
{
public:
  SleepTracker();

  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComps ) override;
  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::RobotInfo);
  }
  virtual void GetUpdateDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::RobotInfo);
  }
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;

  void SetIsSleeping(bool sleeping);

  // true if the robot has accumulated enough "sleep debt" to want to sleep for at least an hour (if not
  // coming from sleep). If coming from sleep, then remain sleepy until sleep debt goes to zero. Does not take
  // stimulation into account at all.
  bool IsSleepy(bool fromSleep) const;

  // check if local time is within the defined "night time" hours
  bool IsNightTime() const;

  // Add sleep debt if the current debt is below the provided min. This can be useful to make sure the robot
  // wants to say asleep for at least a certain amount of time
  void EnsureSleepDebtAtLeast(float minDebt_s);

private:

  // "sleep debt" tracking
  float _sleepDebt_s = 0.0f;

  float _nextUpdateTime_s = 0.0f;
  bool _asleep = false;

  std::vector<::Signal::SmartHandle> _eventHandles;

  void PopulateWebVizJson(Json::Value& data) const;
};

}
}


#endif
