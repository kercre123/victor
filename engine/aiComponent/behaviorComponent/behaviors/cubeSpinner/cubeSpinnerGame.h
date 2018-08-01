/**
* File: cubeSpinnerGame.h
*
* Author: Kevin M. Karol
* Created: 2018-06-21
*
* Description: Manages the phases/light overlays for the cube spinner game
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_CubeSpinnerGame__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_CubeSpinnerGame__

#include "clad/types/backpackAnimationTriggers.h"
#include "clad/types/cubeAnimationTrigger.h"
#include "clad/types/ledTypes.h"
#include "coretech/common/shared/types.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/engineTimeStamp.h"
#include "json/json.h"
#include "util/random/randomGenerator.h"

namespace Anki{
namespace Cozmo{

// forward declarations
class BackpackLightComponent;
class BlockWorld;
class CubeLightComponent;

class CubeSpinnerGame
{
public:
  using LightsLocked = std::array<bool,CubeLightAnimation::kNumCubeLEDs>;

  enum class LockResult{
    Locked,   // the light successfully locked into a slot
    Error,    // the light was the wrong color or the slot was full 
    Complete, // all lights on the cube successfully filled
    Count
  };

  // can be passed in to override GameSettings
  struct GameSettingsConfig{
    GameSettingsConfig(){}
    GameSettingsConfig(const Json::Value& settingsConfig);
    uint32_t getInLength_ms = 0;
    uint32_t timePerLED_ms = 0;
    std::array<float,CubeLightAnimation::kNumCubeLEDs> speedMultipliers;
    std::array<float,CubeLightAnimation::kNumCubeLEDs> minWrongColorsPerRound;
    std::array<float,CubeLightAnimation::kNumCubeLEDs> maxWrongColorsPerRound;
  };
  
  struct GameSnapshot{
    bool areLightsCycling = false;
    uint8_t currentLitLEDIdx = 0;
    uint8_t roundNumber = 0;
    bool isCurrentLightTarget = false;
    CubeSpinnerGame::LightsLocked lightsLocked;
    EngineTimeStamp_t timeUntilNextRotation = 0;
  };


  CubeSpinnerGame(const Json::Value& gameConfig,
                  const Json::Value& lightConfigs, 
                  CubeLightComponent& cubeLightComponent,
                  BackpackLightComponent& backpackLightComponent,
                  BlockWorld& blockWorld,
                  Util::RandomGenerator& rng);
  virtual ~CubeSpinnerGame();
  
  using GameReadyCallback = std::function<void(bool gameStartupSuccess, const ObjectID& id)>;

  // Control game phase
  void PrepareForNewGame(GameReadyCallback callback);
  // Will fail if game is not prepared 
  bool StartGame();
  void StopGame();
  // Update must be regularly called every tick once the game starts
  void Update();

  // Call this function to lock the light in response to some input
  void LockNow();

  // Callbacks will be called when certain game events occur
  using GameEventCallback = std::function<void(LockResult result)>;
  void RegisterLightLockedCallback(GameEventCallback callback){
    _lightLockedCallbacks.push_back(callback);
  }

  void SetGameSettings(GameSettingsConfig&& gameSettings){
    _settingsConfig = std::move(gameSettings);
  }

  // Function which allows Vector to "cheat" by gaining insight into what's happening on the cube
  void GetGameSnapshot(GameSnapshot& outSnapshot) const;

private:
  enum class GamePhase{
    GameGetIn,
    CycleColorsUntilTap,
    SuccessfulTap,
    ErrorTap,
    Celebration
  };

  struct LightMapEntry {
    LightMapEntry(const Json::Value& entryConfig);
    std::string debugColorName;
    // backpack lights
    BackpackAnimationTrigger backpackCelebrationTrigger;
    BackpackAnimationTrigger backpackHoldTargetTrigger;
    BackpackAnimationTrigger backpackSelectTargetTrigger;

    // cube lights    
    CubeAnimationTrigger cubeCelebrationTrigger;
    CubeAnimationTrigger cubeCycleTrigger;
    CubeAnimationTrigger cubeLockInTrigger;
    CubeAnimationTrigger cubeLockedPulseTrigger;
    CubeAnimationTrigger cubeLockedTrigger;
  };

  struct GameLightConfig{
    GameLightConfig(const Json::Value& entryConfig);
    CubeAnimationTrigger startGameCubeTrigger;
    CubeAnimationTrigger playerErrorCubeTrigger;
    std::vector<LightMapEntry> lights;
  };

  static const uint32_t kGameHasntStartedTick;
  struct CurrentGame{
    bool hasStarted = false;

    ObjectID targetObject;
    GamePhase gamePhase = GamePhase::GameGetIn;
    // Idxs correspond to light list in GameLightConfig
    uint8_t targetLightIdx = 0;
    uint8_t currentCycleLightIdx = 0;
    EngineTimeStamp_t timeNextAdvanceToLED_ms = 0;
    // tracking locked/rotation light state
    uint8_t lastLEDLockedIdx = 0;
    uint8_t currentCycleLEDIdx = 0;
    LightsLocked lightsLocked;
    EngineTimeStamp_t lastTimeLightLocked_ms = kGameHasntStartedTick;

    uint32_t numberOfCyclesTillNextCorrectLight = 0;
    
    EngineTimeStamp_t lastTimePhaseChanged_ms = kGameHasntStartedTick;
    // Cube light playback
    CubeLightAnimation::LightPattern baseLightPattern;
    CubeLightComponent::AnimationHandle currentCubeHandle = 0;
    bool hasSentLightPattern = false;
    // error checking
    size_t lastUpdateTick = kGameHasntStartedTick;
  };
  
  // Game member variables
  GameSettingsConfig _settingsConfig;
  GameLightConfig _lightsConfig;
  CurrentGame _currentGame;
  std::vector<GameEventCallback> _lightLockedCallbacks;

  // Components used by the game
  CubeLightComponent& _cubeLightComponent;
  BackpackLightComponent& _backpackLightComponent;
  BlockWorld& _blockWorld;
  Util::RandomGenerator& _rng;

  // Checks to ensure we have a connected object
  bool CanGameStart() const;

  bool ResetGame();

  void CheckForGamePhaseTransitions();
  void TransitionToGamePhase(GamePhase phase);
  uint8_t GetNewLightColorIdx(bool forTargetLight = false);
  
  void CheckForNextLEDRotation();
  void ComposeAndSendLights();
  void LockCurrentLightsIn();
  CubeLightAnimation::LightPattern GetCurrentCyclePattern() const;
  CubeLightAnimation::LightPattern GetCurrentLockPattern() const;
  uint8_t GetCurrentCycleIdx() const;
  bool IsCurrentCycleIdxLocked() const;
  bool IsGameOver() const;

  // Use this function to start lights so that tracking variables are set
  void PlayCubeAnimation(CubeLightAnimation::Animation& animToPlay);
  uint32_t MillisecondsBetweenLEDRotations() const;
  uint8_t GetRoundNumber() const;

};

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_CubeSpinnerGame__
 
