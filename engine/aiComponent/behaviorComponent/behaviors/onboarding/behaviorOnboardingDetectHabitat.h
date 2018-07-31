/**
 * File: behaviorOnboardingDetectHabitat.h
 *
 * Author: ross
 * Created: 2018-07-26
 *
 * Description: passively detects being in the habitat during the first stage of onboarding
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingDetectHabitat__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingDetectHabitat__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorOnboardingDetectHabitat : public ICozmoBehavior
{
public: 
  virtual ~BehaviorOnboardingDetectHabitat() = default;
  
  void SetOnboardingStep( int step );

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorOnboardingDetectHabitat(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

private:
  
  bool GetProxReading( u16& distance_mm ) const;
  u16 GetDistanceDriven() const; // only valid once drivingComeHere has been set true
  
  enum class DetectionState : uint8_t {
    Indeterminate = 0, // can't be known until placed on charger
    OnChargerContacts, // waiting to drive off the charger to start checking sensor values
    Undetermined,      // passively checking sensor values to see if it is in habitat
    InHabitat,         // likely in the habitat
    NotInHabitat,      // likely not in the habitat
  };

  struct InstanceConfig {
    InstanceConfig();
    std::string behaviorIfDetectedStr;
    ICozmoBehaviorPtr behaviorIfDetected; // example of a good subbehavior: react to cliff then wait
  };

  struct DynamicVariables {
    DynamicVariables();
    DetectionState state;
    int lastOnboardingStep;
    bool drivingComeHere;
    bool activatedOnce;
    double bodyOdomAtInitComeHere_mm;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorOnboardingDetectHabitat__
