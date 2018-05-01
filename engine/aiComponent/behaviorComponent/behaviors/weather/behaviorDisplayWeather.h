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

namespace Anki {

// forward declaration
namespace Vision {
class CompositeImage;
}
  
namespace Cozmo {

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
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& layoutConfig,
                   const Json::Value& mapConfig);
    const Json::Value& compLayoutConfig;
    const Json::Value& compMapConfig;
    std::unique_ptr<Vision::CompositeImage> compImg;
    std::string animationName;

    std::vector<Vision::SpriteName> temperatureAssets;
    // layouts stored least -> greatest pos followed by least -> greatest neg
    std::vector<Vision::CompositeImage> temperatureLayouts;
  };

  struct DynamicVariables {
    DynamicVariables();
  };

  std::unique_ptr<InstanceConfig> _iConfig;
  DynamicVariables _dVars;

  bool GenerateTemperatureLayer(int temp, bool isFahrenheit, Vision::CompositeImage*& outImg);


  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorDisplayWeather__
