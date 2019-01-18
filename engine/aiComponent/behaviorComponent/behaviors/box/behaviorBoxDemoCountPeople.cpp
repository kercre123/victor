/**
 * File: BehaviorBoxDemoCountPeople.cpp
 *
 * Author: Andrew Stein
 * Created: 2019-01-14
 *
 * Description: Uses local neural net person classifier to trigger cloud object/person detection
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoCountPeople.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/visionComponent.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "opencv2/highgui.hpp" // Only needed for CV_FONT_NORMAL

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
 
namespace {
  CONSOLE_VAR(f32, kCountPeople_FontScale, "TheBox.Screen",  0.6f);
  CONSOLE_VAR_RANGED(f32, kTheBox_CountPeopleMotionThreshold, "TheBox", 0.05f, 0.f, 1.f);
  CONSOLE_VAR_RANGED(f32, kTheBox_CountPeopleSalientPointOverlapThreshold, "TheBox", 0.5f, 0.f, 1.f);
}
  
namespace ConfigKeys
{
  const char* const kVisionTimeout         = "visionRequestTimeout_sec";
  const char* const kWaitTimeBetweenImages = "waitTimeBetweenImages_sec";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::BehaviorBoxDemoCountPeople(const Json::Value& config)
 : ICozmoBehavior(config)
{
  _iConfig.visionRequestTimeout_sec = JsonTools::ParseFloat(config, ConfigKeys::kVisionTimeout,
                                                            "BehaviorBoxDemoCountPeople");
  
  _iConfig.waitTimeBetweenImages_sec = JsonTools::ParseFloat(config, ConfigKeys::kWaitTimeBetweenImages,
                                                            "BehaviorBoxDemoCountPeople");
  
  SubscribeToTags({EngineToGameTag::RobotObservedMotion});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoCountPeople::~BehaviorBoxDemoCountPeople()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoCountPeople::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.behaviorAlwaysDelegates = false;
  
  modifiers.visionModesForActiveScope->insert({VisionMode::DetectingMotion,              EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorMode,                   EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorModeCacheToWhiteboard,  EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    ConfigKeys::kVisionTimeout,
    ConfigKeys::kWaitTimeBetweenImages,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
 
  _dispImg.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  
  // Wait a few images on startup to clear out MirrorMode?
  WaitForImagesAction* waitAction = new WaitForImagesAction(10);
  DelegateIfInControl(waitAction, &BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::DrawText3Lines(const std::array<std::string,3>& strings, const ColorRGBA& color)
{
  _dispImg.FillWith(Vision::PixelRGB(0,0,0));
  
  const Vec2f textSize = _dispImg.GetTextSize(strings[0], kCountPeople_FontScale, 1);
  const std::array<f32,3> ypos = {{
    FACE_DISPLAY_HEIGHT/2 - 1.5f*textSize.y(),
    FACE_DISPLAY_HEIGHT/2,
    FACE_DISPLAY_HEIGHT/2 + 1.5f*textSize.y(),
  }};
  
  for(s32 i=0; i<3; ++i)
  {
    _dispImg.DrawTextCenteredHorizontally(strings[i], CV_FONT_NORMAL, kCountPeople_FontScale, 1,
                                     color, ypos[i], true);
  }
  
  const bool kInterruptRunning = true;
  GetBEI().GetAnimationComponent().DisplayFaceImage(_dispImg, ANIM_TIME_STEP_MS, kInterruptRunning);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForMotion", "");
  
  DrawText3Lines({{"Looking", "for", "Motion"}}, NamedColors::YELLOW);
  
  _dVars.startImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
  
  _dVars.isWaitingForMotion = true;
  _dVars.motionObserved = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::BehaviorUpdate()
{
  if(_dVars.isWaitingForMotion && _dVars.motionObserved)
  {
    _dVars.isWaitingForMotion = false;
    TransitionToWaitingForPeople();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForPeople()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForPeople", "");
  
  DrawText3Lines({{"Looking", "for", "People"}}, NamedColors::MAGENTA);
  
  WaitForImagesAction* waitForImage = new WaitForImagesAction(1, VisionMode::ClassifyingPeople, _dVars.startImageTime_ms);
  DelegateIfInControl(waitForImage, [this]() {
    
    std::list<Vision::SalientPoint> salientPoints;
    const auto& salientPointComp = GetAIComp<SalientPointsComponent>();
    salientPointComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::PersonPresent,
                                              _dVars.startImageTime_ms);
    
    LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForPeople.ClassifyingPeopleRan",
             "SalientPoints:%zu", salientPoints.size());
    
    if(!salientPoints.empty())
    {
      TransitionToWaitingForCloud();
    }
    else
    {
      TransitionToWaitingForMotion();
    }
    
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForCloud()
{
  LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForCloud", "");
  
  DrawText3Lines({{"Waiting", "for", "Cloud"}}, NamedColors::CYAN);
  
  WaitForImagesAction* waitForImage = new WaitForImagesAction(1, VisionMode::OffboardPersonDetection);
  waitForImage->SetTimeoutInSeconds(_iConfig.visionRequestTimeout_sec);
  DelegateIfInControl(waitForImage, [this]() {
    
    std::list<Vision::SalientPoint> salientPoints;
    const auto& salientPointsComp = GetAIComp<SalientPointsComponent>();
    salientPointsComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::Person,
                                               _dVars.startImageTime_ms);
    
    LOG_INFO("BehaviorBoxDemoCountPeople.TransitionToWaitingForCloud.OffboardPersonDetectionRan",
             "SalientPoints:%zu", salientPoints.size());
    
    const bool drewSomething = DrawSalientPoints(salientPoints);
    if(drewSomething)
    {
      WaitAction* waitAction = new WaitAction(_iConfig.waitTimeBetweenImages_sec);
      DelegateIfInControl(waitAction, &BehaviorBoxDemoCountPeople::TransitionToWaitingForMotion);
    }
    else
    {
      TransitionToWaitingForMotion();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoCountPeople::DrawSalientPoints(const std::list<Vision::SalientPoint>& salientPoints)
{
  if(salientPoints.empty())
  {
    return false;
  }
  
  const TimeStamp_t atTimestamp = salientPoints.front().timestamp;
  const bool gotImage = GetAIComp<AIWhiteboard>().GetMirrorModeImage(atTimestamp, _dispImg);
  if(!gotImage)
  {
    // Couldn't find a mirror mode image that matches the timestamp we want.
    // Fall back on just using the oldest so we still display _something_.
    // This may indicate that we've got more than normal lag to the Cloud, coupled with
    //  not a large enough cache of MirrorMode images in AIWhiteboard.
    LOG_WARNING("BehaviorBoxDemoCountPeople.DrawSalientPoints.NoImageFoundForTimestamp", "t:%u", atTimestamp);
    GetAIComp<AIWhiteboard>().GetOldestMirrorModeImage(_dispImg);
  }
  
  std::vector<Rectangle<f32>> alreadyDrawn;
  for(const auto& salientPoint : salientPoints)
  {
    Poly2f poly(salientPoint.shape);
    
    Rectangle<f32> rect(poly);
    bool keep = true;
    for(const auto& rectAlreadyDrawn : alreadyDrawn)
    {
      // Simple "non-maxima suppression". Don't draw this salient point if it overlaps one we've
      // already drawn too much
      if(rect.ComputeOverlapScore(rectAlreadyDrawn) > kTheBox_CountPeopleSalientPointOverlapThreshold)
      {
        keep = false;
        break;
      }
    }
    
    if(keep)
    {
      alreadyDrawn.emplace_back(rect);
      
      for(auto & pt : poly)
      {
        pt.x() *= FACE_DISPLAY_WIDTH;
        pt.x() = FACE_DISPLAY_WIDTH - pt.x(); // Mirror
        pt.y() *= FACE_DISPLAY_HEIGHT;
      }
      
      _dispImg.DrawPoly(poly, NamedColors::YELLOW, 2);
    }
  }
  
  GetBEI().GetAnimationComponent().DisplayFaceImage(_dispImg, ANIM_TIME_STEP_MS);
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::HandleWhileActivated(const EngineToGameEvent& event)
{
  if(event.GetData().GetTag() == EngineToGameTag::RobotObservedMotion)
  {
    const auto& msg = event.GetData().Get_RobotObservedMotion();
    if(_dVars.isWaitingForMotion && msg.img_area > kTheBox_CountPeopleMotionThreshold)
    {
      _dVars.motionObserved = true;
    }
  }
  else
  {
    LOG_ERROR("BehaviorBoxDemoCountPeople.HandeWhileActivated.UnexpectedEngineToGameEvent", "%s",
              MessageEngineToGameTagToString(event.GetData().GetTag()));
  }
}
  
  
}
}
