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


#include "engine/aiComponent/behaviorComponent/behaviors/weather/behaviorDisplayWeather.h"


#include "clad/audio/audioSwitchTypes.h"
#include "components/textToSpeech/textToSpeechCoordinator.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/behaviorComponent/weatherIntents/weatherIntentParser.h"
#include "engine/components/animationComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/settingsManager.h"
#include "engine/faceWorld.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/proceduralFace/proceduralFace.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"

#include "clad/types/behaviorComponent/userIntent.h"
#include "clad/types/featureGateTypes.h"


namespace Anki {
namespace Vector {

namespace{
const char* kImageLayoutListKey = "imageLayouts";
const char* kImageMapListKey    = "imageMaps";
const char* kAnimationNameKey   = "animationName";

// staticly defined for now - can be moved into JSON easily if
// we need to support different asset designs
const std::vector<Vision::SpriteName> kTemperatureAssets = {
  Vision::SpriteName::Weather_Temp_0,
  Vision::SpriteName::Weather_Temp_1,
  Vision::SpriteName::Weather_Temp_2,
  Vision::SpriteName::Weather_Temp_3,
  Vision::SpriteName::Weather_Temp_4,
  Vision::SpriteName::Weather_Temp_5,
  Vision::SpriteName::Weather_Temp_6,
  Vision::SpriteName::Weather_Temp_7,
  Vision::SpriteName::Weather_Temp_8,
  Vision::SpriteName::Weather_Temp_9,
};
// Positive temperature layouts
const std::vector<Vision::CompositeImageLayout> kPosTemperatureLayouts  = {
  Vision::CompositeImageLayout::TemperatureSingleDig,
  Vision::CompositeImageLayout::TemperatureDoubleDig,
  Vision::CompositeImageLayout::TemperatureTripleDig
};
// Negative temperature layouts
const std::vector<Vision::CompositeImageLayout> kNegTemperatureLayouts  = {
  Vision::CompositeImageLayout::TemperatureNegSingleDig,
  Vision::CompositeImageLayout::TemperatureNegDoubleDig,
  Vision::CompositeImageLayout::TemperatureNegTripleDig
};

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::InstanceConfig::InstanceConfig(const Json::Value& layoutConfig,
                                                       const Json::Value& mapConfig)
: compLayoutConfig(layoutConfig)
, compMapConfig(mapConfig)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::DynamicVariables::DynamicVariables()
{
  temperatureImg = nullptr;
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

  if(config.isMember(kImageLayoutListKey) && config.isMember(kImageMapListKey)){
    _iConfig =  std::make_unique<InstanceConfig>(config[kImageLayoutListKey], config[kImageMapListKey]);
    _iConfig->animationName = JsonTools::ParseString(config, kAnimationNameKey, "BehaviorDisplayWeather.Constructor.MissingAnimName");
    _iConfig->temperatureAssets = kTemperatureAssets;
  }else{
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.Constructor.MissingConfigKeys",
                      "Behavior %s does not have all layout keys defined",
                      GetDebugLabel().c_str());
  }
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
    kImageLayoutListKey,
    kImageMapListKey,
    kAnimationNameKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig->lookAtFaceInFront.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::InitBehavior()
{
  if(_iConfig == nullptr){
    return;
  }
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();

  // Initialize composite image
  {
    auto* spriteCache = dataAccessorComp.GetSpriteCache();
    Vision::HSImageHandle faceHueAndSaturation = ProceduralFace::GetHueSatWrapper();
    _iConfig->compImg = std::make_unique<Vision::CompositeImage>(spriteCache, faceHueAndSaturation,
                                                                 FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT);
  }

  auto& compImgMap = *dataAccessorComp.GetCompImgMap();
  auto& compLayoutMap = *dataAccessorComp.GetCompLayoutMap();

  // Add the temperature layouts to iConfig
  for(const auto& name : kPosTemperatureLayouts){
    auto iter = compLayoutMap.find(name);
    if(iter != compLayoutMap.end()){
      _iConfig->temperatureLayouts.emplace_back(iter->second);
    }
  }

  for(const auto& name : kNegTemperatureLayouts){
    auto iter = compLayoutMap.find(name);
    if(iter != compLayoutMap.end()){
      _iConfig->temperatureLayouts.emplace_back(iter->second);
    }
  }

  // Add data defined layouts to the image
  for(const auto& layoutName : _iConfig->compLayoutConfig){
    auto layoutEnum = Vision::CompositeImageLayoutFromString(layoutName.asString());
    const auto iter = compLayoutMap.find(layoutEnum);
    if(iter != compLayoutMap.end()){
      _iConfig->compImg->MergeInImage(iter->second);
    }else{
      PRINT_NAMED_WARNING("BehaviorDisplayWeather.InitBehavior.InvalidLayout",
                          "Layout %s not found in compLayoutMap",
                          layoutName.asString().c_str());
    }
  }

  // Add the image map assets to the composite image
  for(const auto& mapName : _iConfig->compMapConfig){
    auto mapEnum = Vision::CompositeImageMapFromString(mapName.asString());
    const auto iter = compImgMap.find(mapEnum);
    if(iter != compImgMap.end()){
      const Vision::CompositeImage::LayerImageMap& layerMap = iter->second;
      for(const auto& layerPair : layerMap){
        // Get the layer from the composite image
        const auto& layerName = layerPair.first;
        Vision::CompositeImageLayer* layer = _iConfig->compImg->GetLayerByName(layerName);
        if(layer == nullptr){
          PRINT_NAMED_WARNING("BehaviorDisplayWeather.InitBehavior.LayerNotFound",
                              "Image map has sprite boxes for layer %s which is not present in comp img for behavior %s",
                              Vision::LayerNameToString(layerName),
                              GetDebugLabel().c_str());
          continue;
        }
        // add sprite boxes to the layer
        for(const auto& sbPair : layerPair.second){
          layer->AddToImageMap(sbPair.first, sbPair.second);
        }
      }
    }else{
      PRINT_NAMED_WARNING("BehaviorDisplayWeather.InitBehavior.InvalidImageMap",
                          "Map %s not found in compImgMap",
                          mapName.asString().c_str());
    }
  }

  // Add an empty layer that will be filled in by procedural eyes extracted from the animation
  auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();
  _iConfig->compImg->AddEmptyLayer(seqContainer, Vision::LayerName::Procedural_Eyes);

  // Get the animation ptr
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if((animContainer != nullptr) && !_iConfig->animationName.empty()){
    _iConfig->animationPtr = animContainer->GetAnimation(_iConfig->animationName);
  }

  if(_iConfig->animationPtr == nullptr){
    PRINT_NAMED_WARNING("BehaviorDisplayWeather.InitBehavior.AnimationNotFoundInContainer",
                        "Animations need to be manually loaded on engine side - %s is not", _iConfig->animationName.c_str());
    return;
  }

  _iConfig->intentParser = std::make_unique<WeatherIntentParser>(
    GetBEI().GetDataAccessorComponent().GetWeatherResponseMap(),
    GetBEI().GetDataAccessorComponent().GetWeatherRemaps()
  );

  ParseDisplayTempTimesFromAnim();

  auto& behaviorContainer = GetBEI().GetBehaviorContainer();
  _iConfig->lookAtFaceInFront   = behaviorContainer.FindBehaviorByID(BEHAVIOR_ID(SingletonFindFaceInFrontWallTime));

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
  _iConfig->intentParser->SendDASEventForResponse(weatherResponse);

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
  ANKI_VERIFY(_iConfig->lookAtFaceInFront->WantsToBeActivated(),
              "BehaviorWallTimeCoordinator.TransitionToShowWallTime.BehaviorDoesntWantToBeActivated", "");
  // We should see a face during this behavior if there's one in front of us to center on
  DelegateIfInControl(_iConfig->lookAtFaceInFront.get(), [this](){
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

  int outAnimationDuration = 0;
  const bool shouldInterrupt = true;
  const bool emptySpriteBoxesAllowed = false;
  const Result result = GetBEI().GetAnimationComponent().PlayCompositeAnimation(_iConfig->animationName,
                                                                                *(_iConfig->compImg.get()),
                                                                                ANIM_TIME_STEP_MS,
                                                                                outAnimationDuration,
                                                                                shouldInterrupt,
                                                                                emptySpriteBoxesAllowed,
                                                                                animationCallback);

  // if we fail to play the anim, simply bail
  if(Result::RESULT_FAIL == result){
    CancelSelf();
    return;
  }

  int temperature = 0;
  auto success = _iConfig->intentParser->GetRawTemperature(weatherResponse, temperature);
  const bool isFahrenheit = _iConfig->intentParser->IsFahrenheit(weatherResponse);
  success &= GenerateTemperatureImage(temperature, isFahrenheit, _dVars.temperatureImg);
  if(!success){
    return;
  }

  #if ANKI_DEV_CHEATS
  // Lookup the temperature display times in the animation in case the animation has been updated
  // since this behavior was initialized (which will probably only happen during development if an
  // animator adjusts those times in the animation)
  ParseDisplayTempTimesFromAnim();
  #endif

  GetBEI().GetAnimationComponent().UpdateCompositeImage(*_dVars.temperatureImg, _iConfig->timeTempShouldAppear_ms);
  GetBEI().GetAnimationComponent().ClearCompositeImageLayer(Vision::LayerName::Weather_Temperature,
                                                            _iConfig->timeTempShouldDisappear_ms);

  // Play TTS
  if ((kInvalidUtteranceID != _dVars.utteranceID) && (_dVars.utteranceState == UtteranceState::Ready)){
    RobotInterface::TextToSpeechPlay ttsPlay;
    ttsPlay.ttsID = _dVars.utteranceID;
    RobotInterface::EngineToRobot wrapper(std::move(ttsPlay));
    GetBEI().GetAnimationComponent().AlterStreamingAnimationAtTime(std::move(wrapper), _iConfig->timeTempShouldAppear_ms);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWeather::GenerateTemperatureImage(int temp, bool isFahrenheit, Vision::CompositeImage*& outImg) const
{
  if(_iConfig->temperatureLayouts.size() != 6){
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.GenerateTemperatureImage.TemperatureLayoutsNotFound",
                      "Temperature layouts has %zu elements, expected 6",
                      _iConfig->temperatureLayouts.size());
    return false;
  }

  ///////
  /// PR DEMO HACK - Cloud should be source of truth for fahrenheit/celsius, but check
  /// settings here for PR demo since cloud is still implementing their end
  ///////
  const auto* featureGate = GetBEI().GetRobotInfo().GetContext()->GetFeatureGate();
  const bool featureEnabled = (featureGate != nullptr) ?
                                featureGate->IsFeatureEnabled(Anki::Vector::FeatureType::PRDemo) :
                                false;
  if(featureEnabled){
    auto& settings = GetBEI().GetSettingsManager();
    const bool shouldBeFahrenheit = settings.GetRobotSettingAsBool(external_interface::RobotSetting::temp_is_fahrenheit);
    if(!shouldBeFahrenheit && isFahrenheit){
      temp = static_cast<int>((temp - 32) * (5.0/9.0));
      isFahrenheit = false;
    }
  }
  ///////
  /// END PR DEMO HACK
  ///////


  // Grab the image from temperature layouts - indexed least -> greatest pos followed by least -> greatest neg
  if(temp > 0){
    if(temp < 10){
      outImg = &_iConfig->temperatureLayouts[0];
    }else if(temp < 100){
      outImg = &_iConfig->temperatureLayouts[1];
    }else{
      outImg = &_iConfig->temperatureLayouts[2];
    }
  }else{
    if(temp > -10){
      outImg = &_iConfig->temperatureLayouts[3];
    }else if(temp > -100){
      outImg = &_iConfig->temperatureLayouts[4];
    }else{
      outImg = &_iConfig->temperatureLayouts[5];
    }
  }
  if(!ANKI_VERIFY(outImg->GetLayerLayoutMap().size() == 1,
                  "BehaviorDisplayWeather.GenerateTemperatureImage.ImproperNumberOfLayers",
                  "Expected one layer, but image has %zu",
                  outImg->GetLayerLayoutMap().size())){
    return false;
  }

  auto& layer = outImg->GetLayerLayoutMap().begin()->second;

  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();

  auto* spriteCache = dataAccessorComp.GetSpriteCache();
  auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();

  // Add sprite boxes as appropriate to the layer
  {
    const auto& tempIndicator = isFahrenheit ? Vision::SpriteName::Weather_Fahrenheit_Indicator : Vision::SpriteName::Weather_Celsius_Indicator;
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureDegreeIndicator,
                        tempIndicator);
  }
  if(temp < 0){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureNegativeIndicator,
                        Vision::SpriteName::Weather_Negative_Indicator);
  }

  const auto absTemp = std::abs(temp);
  const auto onesDig = absTemp % 10;
  const auto tensDig = (absTemp / 10) % 10;
  const auto hundredsDig = (absTemp / 100) % 10;
  {
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureOnesDigit,
                        _iConfig->temperatureAssets[onesDig]);
  }
  // Don't show leading zeroes
  if((tensDig > 0) || (hundredsDig > 0)){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureTensDigit,
                        _iConfig->temperatureAssets[tensDig]);
  }
  if(hundredsDig > 0){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureHundredsDigit,
                        _iConfig->temperatureAssets[hundredsDig]);
  }

  ApplyModifiersToTemperatureDisplay(layer, temp);
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::ApplyModifiersToTemperatureDisplay(Vision::CompositeImageLayer& layer, int temperature) const
{
  const auto absTemp = std::abs(temperature);
  const auto onesDigit = absTemp % 10;
  const auto tensDigit = (absTemp / 10) % 10;
  const auto hundredsDigit = (absTemp / 100) % 10;

  // Setup new image layout modifier to deal with the fact that 1s are half width images
  Vision::CompositeImageLayer::LayoutMap& layoutMap = layer.GetLayoutMap();
  auto onesDigitIter = layoutMap.find(Vision::SpriteBoxName::TemperatureOnesDigit);
  if(onesDigitIter == layoutMap.end()){
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.GenerateTemperatureImage.NoOnesDigit",
                      "Cant calculate layout modifier without a width for the sprite box");
    return;
  }

  Point2i outPoint;
  int outWidth;
  int outHeight;

  // Since ones are half width we want to move digits towards the center if they're present
  // Since we're moving from both sides, the width each side will move towards the center is 1/4 the total width
  onesDigitIter->second.GetPositionForFrame(0, outPoint, outWidth, outHeight);
  const auto widthToMovePerOne = outWidth/4;

  int numberOfOnesToLeft = 0;
  int numberOfOnesToRight = (onesDigit == 1 ? 1 : 0) +
                            (tensDigit == 1 ? 1 : 0) +
                            (hundredsDigit == 1 ? 1 : 0);

  auto modifyLayoutFunc = [&layoutMap, &numberOfOnesToLeft, &numberOfOnesToRight, widthToMovePerOne]
                             (Vision::SpriteBoxName sbName, bool addModifier = true){
    auto iter = layoutMap.find(sbName);
    if(iter != layoutMap.end()){
      // clear out any previous modifier so that we get the proper outPoint below
      Vision::CompositeImageLayoutModifier* modifier = nullptr;
      iter->second.SetLayoutModifier(modifier);

      if(addModifier){
        modifier = new Vision::CompositeImageLayoutModifier();

        Point2i outPoint;
        int outWidth;
        int outHeight;

        iter->second.GetPositionForFrame(0, outPoint, outWidth, outHeight);
        outPoint[0] += (widthToMovePerOne * numberOfOnesToRight);
        outPoint[0] -= (widthToMovePerOne * numberOfOnesToLeft);

        modifier->UpdatePositionForFrame(0, outPoint);
        iter->second.SetLayoutModifier(modifier);
      }
    }
  };


  // Negative indicator
  {
    modifyLayoutFunc(Vision::SpriteBoxName::TemperatureNegativeIndicator, temperature < 0);
  }

  if(hundredsDigit == 1){
    numberOfOnesToLeft++;
    numberOfOnesToRight--;
  }

  // hundreds digit
  {
    modifyLayoutFunc(Vision::SpriteBoxName::TemperatureHundredsDigit);
  }

  if(tensDigit == 1){
    numberOfOnesToLeft++;
    numberOfOnesToRight--;
  }

  // tens digit
  {
    modifyLayoutFunc(Vision::SpriteBoxName::TemperatureTensDigit);
  }

  if(onesDigit == 1){
    numberOfOnesToLeft++;
    numberOfOnesToRight--;
  }

  // ones digit
  {
    modifyLayoutFunc(Vision::SpriteBoxName::TemperatureOnesDigit);
  }

  // unit indicator
  {
    modifyLayoutFunc(Vision::SpriteBoxName::TemperatureDegreeIndicator);
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::ParseDisplayTempTimesFromAnim()
{
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();

  const Animation* anim = nullptr;
  bool gotAnim = false;

  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if(animContainer != nullptr){
    anim = animContainer->GetAnimation(_iConfig->animationName);
    gotAnim = (anim != nullptr);
  }

  if(!gotAnim){
    PRINT_NAMED_WARNING("BehaviorDisplayWeather.ParseDisplayTempTimesFromAnim.AnimationNotFoundInContainer",
                        "Animations need to be manually loaded on engine side - %s is not", _iConfig->animationName.c_str());
    return;
  }

  const auto& track = anim->GetTrack<EventKeyFrame>();
  if(track.TrackLength() == 2){
    _iConfig->timeTempShouldAppear_ms = track.GetFirstKeyFrame()->GetTriggerTime_ms();
    _iConfig->timeTempShouldDisappear_ms = track.GetLastKeyFrame()->GetTriggerTime_ms();
    PRINT_CH_DEBUG("Behaviors",
                   "BehaviorDisplayWeather.ParseDisplayTempTimesFromAnim.TemperatureTimes",
                   "For animation named %s temp will appear at %d and disappear at %d",
                   _iConfig->animationName.c_str(),
                   _iConfig->timeTempShouldAppear_ms,
                   _iConfig->timeTempShouldDisappear_ms);
  }else{
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.ParseDisplayTempTimesFromAnim.IncorrectNumberOfKeyframes",
                      "Expected 2 keyframes in event track, but track has %d",
                      track.TrackLength());
  }
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
  const auto condition = _iConfig->intentParser->GetCondition(weatherResponse);

  int temperature = 0;
  auto success = _iConfig->intentParser->GetRawTemperature(weatherResponse, temperature);
  const auto& ttsMap = GetBEI().GetDataAccessorComponent().GetWeatherConditionTTSMap();
  const auto& ttsIter = ttsMap->find(condition);
  if(success && ttsIter != ttsMap->end()){
      std::string conditionText = std::to_string(temperature)  + " degrees " + ttsIter->second;
      const UtteranceTriggerType triggerType = UtteranceTriggerType::Manual;
      const AudioTtsProcessingStyle style = AudioTtsProcessingStyle::Default_Processed;

      _dVars.utteranceID = GetBEI().GetTextToSpeechCoordinator().CreateUtterance(conditionText, triggerType, style,
                                                                                 1.0f, callback);
  }

  if(kInvalidUtteranceID == _dVars.utteranceID){
    _dVars.utteranceState = UtteranceState::Invalid;
  }
}


}
}
