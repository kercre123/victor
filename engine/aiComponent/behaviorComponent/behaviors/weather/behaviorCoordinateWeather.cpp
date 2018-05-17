/**
 * File: BehaviorDisplayWeather.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2018-04-25
 *
 * Description: Displays weather information by compositing temperature information and weather conditions returned from the cloud
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/weather/behaviorCoordinateWeather.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/weatherIntentParser.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kResponseMapKey =  "responseMap";
const char* kCladTypeKey    =  "CladType";
const char* kBehaviorIDKey  =  "BehaviorID";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWeather::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWeather::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWeather::BehaviorCoordinateWeather(const Json::Value& config)
: ICozmoBehavior(config)
{
  const bool isMember = config[kResponseMapKey].isArray();
  if(ANKI_VERIFY(isMember, 
                 "BehaviorCoordinateWeather.Constructor.NoBehaviorsSpecified",
                 "")){
    for(const auto& entry : config[kResponseMapKey]){
      const auto behaviorIDStr = BehaviorTypesWrapper::BehaviorIDFromString(entry[kBehaviorIDKey].asString());
      _iConfig.behaviorIDs.push_back(behaviorIDStr);
      _iConfig.conditions.push_back(WeatherConditionTypeFromString(entry[kCladTypeKey].asString()));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorCoordinateWeather::~BehaviorCoordinateWeather()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for(const auto& pair: _iConfig.weatherBehaviorMap){
    delegates.insert(pair.second.get());
  }
  delegates.insert(_iConfig.iCantDoThatBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const
{
  for(const auto& pair: _iConfig.weatherBehaviorMap){
    delegates.insert(pair.second.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorCoordinateWeather::WantsToBeActivatedBehavior() const
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  return uic.IsUserIntentPending(USER_INTENT(weather_response));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kResponseMapKey,
    kCladTypeKey,
    kBehaviorIDKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::InitBehavior()
{
  auto& behaviorContainer = GetBEI().GetBehaviorContainer();
  for(auto i = 0; i < _iConfig.behaviorIDs.size(); i++){
    const auto& behaviorPtr = behaviorContainer.FindBehaviorByID(_iConfig.behaviorIDs[i]);

    if(behaviorPtr != nullptr){
      _iConfig.weatherBehaviorMap.emplace(_iConfig.conditions[i], behaviorPtr);
    }else{
      PRINT_NAMED_ERROR("BehaviorCoordinateWeather.InitBehavior.MissingBehavior", 
                        "BehaviorID %s not found in container - no behavior will be added for condition %s",
                        EnumToString(_iConfig.behaviorIDs[i]), EnumToString(_iConfig.conditions[i]));
    }
  }
  _iConfig.behaviorIDs.clear();

  _iConfig.iCantDoThatBehavior = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(SingletonICantDoThat));

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntent intent;
  const bool isPending = uic.IsUserIntentPending(USER_INTENT(weather_response), intent);
  if(!ANKI_VERIFY(isPending, 
                  "BehaviorCoordinateWeather.OnBehaviorActivated.NoIntentPending",
                  "")){
    return;
  }

  const auto& weatherResponse = intent.Get_weather_response();

  // Respond with appropriate weather response
  bool cantDoThat = false;
  if(WeatherIntentParser::IsForecast(weatherResponse)){
    cantDoThat = true;
  }else{
    const auto condition = WeatherIntentParser::GetCondition(weatherResponse);
    const auto iter = _iConfig.weatherBehaviorMap.find(condition);
    if((iter != _iConfig.weatherBehaviorMap.end()) &&
       iter->second->WantsToBeActivated()){
      DelegateIfInControl(iter->second.get());
    }else{
      cantDoThat = true;
      PRINT_NAMED_WARNING("BehaviorCoordinateWeather.OnBehaviorActivated.NoResponseForCondition",
                          "No behavior for condition %s, play ICantDoThat for the time being",
                          weatherResponse.condition.c_str());
    }
  }

  // For some reason the robot cant
  if(cantDoThat){
    if(_iConfig.iCantDoThatBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.iCantDoThatBehavior.get());
      SmartActivateUserIntent(intent.GetTag());
    }else{
      PRINT_NAMED_ERROR("BehaviorCoordinateWeather.OnBehaviorActivated.ICantDoThatCantBeActivated",
                        "");
    }
  }

  // Intent should have been cleared above by cant do that or a weather behavior
  // If we have to clear it here something went wrong
  if(uic.IsUserIntentPending(USER_INTENT(weather_response))){
    PRINT_NAMED_ERROR("BehaviorCoordinateWeather.OnBehaviorActivated.UserIntentConditionNotCleared",
                      "Something went wrong for condition %s, clearing intent",
                      weatherResponse.condition.c_str());
    SmartActivateUserIntent(intent.GetTag());
  }

}

} // namespace Cozmo
} // namespace Anki
