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

#include "engine/engineTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "json/json-forwards.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

// Forward declarations:
class BEIRobotInfo;
class MovementComponent;

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
  
  bool IsHeldInPalm() const { return _isHeldInPalm; }
  
  u32 GetTimeSinceLastHeldInPalm_ms() const;
  
  u32 GetHeldInPalmDuration_ms() const;

private:
  
  float _nextUpdateTime_s = 0.0f;
  
  EngineTimeStamp_t engineTime = 0;
  
  EngineTimeStamp_t _lastHeldInPalmTime = 0;
  EngineTimeStamp_t _lastTimeOnTreadsOrCharger = 0;
  EngineTimeStamp_t _lastTimeNotHeldInPalm = 0;
    
  bool _enoughCliffsDetectedSincePickup = false;
  
  bool _isHeldInPalm = false;

  std::vector<::Signal::SmartHandle> _eventHandles;
  
  void SetIsHeldInPalm(const bool isHeldInPalm, MovementComponent& moveComp);
  
  void CheckIfIsHeldInPalm(const BEIRobotInfo& robotInfo);
  
  // Helper functions to check whether robot has transitioned into user's palm:
  bool WasRobotPlacedInPalmWhileHeld(const BEIRobotInfo& robotInfo,
                                     const u32 timeToConfirmHeldInPalm_ms) const;
  
  bool HasDetectedEnoughCliffsSincePickup(const BEIRobotInfo& robotInfo) const;
  
  bool WasRobotMovingRecently(const BEIRobotInfo& robotInfo) const;
  
  // Auxilary helper functions:
  u32 GetMillisecondsSince(const EngineTimeStamp_t& pastTimestamp) const;

  // WebViz updates on trust-levels, last event, etc.
  void PopulateWebVizJson(Json::Value& data) const {}
};

}
}


#endif
