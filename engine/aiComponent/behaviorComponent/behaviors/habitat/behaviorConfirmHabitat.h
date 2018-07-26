/**
 * File: behaviorConfirmHabitat.h
 *
 * Author: Arjun Menon
 * Date:   06 04 2018
 *
 * Description:
 * behavior that represents the robot's deliberate maneuvers to confirm whether
 *  it is inside the habitat after a pickup and putdown, or driving off charger
 *
 * the robot will engage in discrete actions to get to a configuration on top
 * of the white line so that its front cliff sensors are triggered. At this
 * position, the behavior will take no more actions since we are in the pose
 * to confirm the habitat in.
 *
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/habitatDetectionTypes.h"

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;

class BehaviorConfirmHabitat : public ICozmoBehavior
{
protected:

  friend class BehaviorFactory;
  BehaviorConfirmHabitat(const Json::Value& config);
  
public:

  virtual ~BehaviorConfirmHabitat();
  virtual bool WantsToBeActivatedBehavior() const override;
  void InitBehavior() override;
  
protected:

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override
  {
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  }
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void OnBehaviorEnteredActivatableScope() override;
  virtual void OnBehaviorLeftActivatableScope() override;
  virtual void BehaviorUpdate() override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  // state action helper methods
  void DelegateActionHelper(IActionRunner* action, RobotCompletedActionCallback&& callback);
  void TransitionToRandomWalk();
  void TransitionToSeekLineFromCharger();
  void TransitionToBackupForward();
  void TransitionToLocalizeCharger();
  void TransitionToReactToHabitat();
  void TransitionToReactToWhite(uint8_t whiteDetectedFlags);
  void TransitionToReactToWhite(HabitatLineRelPose lineRelPose);
  
  // helper motions to reposition the robot from certain line positions
  void TransitionToCliffAlignWhite();
  void TransitionToBackupTurnForward(int backDist_mm, f32 angle, int forwardDist_mm);
  void TransitionToTurnBackupForward(f32 angle, int backDist_mm, int forwardDist_mm);
  
  // returns nullptr if there is no charger seen
  const ObservableObject* GetChargerIfObserved() const;
  
  // Accumulate readings from ProxSensor when trying to check for close obstacles
  // returns whether there are enough readings to call CheckIsCloseToObstacle()
  bool UpdateProxSensorFilter();
  
  // perceive whether an object is very close to the robot
  // => this triggers a backup reaction to unstick us from tight situations
  bool CheckIsCloseToObstacle() const;
  
  int RandomSign() const
  {
    return (GetRNG().RandInt(2)==1) ? 1 : -1;
  }
  
  // steps in the process of confirming habitat
  enum class ConfirmHabitatPhase
  {
    InitialCheckObstacle,
    InitialCheckCharger,
    LocalizeCharger,
    SeekLineFromCharger,
    RandomWalk,
    LineDocking
  };
  
  struct DynamicVariables
  {
    // the current step or phase of habitat confirmation procedure
    ConfirmHabitatPhase _phase = ConfirmHabitatPhase::InitialCheckObstacle;
    
    // flag set if the random walk detects a nearby obstacle
    // this will gate choosing certain drive actions, or
    // make the robot take a different maneuver to unblock it
    bool _randomWalkTooCloseObstacle = false;
    
    // if we are in the unknown habitat state, then a stop-on-white
    // message is indicative of possibly being inside the habitat
    // so use this control flag to activate the behavior
    bool _robotStoppedOnWhite = false;
    
    // time of last reaction animation, for cooldown purposes
    float _nextWhiteReactionAnimTime_sec = 0.0;
    
    // buffer of prox sensor readings to accumulate in order to
    // check for a too-close obstacle in front of the robot
    std::vector<u16> _validProxDistances = {};
    
    u32 _numTicksWaitingForProx = 0;

    explicit DynamicVariables() = default;
  };
  
  struct InstanceConfig
  {
    IBEIConditionPtr onTreadsTimeCondition;
    
    std::string searchForChargerBehaviorIDStr;
    ICozmoBehaviorPtr searchForChargerBehavior;
    
    explicit InstanceConfig();
  };
  
  // persist this filter because we use it to search for the charger
  std::unique_ptr<BlockWorldFilter> _chargerFilter;
  
  DynamicVariables _dVars;
  InstanceConfig _iConfig;
};

}
}

#endif//__Engine_AiComponent_BehaviorComponent_Behaviors_Habitat_BehaviorConfirmHabitat_H__
