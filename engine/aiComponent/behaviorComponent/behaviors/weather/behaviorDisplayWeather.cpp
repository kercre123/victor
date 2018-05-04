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

#include "engine/components/animationComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/proceduralFace/proceduralFace.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const char* kConsoleVarGroup    = "WeatherBehavior";

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
CONSOLE_VAR_RANGED(int, kTestTemperature, kConsoleVarGroup, 0, -150, 150);
CONSOLE_VAR(bool, kTestTempIsFahrenheit, kConsoleVarGroup, true);

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::BehaviorDisplayWeather(const Json::Value& config)
: ICozmoBehavior(config)
{
  if(config.isMember(kImageLayoutListKey) && config.isMember(kImageMapListKey)){
    _iConfig =  std::make_unique<InstanceConfig>(config[kImageLayoutListKey], config[kImageMapListKey]);
    _iConfig->animationName = JsonTools::ParseString(config, kAnimationNameKey, "BehaviorDisplayWeather.Constructor.MissingAnimName");
    _iConfig->temperatureAssets = kTemperatureAssets;
  }else{
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.Constructor.MissingConfigKeys",
                      "Behavior %s does not have all layout keys defined",
                      GetDebugLabel().c_str());
  }
  
  MakeMemberTunable(_iConfig->devIsRunnable, "isRunnable", kConsoleVarGroup);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDisplayWeather::~BehaviorDisplayWeather()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDisplayWeather::WantsToBeActivatedBehavior() const
{
  return _iConfig->devIsRunnable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.behaviorAlwaysDelegates = false;
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
void BehaviorDisplayWeather::InitBehavior()
{
  if(_iConfig == nullptr){
    return;
  }
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetValue<DataAccessorComponent>();

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
                              "Image map has sprite boxes for layer %s which is not present in comp img for behaivor %s",
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
  
  ParseDisplayTempTimesFromAnim();
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  auto animationCallback = [this](const AnimationComponent::AnimResult res){
    CancelSelf();
  };

  int outAnimationDuration = 0;
  const bool shouldInterrupt = true;
  GetBEI().GetAnimationComponent().PlayCompositeAnimation(_iConfig->animationName,
                                                          *(_iConfig->compImg.get()),
                                                          ANIM_TIME_STEP_MS,
                                                          outAnimationDuration,
                                                          shouldInterrupt,
                                                          animationCallback);
  
  const auto temperature = _iConfig->devIsRunnable ? kTestTemperature : 0;
  const auto success = GenerateTemperatureImage(temperature, kTestTempIsFahrenheit, _dVars.temperatureImg);
  if(!success){
    return;
  }
  
  GetBEI().GetAnimationComponent().UpdateCompositeImage(*_dVars.temperatureImg, _iConfig->timeTempShouldAppear_ms);
  GetBEI().GetAnimationComponent().ClearCompositeImageLayer(Vision::LayerName::Weather_Temperature,
                                                            _iConfig->timeTempShouldDisappear_ms);
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

  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetValue<DataAccessorComponent>();

  auto* spriteCache = dataAccessorComp.GetSpriteCache();
  auto* seqContainer = dataAccessorComp.GetSpriteSequenceContainer();

  // Add sprite boxes as appropriate to the layer
  {
    const auto& tempIndicator = isFahrenheit ? Vision::SpriteName::Weather_Temp_Fahr : Vision::SpriteName::Weather_Temp_Cel;
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureDegreeIndicator, 
                        tempIndicator);
  }
  if(temp < 0){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureNegativeIndicator, 
                        Vision::SpriteName::Weather_Temp_Neg);
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
  if(tensDig > 0){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureTensDigit, 
                        _iConfig->temperatureAssets[tensDig]);
  }
  if(hundredsDig > 0){
    layer.AddToImageMap(spriteCache, seqContainer,
                        Vision::SpriteBoxName::TemperatureHundredsDigit, 
                        _iConfig->temperatureAssets[hundredsDig]);
  }

  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDisplayWeather::ParseDisplayTempTimesFromAnim()
{
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetValue<DataAccessorComponent>();

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

  const TimeStamp_t time_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  const auto& track = anim->GetTrack<EventKeyFrame>();
  if(track.TrackLength() == 2){
    // assumes only one keyframe per eating anim
    _iConfig->timeTempShouldAppear_ms =  time_ms + track.GetFirstKeyFrame()->GetTriggerTime();
    _iConfig->timeTempShouldDisappear_ms = time_ms + track.GetLastKeyFrame()->GetTriggerTime();
    PRINT_CH_INFO("Behaviors",
                  "BehaviorDisplayWeather.ParseDisplayTempTimesFromAnim.TemperatureTimes",
                  "For animation named %s temp will appear at %d and dissapear at %d",
                  _iConfig->animationName.c_str(),
                  _iConfig->timeTempShouldAppear_ms,
                  _iConfig->timeTempShouldDisappear_ms);
  }else{
    PRINT_NAMED_ERROR("BehaviorDisplayWeather.ParseDisplayTempTimesFromAnim.IncorrectNumberOfKeyframes",
                      "Expected 2 keyframes in event track, but track has %d",
                      track.TrackLength());
  }
}


}
}
