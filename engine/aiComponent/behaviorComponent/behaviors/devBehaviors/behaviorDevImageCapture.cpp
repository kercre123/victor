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

#include "coretech/common/include/anki/common/basestation/jsonTools.h"
#include "coretech/common/include/anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/bodyLightComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/visionComponent.h"

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

BehaviorDevImageCapture::BehaviorDevImageCapture(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _imageSavePath = JsonTools::ParseString(config, "save_path", "BehaviorDevImageCapture");
  _imageSaveQuality = JsonTools::ParseInt8(config, "quality", "BehaviorDevImageCapture");
  _useCapTouch = JsonTools::ParseBool(config, "use_capacitive_touch", "BehaviorDevImageCapture");
}

BehaviorDevImageCapture::~BehaviorDevImageCapture()
{
}

Result BehaviorDevImageCapture::OnBehaviorActivated(BehaviorExternalInterface& bei)
{
  _touchStartedTime_s = -1.0f;
  _isStreaming = false;
  _timeToBlink = -1.0f;
  _blinkOn = false;

  auto& visionComponent = bei.GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(true);

  auto& robotInfo = bei.GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(false);
  
  return Result::RESULT_OK;
}

void BehaviorDevImageCapture::OnBehaviorDeactivated(BehaviorExternalInterface& bei)
{
  auto& robotInfo = bei.GetRobotInfo();
  // wait for the lift to relax 
  robotInfo.GetMoveComponent().EnableLiftPower(true);

  auto& visionComponent = bei.GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();
  visionComponent.EnableDrawImagesToScreen(false);
}

BehaviorStatus BehaviorDevImageCapture::UpdateInternal_WhileRunning(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& visionComponent = bei.GetComponentWrapper(BEIComponentID::Vision).GetValue<VisionComponent>();

  // update light blinking if needed
  if( _timeToBlink > 0.0f ) {
    if( currTime_s >= _timeToBlink ) {
      BlinkLight(bei);
    }
  }

  const bool wasTouched = _touchStartedTime_s >= 0.0f;

  const bool isTouched = (_useCapTouch ? 
                          bei.GetTouchSensorComponent().IsTouched() :
                          bei.GetRobotInfo().IsPowerButtonPressed());

  if( wasTouched && !isTouched ) {
    // just "released", see if it's been long enough to count as a "hold"
    if( currTime_s >= _touchStartedTime_s + kHoldTimeForStreaming_s ) {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.longPress", "long press release");
        
      // toggle streaming
      _isStreaming = !_isStreaming;

      const ImageSendMode sendMode = _isStreaming ? ImageSendMode::Stream : ImageSendMode::Off;
      visionComponent.SetSaveImageParameters(sendMode,
                                             _imageSavePath,
                                             _imageSaveQuality);
      
      if( _isStreaming ) {
        BlinkLight(bei);
      }
    }
    else {
      PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.shortPress", "short press release");
      // take single photo
      visionComponent.SetSaveImageParameters(ImageSendMode::SingleShot,
                                             _imageSavePath,
                                             _imageSaveQuality);

      BlinkLight(bei);
    }
  }
  else if( !wasTouched && isTouched ) {
    PRINT_CH_DEBUG("Behaviors", "BehaviorDevImageCapture.touch.newTouch", "new press");
    _touchStartedTime_s = currTime_s;

    if( _isStreaming ) {
      // we were streaming but now should stop because there was a new touch
      visionComponent.SetSaveImageParameters(ImageSendMode::Off,
                                             _imageSavePath,
                                             _imageSaveQuality);

      _isStreaming = false;
    }
  }

  if( !isTouched ) {
    _touchStartedTime_s = -1.0f;
  }
  
  // always stay running
  return BehaviorStatus::Running;
}

void BehaviorDevImageCapture::BlinkLight(BehaviorExternalInterface& bei)
{
  _blinkOn = !_blinkOn;

  bei.GetBodyLightComponent().SetBackpackLights( _blinkOn ? kLightsOn : kLightsOff );
  
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

