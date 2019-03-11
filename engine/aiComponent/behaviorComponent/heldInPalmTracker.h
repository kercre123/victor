/**
 * File: heldInPalmTracker.h
 *
 * Author: Guillermo Bautista
 * Created: 2019-02-05
 *
 * Description: Behavior component to track various metrics, such as the
 * "trust" level that the robot has when being held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_Components_HeldInPalmTracker_H__
#define __Engine_Components_HeldInPalmTracker_H__
#pragma once

#include "coretech/common/shared/types.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "json/json-forwards.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <unordered_map>

namespace Anki {
namespace Vector {


class HeldInPalmTracker : public IDependencyManagedComponent<BCComponentID>,
                          public Anki::Util::noncopyable
{
public:
    
  HeldInPalmTracker();

  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComps ) override;
  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::RobotInfo);
  }
  virtual void GetUpdateDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::RobotInfo);
  }
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;
  
  void SetIsHeldInPalm(const bool isHeldInPalm);
  
  bool IsHeldInPalm() const { return _isHeldInPalm; }

private:

  float _nextUpdateTime_s = 0.0f;
  bool _isHeldInPalm = false;

  std::vector<::Signal::SmartHandle> _eventHandles;

  // WebViz updates on trust-levels, last event, etc.
  void PopulateWebVizJson(Json::Value& data) const {}
};

}
}


#endif
