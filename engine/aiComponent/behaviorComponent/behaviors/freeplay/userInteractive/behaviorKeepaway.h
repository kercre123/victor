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
namespace Vector{

// Forward Declarations
class BlockWorldFilter;
struct TargetStatus;

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
    modifiers.behaviorAlwaysDelegates = true;
    modifiers.wantsToBeActivatedWhenOnCharger = false;
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

private:

  enum class KeepawayState{
    GetIn,
    Searching,
    Stalking,
    Creeping,
    Pouncing,
    FakeOut,
    Reacting,
    GetOut
  };

  enum class PounceReadyState{
    Unready,
    Ready
  };

  void UpdateTargetStatus();
  void CheckGetOutTimeOuts();

  void TransitionToGetIn();
  void TransitionToSearching();
  void TransitionToStalking();
  void UpdateStalking();

  void TransitionToCreeping();
  void TransitionToPouncing();
  void TransitionToFakeOut();
  void TransitionToReacting();
  void TransitionToGetOutBored();

  bool PitchIndicatesPounceSuccess() const;

  void StartIdleAnimation();
  void StopIdleAnimation();

  float GetCurrentTimeInSeconds() const;

  struct KeepawayTarget {
    bool isValid = false;
    bool isVisible = false;
    bool isInPlay = false;
    bool isOffCenter = false;
    bool isNotMoving = false;
    bool isInPounceRange = false;
    bool isInMousetrapRange = false;
  };

  struct InstanceConfig{
    InstanceConfig(const Json::Value& config);
    float   targetUnmovedGameEndTimeout_s;
    float   noVisibleTargetGameEndTimeout_s;
    float   targetVisibleTimeout_s;
    float   globalOffsetDist_mm;
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
    float   mousetrapPounceDistance_mm;
    float   probBackupInsteadOfMousetrap;
    float   pounceSuccessPitchDiff_deg;
    float   excitementIncPerHit;
    float   maxProbExitExcited;
    float   frustrationIncPerMiss;
    float   maxProbExitFrustrated;
    float   minProbToExit;
    bool    useProxForDistance;
      
    std::vector<std::string> floatNames; // autofilled names of the above floats
  };

  struct DynamicVariables{
    DynamicVariables(const InstanceConfig& iConfig);
    KeepawayState    state;
    ObjectID         targetID;
    KeepawayTarget   target;
    PounceReadyState pounceReadyState;
    float            gameStartTime_s;
    float            creepTime;
    float            pounceChance;
    float            pounceTime;
    float            pounceSuccessPitch_deg;
    float            frustrationExcitementScale;
    bool             isIdling;
    bool             victorGotLastPoint;
    bool             gameOver;
   };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorKeepaway_H_
