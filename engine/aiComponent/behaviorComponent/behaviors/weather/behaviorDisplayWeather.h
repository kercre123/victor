/**
 * File: BehaviorDisplayWeather.h
 *
 * Author: Kevin M. Karol
 * Created: 2018-04-25
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

namespace Anki {

// forward declaration
namespace Vision {
class CompositeImage;
}
  
namespace Vector {

// Fwd Declarations
class BehaviorTextToSpeechLoop;
class WeatherIntentParser;
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
    InstanceConfig(const Json::Value& layoutConfig,
                   const Json::Value& mapConfig);
    const Json::Value& compLayoutConfig;
    const Json::Value& compMapConfig;
    std::unique_ptr<Vision::CompositeImage> compImg;

    // Animation metadata
    std::string animationName;
    const Animation* animationPtr = nullptr;
    u32 timeTempShouldAppear_ms = 0;
    u32 timeTempShouldDisappear_ms = 0;

    std::vector<Vision::SpriteName> temperatureAssets;
    // layouts stored least -> greatest pos followed by least -> greatest neg
    std::vector<Vision::CompositeImage> temperatureLayouts;
    std::unique_ptr<WeatherIntentParser> intentParser;
    ICozmoBehaviorPtr                    lookAtFaceInFront;
  };

  struct DynamicVariables {
    DynamicVariables();
    Vision::CompositeImage* temperatureImg;
    UserIntentPtr           currentIntent;
    uint8_t utteranceID;
    UtteranceState  utteranceState;
    bool playingWeatherResponse;
  };

  std::unique_ptr<InstanceConfig> _iConfig;
  DynamicVariables _dVars;

  void TransitionToDisplayWeatherResponse();
  void TransitionToFindFaceInFront();

  bool GenerateTemperatureImage(int temp, bool isFahrenheit, Vision::CompositeImage*& outImg) const;
  // To accomodate 1s being half the width of other numerals we need to dynamically modify the temperature layer
  void ApplyModifiersToTemperatureDisplay(Vision::CompositeImageLayer& layer, int temperature) const;
  
  void ParseDisplayTempTimesFromAnim();
  void StartTTSGeneration();

};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWeather__
