/**
 * File: behaviorReactToCliff.h
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff. This behavior actually handles both
 *              the stop and cliff events
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#ifndef __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
#define __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include <array>

namespace Anki {
namespace Vector {

class ICompoundAction;
  
class BehaviorReactToCliff : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToCliff(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  
  virtual void BehaviorUpdate() override;

private:  
  void TransitionToWaitForNoMotion();
  void TransitionToStuckOnEdge();
  void TransitionToPlayingCliffReaction();
  void TransitionToRecoveryBackup();
  void TransitionToHeadCalibration();
  void TransitionToVisualExtendCliffs();
  void TransitionToFaceAndBackAwayCliff();
  
  // Based on which cliff sensor(s) was tripped, create the appropriate reaction
  IActionRunner* GetCliffReactAction(uint8_t cliffDetectedFlags);

  // returns the cliff pose as estimated using the drop-sensor
  // note: the cliff in question will be the newly discovered
  //        one that caused this behavior to be triggered
  Pose3d GetCliffPoseToLookAt() const;
  
  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    ICozmoBehaviorPtr stuckOnEdgeBehavior;
    ICozmoBehaviorPtr askForHelpBehavior;
    
    float cliffBackupDist_mm;
    float cliffBackupSpeed_mmps;
    
    u32 eventFlagTimeout_ms;
  };

  InstanceConfig _iConfig;

  struct DynamicVariables {
    DynamicVariables();
    bool quitReaction;

    // whether the robot has received a cliff event with a valid cliff pose
    // that serves as the look-at target for any visual observation actions
    bool hasTargetCliff; 
    
    // Used to cancel behavior if picked up for too long
    TimeStamp_t lastPickupStartTime_ms;
    
    // used to determine where the robot searches for visual edges
    Pose3d cliffPose;

    struct Persistent {
      bool gotStop;
      int numStops;
      int numCliffReactAttempts;
      bool  putDownOnCliff;
      TimeStamp_t lastActiveTime_ms;
      std::array<u16, CliffSensorComponent::kNumCliffSensors> cliffValsAtStart;
    };
    Persistent persistent;
  };
  
  DynamicVariables _dVars;
  
}; // class BehaviorReactToCliff
  

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToCliff_H__
