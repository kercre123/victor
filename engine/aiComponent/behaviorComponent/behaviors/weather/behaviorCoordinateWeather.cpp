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
#include "engine/aiComponent/behaviorComponent/behaviors/weather/behaviorDisplayWeather.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/featureGateTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/utils/cozmoFeatureGate.h"


namespace Anki {
namespace Vector {
  
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
  const bool intentPending = uic.IsUserIntentPending(USER_INTENT(weather_response));
  
  const bool hasAlexaWeather = _dVars.alexaWeather != nullptr;
  return intentPending || hasAlexaWeather;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
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

  _iConfig.intentParser = std::make_unique<WeatherIntentParser>(
    GetBEI().GetDataAccessorComponent().GetWeatherResponseMap(),
    GetBEI().GetDataAccessorComponent().GetWeatherRemaps()
  );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorCoordinateWeather::OnBehaviorActivated() 
{
  

  auto& uic = GetBehaviorComp<UserIntentComponent>();
  UserIntent intent;
  const bool isPending = uic.IsUserIntentPending(USER_INTENT(weather_response), intent);
  const bool hasAlexa = _dVars.alexaWeather != nullptr;
  UserIntent_WeatherResponse weatherResponse;
  if( isPending ) {
    weatherResponse = intent.Get_weather_response();
  } else if(hasAlexa) {
    // build intent from msg
    weatherResponse.speakableLocationString = "something not empty";
    weatherResponse.isForecast = false;
    weatherResponse.condition = _dVars.alexaWeather->condition;
    weatherResponse.temperature = std::to_string(_dVars.alexaWeather->temperature);
    weatherResponse.temperatureUnit = "F";
    // "%Y-%m-%dT%H:%M%S-"
    weatherResponse.localDateTime = "2018-10-08T" + std::to_string(_dVars.alexaWeather->hour) + ":0000-";
  } else {
    return;
  }
  
  _dVars.alexaWeather.reset();

  // reset dynamic variables
  _dVars = DynamicVariables();
  

  // Respond with appropriate weather response
  bool cantDoThat = false;
  if(_iConfig.intentParser->IsForecast(weatherResponse)){
    cantDoThat = true;
  }else{
    ///////
    /// PR DEMO HACK - want a city where it always rains, so if it's the PRDemo the WeatherIntentParser may lie
    /// about the weather condition based on the location returned by the cloud intent
    ///////
    const auto* featureGate = GetBEI().GetRobotInfo().GetContext()->GetFeatureGate();
    const bool isForPRDemo = (featureGate != nullptr) ? 
                                  featureGate->IsFeatureEnabled(Anki::Vector::FeatureType::PRDemo) :
                                  false;

    const auto condition = _iConfig.intentParser->GetCondition(weatherResponse,
                                                               isForPRDemo);
    ///////
    /// END PR DEMO HACK
    ///////
    PRINT_NAMED_WARNING("WHATNOW", "Looking for behavior for condition %s", WeatherConditionTypeToString(condition));
    const auto iter = _iConfig.weatherBehaviorMap.find(condition);
    
    if(iter != _iConfig.weatherBehaviorMap.end()) {
      auto castPtr = std::dynamic_pointer_cast<BehaviorDisplayWeather>(iter->second);
      if( castPtr != nullptr ) {
        castPtr->SetWeatherResponse(weatherResponse);
        if( iter->second->WantsToBeActivated() ) {
          DelegateIfInControl(iter->second.get());
        } else {
          PRINT_NAMED_WARNING("WHATNOW BehaviorCoordinateWeather.OnBehaviorActivated.DWTBA",
                              "%s Doesnt want to activate", iter->second->GetDebugLabel().c_str());
          cantDoThat = true;
        }
      } else {
        PRINT_NAMED_WARNING("WHATNOW","Couldnt cast");
        cantDoThat = true;
      }
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

void BehaviorCoordinateWeather::SetAlexaWeather( const RobotInterface::AlexaWeather& weather )
{
  _dVars.alexaWeather = std::make_unique<RobotInterface::AlexaWeather>(weather);
}

} // namespace Vector
} // namespace Anki
