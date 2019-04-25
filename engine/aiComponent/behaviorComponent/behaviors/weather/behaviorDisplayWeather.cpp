/**
 * File: BehaviorDisplayWeather.cpp
 *
 * Author: Kevin M. Karol refactored by Sam Russell
 * Created: 2018-04-25 refactor 2019-4-12
 *
 * Description: Displays weather information by compositing temperature information and weather conditions returned from the cloud
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/weather/behaviorDisplayWeather.h"

#include "clad/audio/audioSwitchTypes.h"
#include "clad/types/featureGateTypes.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/localeComponent.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"

#include "coretech/common/engine/utils/timer.h"



#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {
const char* kAnimationNameKey   = "animationName";

const Vision::SpritePathMap::AssetID kFahrenheitIndicatorSpriteID =
  Vision::SpritePathMap::GetAssetID("weather_fahrenheit_indicator");
const Vision::SpritePathMap::AssetID kCelsiusIndicatorSpriteID =
  Vision::SpritePathMap::GetAssetID("weather_celsius_indicator");
const Vision::SpritePathMap::AssetID kNegativeTempIndicatorSpriteID =
   Vision::SpritePathMap::GetAssetID("weather_negative_indicator");

const std::vector<Vision::SpritePathMap::AssetID> kTemperatureAssets = {
  Vision::SpritePathMap::GetAssetID("weather_temp_0"),
  Vision::SpritePathMap::GetAssetID("weather_temp_1"),
  Vision::SpritePathMap::GetAssetID("weather_temp_2"),
  Vision::SpritePathMap::GetAssetID("weather_temp_3"),
  Vision::SpritePathMap::GetAssetID("weather_temp_4"),
  Vision::SpritePathMap::GetAssetID("weather_temp_5"),
  Vision::SpritePathMap::GetAssetID("weather_temp_6"),
  Vision::SpritePathMap::GetAssetID("weather_temp_7"),
  Vision::SpritePathMap::GetAssetID("weather_temp_8"),
  Vision::SpritePathMap::GetAssetID("weather_temp_9"),
};

enum class DigitType : uint8_t {
  NegativeSymbol,
  Hundreds,
  Tens,
  Ones,
  DegreeSymbol
};

const Vision::SpriteBoxName kDigitSpriteBoxes [] {
  Vision::SpriteBoxName::SpriteBox_1,
  Vision::SpriteBoxName::SpriteBox_2,
  Vision::SpriteBoxName::SpriteBox_3,
  Vision::SpriteBoxName::SpriteBox_4,
  Vision::SpriteBoxName::SpriteBox_5
};
using DigitMap = std::unordered_map<DigitType, Vision::SpriteBoxName>;
const DigitMap kSingleDigitMap = {
  {DigitType::NegativeSymbol, kDigitSpriteBoxes[1]},
  {DigitType::Ones, kDigitSpriteBoxes[2]},
  {DigitType::DegreeSymbol, kDigitSpriteBoxes[3]}
};
const DigitMap kDoubleDigitMap = {
  {DigitType::NegativeSymbol, kDigitSpriteBoxes[0]},
  {DigitType::Tens, kDigitSpriteBoxes[1]},
  {DigitType::Ones, kDigitSpriteBoxes[2]},
  {DigitType::DegreeSymbol, kDigitSpriteBoxes[3]}
};
const DigitMap kTripleDigitMap = {
  {DigitType::NegativeSymbol, kDigitSpriteBoxes[0]},
  {DigitType::Hundreds, kDigitSpriteBoxes[1]},
  {DigitType::Tens, kDigitSpriteBoxes[2]},
  {DigitType::Ones, kDigitSpriteBoxes[3]},
  {DigitType::DegreeSymbol, kDigitSpriteBoxes[4]}
};

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::DynamicVariables::DynamicVariables()
{
  currentIntent = nullptr;
  utteranceID = kInvalidUtteranceID;
  utteranceState = UtteranceState::Invalid;
  playingWeatherResponse = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::BehaviorDisplayWeather(const Json::Value& config)
: ICozmoBehavior(config)
{
  AddWaitForUserIntent(USER_INTENT(weather_response));

  _iConfig.animationName = JsonTools::ParseString(config, kAnimationNameKey, "BehaviorDisplayWeather.Constructor.MissingAnimName");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::~BehaviorDisplayWeather()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWeather::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAnimationNameKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.lookAtFaceInFront.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::InitBehavior()
{
  const auto& dac = GetBEI().GetDataAccessorComponent();
  _iConfig.intentParser = std::make_unique<WeatherIntentParser>(
    dac.GetWeatherResponseMap(),
    dac.GetWeatherRemaps()
  );

  auto& behaviorContainer = GetBEI().GetBehaviorContainer();
  _iConfig.lookAtFaceInFront   = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(SingletonFindFaceInFrontWallTime));

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  auto& uic = GetBehaviorComp<UserIntentComponent>();

  _dVars.currentIntent = uic.GetUserIntentIfActive(USER_INTENT(weather_response));
  DEV_ASSERT(_dVars.currentIntent != nullptr, "BehaviorDisplayWeather.InvalidTriggeringIntent");

  const auto& weatherResponse = _dVars.currentIntent->intent.Get_weather_response();
  _iConfig.intentParser->SendDASEventForResponse(weatherResponse);

  StartTTSGeneration();
  TransitionToFindFaceInFront();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::BehaviorUpdate()
{
  if(!IsActivated() || _dVars.playingWeatherResponse){
    return;
  }

  if ( _dVars.utteranceState == UtteranceState::Ready
    || _dVars.utteranceState == UtteranceState::Invalid){
    CancelDelegates(false);
    TransitionToDisplayWeatherResponse();
    _dVars.playingWeatherResponse = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::OnBehaviorDeactivated()
{
  if( _dVars.utteranceID != kInvalidUtteranceID ) {
    GetBEI().GetTextToSpeechCoordinator().CancelUtterance(_dVars.utteranceID);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::TransitionToFindFaceInFront()
{
  ANKI_VERIFY(_iConfig.lookAtFaceInFront->WantsToBeActivated(),
              "BehaviorDisplayWeather.TransitionToFindFaceInFront.BehaviorDoesNotWantToBeActivated", "");
  // We should see a face during this behavior if there's one in front of us to center on
  DelegateIfInControl(_iConfig.lookAtFaceInFront.get(), [this](){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::LookAtUserEndearingly));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::TransitionToDisplayWeatherResponse()
{
  const auto& weatherResponse = _dVars.currentIntent->intent.Get_weather_response();

  auto animationCallback = [this](const AnimationComponent::AnimResult res, u32 streamTimeAnimEnded){
    CancelSelf();
  };

  int temperature = 0;
  auto success = _iConfig.intentParser->GetRawTemperature(weatherResponse, temperature);
  const bool isFahrenheit = _iConfig.intentParser->IsFahrenheit(weatherResponse);
  AnimationComponent::RemapMap spriteBoxRemaps;
  success &= GenerateTemperatureRemaps(temperature, isFahrenheit, spriteBoxRemaps);
  if(!success){
    return;
  }

  // Prepare the utterance to be triggered from the upcoming animation
  GetBEI().GetTextToSpeechCoordinator().PlayUtterance(_dVars.utteranceID);

  const bool interruptRunning = true;
  const Result result = GetBEI().GetAnimationComponent().PlayAnimWithSpriteBoxRemaps(_iConfig.animationName,
                                                                                     spriteBoxRemaps,
                                                                                     interruptRunning,
                                                                                     animationCallback);
  // if we fail to play the anim, simply bail
  if(Result::RESULT_FAIL == result){
    CancelSelf();
    return;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWeather::GenerateTemperatureRemaps(int temp,
                                                       bool isFahrenheit,
                                                       AnimationComponent::RemapMap& spriteBoxRemaps) const
{
  // Clear out all the SBs to start
  for(const auto& spriteBox : kDigitSpriteBoxes){
    spriteBoxRemaps[spriteBox] = Vision::SpritePathMap::kEmptySpriteBoxID;
  }

  const DigitMap& digitMap = (ABS(temp) < 10) ? kSingleDigitMap :
                             ( (ABS(temp) >= 10) && (ABS(temp) < 100) ) ? kDoubleDigitMap :
                             kTripleDigitMap;

  if(temp < 0){
    spriteBoxRemaps[digitMap.at(DigitType::NegativeSymbol)] = kNegativeTempIndicatorSpriteID;
  }

  spriteBoxRemaps[digitMap.at(DigitType::Ones)] = kTemperatureAssets[ABS(temp) % 10];

  if( (kDoubleDigitMap == digitMap) || (kTripleDigitMap == digitMap) ){
    spriteBoxRemaps[digitMap.at(DigitType::Tens)] = kTemperatureAssets[ (ABS(temp) / 10) % 10 ];
  }

  if(kTripleDigitMap == digitMap){
    spriteBoxRemaps[digitMap.at(DigitType::Hundreds)] = kTemperatureAssets[ (ABS(temp) / 100) % 10 ];
  }

  spriteBoxRemaps[digitMap.at(DigitType::DegreeSymbol)] = isFahrenheit ?
                                                          kFahrenheitIndicatorSpriteID :
                                                          kCelsiusIndicatorSpriteID;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::StartTTSGeneration()
{
  _dVars.utteranceID = kInvalidUtteranceID;
  _dVars.utteranceState = UtteranceState::Generating;

  auto callback = [this](const UtteranceState& utteranceState)
  {
    _dVars.utteranceState = utteranceState;
  };

  const auto& weatherResponse = _dVars.currentIntent->intent.Get_weather_response();
  const auto condition = _iConfig.intentParser->GetCondition(weatherResponse);

  int temperature = 0;
  const auto success = _iConfig.intentParser->GetRawTemperature(weatherResponse, temperature);
  const auto & bei = GetBEI();
  const auto & ttsMap = bei.GetDataAccessorComponent().GetWeatherConditionTTSMap();
  const auto ttsIter = ttsMap->find(condition);
  if (success && ttsIter != ttsMap->end()) {

    // Get localized version of "X degrees and cloudy"
    const auto & robotInfo = bei.GetRobotInfo();
    const auto & localeComponent = robotInfo.GetLocaleComponent();
    const auto & ttsString = localeComponent.GetString(ttsIter->second, std::to_string(temperature));

    // Generate TTS utterance for localized string using KeyFrame type trigger so that it plays
    // at a time determined from within the animation
    auto & ttsCoordinator = bei.GetTextToSpeechCoordinator();
    const UtteranceTriggerType triggerType = UtteranceTriggerType::KeyFrame;
    const AudioTtsProcessingStyle style = AudioTtsProcessingStyle::Default_Processed;
    _dVars.utteranceID = ttsCoordinator.CreateUtterance(ttsString, triggerType, style, callback);
  }

  if (kInvalidUtteranceID == _dVars.utteranceID) {
    _dVars.utteranceState = UtteranceState::Invalid;
  }
}

}
}
