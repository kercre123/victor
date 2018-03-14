/**
 * File: behaviorKeepaway.h
 *
 * Author: Sam Russell
 * Date:   02/26/2018
 *
 * Description: Victor attempts to pounce on an object while the user tries to dodge the pounce
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorKeepaway_H_
#define __Cozmo_Basestation_Behaviors_BehaviorKeepaway_H_

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki{
namespace Cozmo{

// Forward Declarations
class BlockWorldFilter;

class BehaviorKeepaway : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorKeepaway(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = false;
    modifiers.behaviorAlwaysDelegates = false;
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  }

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

private:

  enum class KeepawayState{
    GetIn,
    Searching,
    Stalking,
    Creeping,
    Primed,
    Pouncing,
    FakeOut,
    Reacting,
    GetOut
  };

  enum class PounceReadyState{
    Unready,
    Ready
  };

  void SetState_internal(KeepawayState state, const std::string& stateName);

  void TransitionToSearching();
  void UpdateSearching();

  void TransitionToStalking();
  void UpdateStalking();

  void TransitionToCreeping();
  void UpdateCreeping();

  void TransitionToPrimed();
  void UpdatePrimed();

  void TransitionToPouncing();
  void TransitionToFakeOut();
  void TransitionToReacting();
  void TransitionToGetOut();

  bool CheckTargetStatus();
  bool CheckTargetObject();
  void UpdateTargetVisibility();
  void UpdateTargetAngle();
  void UpdateTargetDistance();
  void UpdateTargetMotion();
  bool TargetHasMoved(const ObservableObject* object);
  bool PitchIndicatesPounceSuccess() const;

  float GetCurrentTimeInSeconds() const;

  struct TargetStatus {
    bool isValid = false;
    bool isVisible = false;
    bool isInPlay = false;
    bool isOffCenter = false;
    bool isNotMoving = false;
    bool isInPounceRange = false;
    bool isInInstaPounceRange = false;
  };

  struct InstanceConfig{
    InstanceConfig(const Json::Value& config);
    std::unique_ptr<BlockWorldFilter> keepawayTargetFilter;
    float   naturalGameEndTimeout_s;
    float   targetUnmovedGameEndTimeout_s;
    float   noVisibleTargetGameEndTimeout_s;
    float   noPointsEarnedTimeout_s;
    float   targetVisibleTimeout_s;
    float   animDistanceOffset_mm;
    float   inPlayDistance_mm;
    float   outOfPlayDistance_mm;
    float   allowablePointingError_deg;
    float   targetUnmovedDistance_mm;
    float   targetUnmovedAngle_deg;
    float   targetUnmovedCreepTimeout_s;
    float   creepDistanceMin_mm;
    float   creepDistanceMax_mm;
    float   creepDelayTimeMin_s;
    float   creepDelayTimeMax_s;
    float   pounceDelayTimeMin_s;
    float   pounceDelayTimeMax_s;
    float   basePounceChance;
    float   pounceChanceIncrement;
    float   nominalPounceDistance_mm;
    float   instaPounceDistance_mm;
    float   pounceSuccessPitchDiff_deg;
    uint8_t pointsToWin;
  };

  struct DynamicVariables{
    DynamicVariables(const InstanceConfig& iConfig);
    KeepawayState    state;
    ObjectID         targetID;
    TargetStatus     target;
    PounceReadyState pounceReadyState;
    float            gameStartTime_s;
    float            targetLastValidTime_s;
    float            targetLastObservedTime_s;
    float            targetLastMovedTime_s;
    Pose3d           targetPrevPose;
    float            targetDistance;
    float            creepTime;
    float            pounceChance;
    float            pounceTime;
    float            pounceSuccessPitch_deg;
    uint8_t          victorPoints;
    uint8_t          userPoints;
    bool             victorGotLastPoint;
    bool             gameOver;
    bool             victorIsBored;
   };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorKeepaway_H_