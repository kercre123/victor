/**
 * File: BehaviorVectorPlaysCubeSpinner.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-06-26
 *
 * Description: A Behavior where Vector plays the cube spinner game
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVectorPlaysCubeSpinner__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVectorPlaysCubeSpinner__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/aiComponent/behaviorComponent/behaviors/cubeSpinner/cubeSpinnerGame.h"

namespace Anki {
namespace Cozmo {

class BehaviorVectorPlaysCubeSpinner : public ICozmoBehavior
{
public: 
  virtual ~BehaviorVectorPlaysCubeSpinner();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorVectorPlaysCubeSpinner(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  
private:
  struct VectorPlayerConfig {
    VectorPlayerConfig(){}
    VectorPlayerConfig(const Json::Value& config);
    std::array<float,CubeLightAnimation::kNumCubeLEDs> oddsOfMissingPattern;
    std::array<float,CubeLightAnimation::kNumCubeLEDs> oddsOfTappingAfterCorrectPattern;
    std::array<float,CubeLightAnimation::kNumCubeLEDs> oddsOfTappingCorrectPatternOnLock;
  };

  struct InstanceConfig {
    InstanceConfig();
    std::unique_ptr<CubeSpinnerGame> cubeSpinnerGame;
    Json::Value gameConfig;  
    int minRoundsToPlay = 0;
    int maxRoundsToPlay = 0;
    VectorPlayerConfig playerConfig;
  };

  struct DynamicVariables {
    DynamicVariables();
    ObjectID objID;
    CubeSpinnerGame::LockResult lastLockResult = CubeSpinnerGame::LockResult::Count;
    bool hasGameStarted = false;
    int roundsLeftToPlay = 0;
    
    // player decision tracker
    bool wasLastCycleTarget = false;
    int lightIdxToLock = CubeLightAnimation::kNumCubeLEDs;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void ResetGame();
  void MakeTapDecision();

  bool IsInPositionToTap() const;

  void TransitionToDriveToCube();
  void TransitionToNextGame();
  uint8_t SelectIndexToLock(const CubeSpinnerGame::LightsLocked& lights, bool shouldSelectLockedIdx);
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVectorPlaysCubeSpinner__
