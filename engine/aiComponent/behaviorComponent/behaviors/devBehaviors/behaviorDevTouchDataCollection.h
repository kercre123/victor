/**
 * File: BehaviorDevTouchDataCollection.h
 *
 * Author: Arjun Menon
 * Date:   01/08/2018
 *
 * Description: simple test behavior to respond to touch
 *              and petting input. Does nothing until a
 *              touch event comes in for it to react to
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDevTouchDataCollection_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDevTouchDataCollection_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include <vector>

namespace Anki {
namespace Vector {
  
class IBEICondition;
  
class BehaviorDevTouchDataCollection : public ICozmoBehavior
{
public:
  
  virtual ~BehaviorDevTouchDataCollection() { }
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = false;
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  
protected:

  using BExtI = BehaviorExternalInterface;
  
  friend class BehaviorFactory;
  BehaviorDevTouchDataCollection(const Json::Value& config);
  
  void InitBehavior() override;
  
  virtual void OnBehaviorActivated() override;

  virtual void OnBehaviorDeactivated() override;
  
  virtual void BehaviorUpdate() override;

  virtual void HandleWhileActivated(const RobotToEngineEvent& event) override;

  void EnqueueMotorActions();
  void LoopMotorsActionCallback(ActionResult res);
  bool RobotConfigMatchesExpected(BExtI& bexi) const;

private:
  enum DataAnnotation : int{
    NoTouch,
    SustainedContact,
    PettingSoft,
    PettingHard,
  };

  enum CollectionMode : int {
    IdleChrgOff,
    IdleChrgOn,
    MoveChrgOff,
    MoveChrgOn
  };

  struct InstanceConfig {
    InstanceConfig();

  };

  struct DynamicVariables {
    DynamicVariables();
    // the current type of touch to collect for
    DataAnnotation        annotation;
    
    std::vector<uint16_t> touchValues;
    // the condition under which to collection the requisite touch samples
    // note: this reflects the motor state (and checks for valid charger)
    CollectionMode        collectMode;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;


  std::pair<std::string,std::string> ToString(DataAnnotation da) const;
  std::pair<std::string,std::string> ToString(CollectionMode cm) const;
  void OnNextDataset(BehaviorExternalInterface& bexi,
                     CollectionMode cm, DataAnnotation da) const;
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorDevTouchDataCollection_H__
