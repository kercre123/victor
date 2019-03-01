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
#include "clad/types/behaviorComponent/heldInPalmTrustEventTypes.h"

#include <unordered_map>

namespace Anki {
namespace Vector {

namespace Audio{
  class EngineRobotAudioClient;
}

class CozmoFeatureGate;

class HeldInPalmTracker : public IDependencyManagedComponent<BCComponentID>,
                          public Anki::Util::noncopyable
{
public:
  
  using TrustEventType = HeldInPalmTrustEventType;
  
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
  
  // An external behavior can cause the trust level to change if it tracks and observes one of the specified
  // events from <event enum name here> occurring, and notifies this component.
  void UpdateTrustLevelForEvent( const TrustEventType& trustEventType );
  
  float GetTrustLevel() const { return _trustLevel; }
  
  bool IsHeldInPalm() const { return _isHeldInPalm; }

private:
  
  // Clamps trust level to be between kMinTrustLevel and kMaxTrustLevel, warns the user if desired when a
  // caller tries to set the level outside of these limits.
  void SetTrustLevel(const float trustLevel, const bool verboseWarn = true);

  // Held-In-Palm "trust" tracking
  float _trustLevel = 0.0f;
  std::unordered_map<TrustEventType, float> _trustEvents;
  TrustEventType _lastTrustEventType = TrustEventType::Invalid;

  float _nextUpdateTime_s = 0.0f;
  bool _isHeldInPalm = false;

  std::vector<::Signal::SmartHandle> _eventHandles;
  
  // Audio engine updates
  void SendTrustLevelToAudio();
  Audio::EngineRobotAudioClient* _audioClient;
  // Needed for checks when updating audio engine
  CozmoFeatureGate* _featureGate = nullptr;

  // WebViz updates on trust-levels, last event, etc.
  void PopulateWebVizJson(Json::Value& data) const;
};

}
}


#endif
