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
#include "engine/engineTimeStamp.h"

namespace Anki {
namespace Vector {

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
  enum class BehaviorStage{
    SearchingForCube,
    ApproachingCube,
    WaitingForGameToStart,
    GameHasStarted
  };
  
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
    ICozmoBehaviorPtr searchBehavior;
    TimeStamp_t maxLengthSearchForCube_ms = 0;
  };

  struct DynamicVariables {
    DynamicVariables();

    // Find cube variables
    EngineTimeStamp_t timeSearchForCubeShouldEnd_ms = 0;
    // game variables
    ObjectID objID;
    CubeSpinnerGame::LockResult lastLockResult = CubeSpinnerGame::LockResult::Count;
    int roundsLeftToPlay = 0;
    BehaviorStage stage = BehaviorStage::SearchingForCube;
    bool isCubeSpinnerGameReady = false;
    // player decision tracker
    EngineTimeStamp_t timeOfLastTap = 0;
    bool wasLastCycleTarget = false;
    int lightIdxToLock = CubeLightAnimation::kNumCubeLEDs;
    AnimationTrigger nextResponseAnimation = AnimationTrigger::Count;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void ResetGame();
  void MakeTapDecision(const CubeSpinnerGame::GameSnapshot& snapshot);
  void TapIfAppropriate(const CubeSpinnerGame::GameSnapshot& snapshot);

  bool IsCubePositionKnown() const;
  bool IsInPositionToTap() const;
  
  // Returns the objectID for the game if its set, otherwise guesses what object we'll connect to
  // Reutrns true if bestGuessID was set
  bool GetBestGuessObjectID(ObjectID& bestGuessID) const;

  void TransitionToFindCubeAndApproachCube();
  void TransitionToSearchForCube();
  void TransitionToNextGame();
  uint8_t SelectIndexToLock(const CubeSpinnerGame::LightsLocked& lights, bool shouldSelectLockedIdx);
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorVectorPlaysCubeSpinner__
