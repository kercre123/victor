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

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWeather__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWeather__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/behaviorComponent/weatherConditionTypes.h"

namespace Anki {
namespace Vector {
  
  namespace RobotInterface {
    struct AlexaWeather;
  }

// forward declaration
enum class BehaviorID : uint16_t;
class WeatherIntentParser;

class BehaviorCoordinateWeather : public ICozmoBehavior
{
public: 
  virtual ~BehaviorCoordinateWeather();

  virtual void GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const override;

  void SetAlexaWeather( const RobotInterface::AlexaWeather& weather );
  
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCoordinateWeather(const Json::Value& config);  

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void InitBehavior() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    // indexes of these two vectors match - combined into the map below
    std::vector<BehaviorID> behaviorIDs;
    std::vector<WeatherConditionType> conditions;

    std::map<WeatherConditionType, ICozmoBehaviorPtr> weatherBehaviorMap;
    std::unique_ptr<WeatherIntentParser> intentParser;

    ICozmoBehaviorPtr              iCantDoThatBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    
    std::unique_ptr<RobotInterface::AlexaWeather> alexaWeather;
    
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;

  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWeather__
