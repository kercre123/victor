/**
 * File: behaviorReactToDarkness.h
 *
 * Author: Humphrey Hu
 * Created: 2018-06-03
 *
 * Description: Behavior to react to different illumination darkening
 * 
 * When the positive illumination condition is met, the robot will look around, confirming
 * each time that the positive condition is met and the negative condition is not. If all
 * of these checks are passed then a specified emotion will be triggered and the sleep
 * state suggested to the HLAI. If not, the behavior cancels upon the first failed check.
 * 
 * The sensitivity/reaction speed can be tuned by setting the illumination condition parameters.
 **/
 
 #ifndef __Victor_Behaviors_BehaviorReactToDarkness_H__
 #define __Victor_Behaviors_BehaviorReactToDarkness_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ConditionIlluminationDetected;

class BehaviorReactToDarkness : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorReactToDarkness( const Json::Value& config );

public:

  virtual bool WantsToBeActivatedBehavior() const override;

protected:

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override
  {
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  virtual void BehaviorUpdate() override;

  struct InstanceConfig
  {
    Json::Value positiveConfig;                                       // Config for the positive illumination condition
    Json::Value negativeConfig;                                       // Config for the negative illumination condition
    f32 lookHeadAngleMax_deg;                                         // Max angle in radians the head will tilt to when looking
    f32 lookHeadAngleMin_deg;                                         // Min angle in radians the head will tilt to when looking
    f32 lookTurnAngleMax_deg;                                         // Max absolute angle in radians the robot will turn when looking
    f32 lookTurnAngleMin_deg;                                         // Min absolute angle in radians the robot will turn when looking
    f32 lookWaitTime_s;                                               // The max time in seconds the robot will look before canceling
    u32 numOnChargerLooks;                                            // The number of times to look around to confirm when on charger
    u32 numOffChargerLooks;                                           // The number of times to look around to confirm when off charger
    std::string reactionEmotion;                                      // The reaction emotion to trigger on confirmation
    bool sleepInPlace;                                                // Whether to go home or sleep where he is
    std::unique_ptr<BlockWorldFilter> homeFilter;                     // Used to determine if we know where a charger is
    std::shared_ptr<ConditionIlluminationDetected> negativeCondition; // Condition for detecting room not dark
    std::shared_ptr<ConditionIlluminationDetected> positiveCondition; // Condition for detecting room dark
    ICozmoBehaviorPtr goHomeBehavior;

    InstanceConfig() {
      homeFilter = std::make_unique<BlockWorldFilter>();
    }
  };

  enum class State
  {
    Waiting,          // Waiting to start
    Turning,          // Turning to look
    TurnedAndWaiting, // Waiting for condition
    Responding,       // Animating and adjusting emotion/timers
    GoingHome,        // Delegating to FindAndGoHome
  };

  struct DynamicVariables
  {
    State state;
    f32 waitStartTime;
    u32 numChecksSucceeded;

    void Reset();
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToTurning();
  void TransitionToTurnedAndWaiting();
  void TransitionToResponding();
  void TransitionToGoingHome();
  bool LookedEnoughTimes() const;
};
  

}
}

 #endif
