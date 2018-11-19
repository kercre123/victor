/**
 * File: BehaviorVolume.h
 *
 * Author: Andrew Stout
 * Created: 2018-10-11
 *
 * Description: Respond to volume change requests.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVolume__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVolume__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"

namespace Anki {
namespace Vector {

//class Robot;
class SettingsManager;

// note that if you're changing this enum, you're likely to also want to change
// kVolumeLevelMap and kVolumeLevelAnimMap in behaviorVolume.cpp.
enum class EVolumeLevel : uint32_t {
  MUTE = 0, // don't ever actually set the volume to this (as a matter of character policy)
  MIN = 1,
  MEDLOW = 2,
  MED = 3,
  MEDHIGH = 4,
  MAX = 5
};

class BehaviorVolume : public ICozmoBehavior
{
public: 
  virtual ~BehaviorVolume();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorVolume(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;

private:

  struct DynamicVariables {
    DynamicVariables();
    float lastVolumeChangeNotification_s;
    uint32_t volumeChangeReactorId;
  };

  bool SetVolume(EVolumeLevel desiredVolume);
  EVolumeLevel ComputeDesiredVolumeFromLevelIntent(UserIntentPtr intentData, bool& valid);
  EVolumeLevel ComputeDesiredVolumeFromIncrement(bool positiveIncrement);
  void OnVolumeChange();
  bool ExternalNotificationPending() const;

  DynamicVariables _dVars;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVolume__
