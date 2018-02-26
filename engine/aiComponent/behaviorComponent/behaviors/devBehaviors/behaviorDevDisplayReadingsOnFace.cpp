/**
 * File: behaviorDevDisplayReadingsOnFace.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/13/18
 *
 * Description: Dev behavior which makes it easy to write text to Victor's face so that
 * design and engineering can quickly get a sense of realtime values the robot is using for decision making
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevDisplayReadingsOnFace.h"

#include "engine/components/animationComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"


namespace Anki {
namespace Cozmo {

namespace {
static const float kMagicNumberSpaceBetweenText = 20.f;
static const float kMagicNumberTextScale = 0.5f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDisplayReadingsOnFace::BehaviorDevDisplayReadingsOnFace(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedMotion
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevDisplayReadingsOnFace::~BehaviorDevDisplayReadingsOnFace()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::OnBehaviorActivated()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::OnBehaviorDeactivated()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::BehaviorUpdate()
{
  if(!IsActivated())
  {
    return;
  }

  _dVars.image = Vision::Image(FACE_DISPLAY_HEIGHT,FACE_DISPLAY_WIDTH, NamedColors::BLACK);
  _dVars.autoLayoutPoint = Point2f(0,kTopLeftCornerMagicNumber);
  // Include whatever information you want displayed here
  UpdateDisplayProxReading();
  UpdateDisplayPeripheralMotion();

  GetBEI().GetAnimationComponent().DisplayFaceImage(_dVars.image, 1.0f, true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::DrawTextWithAutoLayout(const std::string& text)
{
  auto layoutPoint = _dVars.autoLayoutPoint;
  _dVars.image.DrawText(layoutPoint, text, NamedColors::WHITE, kMagicNumberTextScale);
  _dVars.autoLayoutPoint.y() += kMagicNumberSpaceBetweenText;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::UpdateDisplayPeripheralMotion()
{
  if(_dVars.motionIsValid){
    std::string leftText   = "Left Periph:"   + std::to_string(_dVars.motionMessage.left_img_area);
    std::string rightText  = "Right Periph:"  + std::to_string(_dVars.motionMessage.right_img_area);
    std::string topText    = "Top Periph:"    + std::to_string(_dVars.motionMessage.top_img_area);
    std::string bottomText = "Bottom Periph:" + std::to_string(_dVars.motionMessage.bottom_img_area);
    DrawTextWithAutoLayout(leftText);
    DrawTextWithAutoLayout(rightText);
    DrawTextWithAutoLayout(topText);
    DrawTextWithAutoLayout(bottomText);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::UpdateDisplayProxReading()
{
  auto& proxSensor = GetBEI().GetComponentWrapper(BEIComponentID::ProxSensor).GetValue<ProxSensorComponent>();
  u16 proxDist_mm = 0;
  const bool isValid = proxSensor.GetLatestDistance_mm(proxDist_mm);

  std::string text = "Prox (mm): ";
  if(isValid){
    text += std::to_string(proxDist_mm);
  }else{
    text += "INVALID READING";
  }

  DrawTextWithAutoLayout(text);


  {
    std::string text = "Intensity: ";
    auto proxData = proxSensor.GetLatestProxData();
    text += std::to_string(proxData.signalIntensity/proxData.spadCount);


    DrawTextWithAutoLayout(text);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDevDisplayReadingsOnFace::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch (event.GetData().GetTag())
  {
    case ExternalInterface::MessageEngineToGameTag::RobotObservedMotion: {
      _dVars.motionMessage = event.GetData().Get_RobotObservedMotion();
      _dVars.motionIsValid = true;
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorPounceOnMotion.AlwaysHandle.InvalidEvent", "");
      break;
    }
  }
  
}


} // namespace Cozmo
} // namespace Anki

