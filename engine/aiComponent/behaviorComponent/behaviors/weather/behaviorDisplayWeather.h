/**
 * File: BehaviorDisplayWeather.h
 *
 * Author: Kevin M. Karol refactored by Sam Russell
 * Created: 2018-04-25 refactor 2019-4-12
 *
 * Description: Displays weather information by compositing temperature information and weather conditions returned from the cloud
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWeather__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWeather__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/animationComponent.h"
#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"

namespace Anki {

// forward declaration
namespace Vision {
class CompositeImage;
}

namespace Vector {

// Fwd Declarations
enum class UtteranceState;

class BehaviorDisplayWeather : public ICozmoBehavior
{
public:
  virtual ~BehaviorDisplayWeather();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorDisplayWeather(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;


  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

private:
  using AudioTtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;

  struct InstanceConfig {
    InstanceConfig() {}

    // Animation metadata
    std::string animationName;
    u32 timeTTSShouldStart_ms = 0;

    std::unique_ptr<WeatherIntentParser> intentParser;
    ICozmoBehaviorPtr                    lookAtFaceInFront;
  };

  struct DynamicVariables {
    DynamicVariables();
    UserIntentPtr           currentIntent;
    uint8_t utteranceID;
    UtteranceState  utteranceState;
    bool playingWeatherResponse;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  void TransitionToDisplayWeatherResponse();
  void TransitionToFindFaceInFront();

  void StartTTSGeneration();
  bool GenerateTemperatureRemaps(int temp, bool isFahrenheit, AnimationComponent::RemapMap& spriteBoxRemaps) const;

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWeather__
