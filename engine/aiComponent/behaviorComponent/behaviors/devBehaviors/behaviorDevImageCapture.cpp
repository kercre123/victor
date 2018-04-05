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

#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/types/imageTypes.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"

#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr const float kLightBlinkPeriod_s = 0.5f;
constexpr const float kHoldTimeForStreaming_s = 1.0f;

static const BackpackLights kLightsOn = {
  .onColors               = {{NamedColors::BLACK,NamedColors::RED,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::RED,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLights kLightsOff = {
  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0}},
  .offPeriod_ms           = {{0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::BehaviorDevImageCapture(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _imageSavePath = JsonTools::ParseString(config, "save_path", "BehaviorDevImageCapture");
  _imageSaveQuality = JsonTools::ParseInt8(config, "quality", "BehaviorDevImageCapture");
  _useCapTouch = JsonTools::ParseBool(config, "use_capacitive_touch", "BehaviorDevImageCapture");

  if (config.isMember("save_sensor_data")) {
    _saveSensorData = config["save_sensor_data"].asBool();
  }

  if(config.isMember("class_names"))
  {
    auto const& classNames = config["class_names"];
    if(classNames.isArray())
    {
      for(Json::ArrayIndex index=0; index < classNames.size(); ++index)
      {
        _classNames.push_back(classNames[index].asString());
      }
    }
    else if(classNames.isString())
    {
      _classNames.push_back(classNames.asString());
    }
    else 
    {
      PRINT_NAMED_WARNING("BehaviorDevImageCapture.Constructor.InvalidClassNames", "");
    }
  }
  _currentClassIter = _classNames.begin();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevImageCapture::~BehaviorDevImageCapture()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::OnBehaviorActivated()
{
  _touchStartedTime_s = -1.0f;
  _isStreaming = false;
  _timeToBlink = -1.0f;
  _blinkOn = false;

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(true);

  const bool kUseDefaultsForUnspecified = false;
  GetBEI().GetVisionComponent().PushNextModeSchedule(AllVisionModesSchedule({
    {VisionMode::SavingImages, VisionModeSchedule(true)},
  }, kUseDefaultsForUnspecified));
  
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

  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(false);
  visionComponent.PopCurrentModeSchedule();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::SwitchToNextClass()
{
  if(!_classNames.empty())
  {
    ++_currentClassIter; 
    if(_currentClassIter == _classNames.end())
    {
      _currentClassIter = _classNames.begin();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorDevImageCapture::GetSavePath() const
{
  if(_currentClassIter == _classNames.end())
  {
    return _imageSavePath;
  }
  
  return Util::FileUtils::FullFilePath({_imageSavePath, *_currentClassIter});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& visionComponent = GetBEI().GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();

  // update light blinking if needed
  if( _timeToBlink > 0.0f ) {
    if( currTime_s >= _timeToBlink ) {
      BlinkLight();
    }
  }

  // Switch classes when lift is lowered
  const bool isLiftUp = (GetBEI().GetRobotInfo().GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK);
  if(_wasLiftUp && !isLiftUp)
  {
    SwitchToNextClass();    
  }
  _wasLiftUp = isLiftUp;

  if(_currentClassIter != _classNames.end())
  {
    // Note this root path is simply copied from what vision component uses. Ideally we'd share it
    // rather than assuming this is where the images go, but hey, this is a dev behavior, so good
    // enough for now.
    static const std::string rootPath = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "camera/images");
    
    using namespace Util;
    const size_t numFiles = FileUtils::FilesInDirectory(FileUtils::FullFilePath({rootPath, GetSavePath()})).size();
    
    std::function<void(Vision::ImageRGB&)> drawClassName = [this,numFiles](Vision::ImageRGB& img)
    {
      img.DrawText({1,14}, *_currentClassIter + ":" + std::to_string(numFiles), NamedColors::YELLOW, 0.6f, true);
    };
    visionComponent.AddDrawScreenModifier(drawClassName);
  }

  const bool wasTouched = (_touchStartedTime_s >= 0.0f);

  const bool isTouched = (_useCapTouch ? 
                          GetBEI().GetTouchSensorComponent().GetIsPressed() :
                          GetBEI().GetRobotInfo().IsPowerButtonPressed());

  if( wasTouched && !isTouched ) {
    // just "released", see if it's been long enough to count as a "hold"
    if( currTime_s >= _touchStartedTime_s + kHoldTimeForStreaming_s ) {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.longPress", "long press release");
        
      // toggle streaming
      _isStreaming = !_isStreaming;

      const ImageSendMode sendMode = _isStreaming ? ImageSendMode::Stream : ImageSendMode::Off;
      visionComponent.SetSaveImageParameters(sendMode,
                                             GetSavePath(),
                                             _imageSaveQuality);
      
      if( _isStreaming ) {
        BlinkLight();
      }
    }
    else {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.shortPress", "short press release");
      // take single photo
      if (_saveSensorData) {
        visionComponent.SetSaveImageParameters(ImageSendMode::SingleShotWithSensorData,
                                               GetSavePath(),
                                               _imageSaveQuality);
      }
      else {
        visionComponent.SetSaveImageParameters(ImageSendMode::SingleShot,
                                               GetSavePath(),
                                               _imageSaveQuality);
      }

      BlinkLight();
    }
  }
  else if( !wasTouched && isTouched ) {
    PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.newTouch", "new press");
    _touchStartedTime_s = currTime_s;

    if( _isStreaming ) {
      // we were streaming but now should stop because there was a new touch
      visionComponent.SetSaveImageParameters(ImageSendMode::Off,
                                             GetSavePath(),
                                             _imageSaveQuality);

      _isStreaming = false;
    }
  }

  if( !isTouched ) {
    _touchStartedTime_s = -1.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevImageCapture::BlinkLight()
{
  _blinkOn = !_blinkOn;

  GetBEI().GetBodyLightComponent().SetBackpackLights( _blinkOn ? kLightsOn : kLightsOff );
  
  // always blink if we are streaming, and set up another blink for single photo if we need to turn off the
  // light
  if( _blinkOn || _isStreaming ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    // need to do another blink after this one
    _timeToBlink = currTime_s + kLightBlinkPeriod_s;
  }
  else {
    _timeToBlink = -1.0f;
  }
}

}
}

