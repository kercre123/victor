/**
 * File: behaviorDevImageCapture.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-12-12
 *
 * Description: Dev behavior to use the touch sensor to enable / disable image capture
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevImageCapture.h"

#include "clad/types/imageTypes.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"

#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const float kLightBlinkPeriod_s = 0.5f;
constexpr const float kHoldTimeForStreaming_s = 1.0f;

static const BackpackLightAnimation::BackpackAnimation kLightsOn = {
  .onColors               = {{NamedColors::BLACK,NamedColors::RED,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::RED,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLightAnimation::BackpackAnimation kLightsOff = {
  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};
  
const char* const kSavePathKey = "save_path";
const char* const kImageSaveQualityKey = "quality";
const char* const kImageScaleKey = "image_scale";
const char* const kImageResizeMethodKey = "resize_method";
const char* const kUseCapacitiveTouchKey = "use_capacitive_touch";
const char* const kSaveSensorDataKey = "save_sensor_data";
const char* const kClassNamesKey = "class_names";

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::InstanceConfig::InstanceConfig()
{
  useCapTouch = false;
  saveSensorData = false;
  imageSaveSize = Vision::ImageCache::Size::Full;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::DynamicVariables::DynamicVariables()
{
  touchStartedTime_s = -1.0f; 

  blinkOn = false;
  timeToBlink = -1.0f;

  isStreaming = false;
  wasLiftUp = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::BehaviorDevImageCapture(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _iConfig.imageSavePath = JsonTools::ParseString(config, kSavePathKey, "BehaviorDevImageCapture");
  _iConfig.imageSaveQuality = JsonTools::ParseInt8(config, kImageSaveQualityKey, "BehaviorDevImageCapture");
  _iConfig.useCapTouch = JsonTools::ParseBool(config, kUseCapacitiveTouchKey, "BehaviorDevImageCapture");

  std::string scaleStr = JsonTools::ParseString(config, kImageScaleKey, "BehaviorDevImageCapture");
  std::string methodStr = JsonTools::ParseString(config, kImageResizeMethodKey, "BehaviorDevImageCapture");
  _iConfig.imageSaveSize = Vision::ImageCache::StringToSize(scaleStr, methodStr);

  if (config.isMember(kSaveSensorDataKey)) {
    _iConfig.saveSensorData = config[kSaveSensorDataKey].asBool();
  }

  if(config.isMember(kClassNamesKey))
  {
    auto const& classNames = config[kClassNamesKey];
    if(classNames.isArray())
    {
      for(Json::ArrayIndex index=0; index < classNames.size(); ++index)
      {
        _iConfig.classNames.push_back(classNames[index].asString());
      }
    }
    else if(classNames.isString())
    {
      _iConfig.classNames.push_back(classNames.asString());
    }
    else 
    {
      PRINT_NAMED_WARNING("BehaviorDevImageCapture.Constructor.InvalidClassNames", "");
    }
  }
  _dVars.currentClassIter = _iConfig.classNames.begin();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::~BehaviorDevImageCapture()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSavePathKey,
    kImageSaveQualityKey,
    kImageScaleKey,
    kImageResizeMethodKey,
    kUseCapacitiveTouchKey,
    kSaveSensorDataKey,
    kClassNamesKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::OnBehaviorActivated()
{
  _dVars.touchStartedTime_s = -1.0f;
  _dVars.isStreaming = false;
  _dVars.timeToBlink = -1.0f;
  _dVars.blinkOn = false;

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetComponent<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(true);
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(true);

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetComponent<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::SwitchToNextClass()
{
  if(!_iConfig.classNames.empty())
  {
    ++_dVars.currentClassIter; 
    if(_dVars.currentClassIter == _iConfig.classNames.end())
    {
      _dVars.currentClassIter = _iConfig.classNames.begin();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevImageCapture::GetSavePath() const
{
  if(_dVars.currentClassIter == _iConfig.classNames.end())
  {
    return _iConfig.imageSavePath;
  }
  
  const std::string& cachePath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->GetCachePath("camera");
  return Util::FileUtils::FullFilePath({cachePath, "images", _iConfig.imageSavePath, *_dVars.currentClassIter});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetComponent<VisionComponent>();

  // update light blinking if needed
  if( _dVars.timeToBlink > 0.0f ) {
    if( currTime_s >= _dVars.timeToBlink ) {
      BlinkLight();
    }
  }

  // Switch classes when lift is lowered
  const bool isLiftUp = (GetBEI().GetRobotInfo().GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK);
  if(_dVars.wasLiftUp && !isLiftUp)
  {
    SwitchToNextClass();
  }
  _dVars.wasLiftUp = isLiftUp;

  if(_dVars.currentClassIter != _iConfig.classNames.end())
  {
    using namespace Util;
    const size_t numFiles = FileUtils::FilesInDirectory(GetSavePath()).size();
    
    std::function<void(Vision::ImageRGB&)> drawClassName = [this,numFiles](Vision::ImageRGB& img)
    {
      img.DrawText({1,14}, *_dVars.currentClassIter + ":" + std::to_string(numFiles), NamedColors::YELLOW, 0.6f, true);
    };
    visionComponent.AddDrawScreenModifier(drawClassName);
  }

  const bool wasTouched = (_dVars.touchStartedTime_s >= 0.0f);

  const bool isTouched = (_iConfig.useCapTouch ? 
                          GetBEI().GetTouchSensorComponent().GetIsPressed() :
                          GetBEI().GetRobotInfo().IsPowerButtonPressed());

  if( wasTouched && !isTouched ) {
    // just "released", see if it's been long enough to count as a "hold"
    ImageSendMode sendMode = ImageSendMode::Off;
    if( currTime_s >= _dVars.touchStartedTime_s + kHoldTimeForStreaming_s ) {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.longPress", "long press release");
        
      // toggle streaming
      _dVars.isStreaming = !_dVars.isStreaming;

      sendMode = (_dVars.isStreaming ? ImageSendMode::Stream : ImageSendMode::Off);
      
      if( _dVars.isStreaming ) {
        BlinkLight();
      }
    }
    else {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.shortPress", "short press release");
      // take single photo

      // shutter sound
      {
        using GE = AudioMetaData::GameEvent::GenericEvent;
        using GO = AudioMetaData::GameObjectType;
        GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Camera_Flash,
                                                 GO::Behavior);
      }

      sendMode = (_iConfig.saveSensorData ? ImageSendMode::SingleShotWithSensorData : ImageSendMode::SingleShot);
     
      BlinkLight();
    }
    
    visionComponent.SetSaveImageParameters(sendMode,
                                           GetSavePath(),
                                           "", // No basename: rely on auto-numbering
                                           _iConfig.imageSaveQuality,
                                           _iConfig.imageSaveSize);
  }
  else if( !wasTouched && isTouched ) {
    PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.newTouch", "new press");
    _dVars.touchStartedTime_s = currTime_s;

    if( _dVars.isStreaming ) {
      // we were streaming but now should stop because there was a new touch
      visionComponent.SetSaveImageParameters(ImageSendMode::Off,
                                             GetSavePath(),
                                             "", // No basename: rely on auto-numbering
                                             _iConfig.imageSaveQuality,
                                             _iConfig.imageSaveSize);

      _dVars.isStreaming = false;
    }
  }

  if( !isTouched ) {
    _dVars.touchStartedTime_s = -1.0f;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::BlinkLight()
{
  _dVars.blinkOn = !_dVars.blinkOn;

  GetBEI().GetBackpackLightComponent().SetBackpackAnimation( _dVars.blinkOn ? kLightsOn : kLightsOff );

  // always blink if we are streaming, and set up another blink for single photo if we need to turn off the
  // light
  if( _dVars.blinkOn || _dVars.isStreaming ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    // need to do another blink after this one
    _dVars.timeToBlink = currTime_s + kLightBlinkPeriod_s;
  }
  else {
    _dVars.timeToBlink = -1.0f;
  }
}

} // namespace Cozmo
} // namespace Anki
