/**
* File: behaviorWallTimeCoordinator.h
*
* Author: Kevin M. Karol
* Created: 2018-06-15
*
* Description: Manage the designed response to a user request for the wall time
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWallTimeCoordinator__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWallTimeCoordinator__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

// Fwd Declarations
class BehaviorDisplayWallTime;
enum class UtteranceState;

class BehaviorWallTimeCoordinator : public ICozmoBehavior
{
public: 
  virtual ~BehaviorWallTimeCoordinator();

  // return a string which can be passed into TTS to say the time correctly 
  static std::string GetTTSStringForTime(struct tm& localTime, const bool use24HourTime);

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorWallTimeCoordinator(const Json::Value& config);  

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

private:
  using AudioTtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
  struct InstanceConfig {
    InstanceConfig();

    ICozmoBehaviorPtr                        iCantDoThatBehavior;
    ICozmoBehaviorPtr                        lookAtFaceInFront;
    std::shared_ptr<BehaviorDisplayWallTime> showWallTime;
  };

  struct DynamicVariables {
    DynamicVariables();
    bool shouldSayTime;
    uint8_t utteranceID;
    UtteranceState  utteranceState;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToICantDoThat();
  void TransitionToFindFaceInFront();
  void TransitionToShowWallTime();

  void StartTTSGeneration();

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorWallTimeCoordinator__
