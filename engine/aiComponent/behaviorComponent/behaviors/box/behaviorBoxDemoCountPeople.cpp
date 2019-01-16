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

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {
  
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
  
  // NOTE: OffboardPersonDetection requires face/motion detection.
  // TODO: Automatic subscription to dependent modes
  modifiers.visionModesForActiveScope->insert({VisionMode::OffboardPersonDetection, EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::ClassifyingPeople,       EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::DetectingMotion,         EVisionUpdateFrequency::High});
  modifiers.visionModesForActiveScope->insert({VisionMode::MirrorMode,              EVisionUpdateFrequency::High});
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
  
  TransitionToWaitingForPeople();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::TransitionToWaitingForPeople()
{
  _dVars.lastImageTime_ms = GetBEI().GetVisionComponent().GetLastProcessedImageTimeStamp();
  
  WaitForImagesAction* waitForImage = new WaitForImagesAction(1, VisionMode::OffboardPersonDetection);
  waitForImage->SetTimeoutInSeconds(_iConfig.visionRequestTimeout_sec);
  DelegateIfInControl(waitForImage, &BehaviorBoxDemoCountPeople::RespondToAnyPeopleDetected);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoCountPeople::RespondToAnyPeopleDetected()
{
  // TODO: is there a pretty way to reuse (delegate to) CheckForAndReactToSalientPoint here instead?
  const SalientPointsComponent& salientPointsComp = GetAIComp<SalientPointsComponent>();
  
  std::list<Vision::SalientPoint> salientPoints;
  salientPointsComp.GetSalientPointSinceTime(salientPoints, Vision::SalientPointType::Person,
                                             _dVars.lastImageTime_ms);
  
  // Hijack the MirrorMode display string to show the current number of people preset
  int numPeopleSeen = 0;
  for(const auto& salientPoint : salientPoints)
  {
    if(Vision::SalientPointType::Person == salientPoint.salientType)
    {
      ++numPeopleSeen;
    }
  }
  
  LOG_INFO("BehaviorBoxDemoCountPeople.RespondToAnyPeopleDetected",
           "Found %zu salient points, %d people",
           salientPoints.size(), numPeopleSeen);
  
  _dVars.maxFacesSeen = std::max(_dVars.maxFacesSeen, numPeopleSeen);
  const std::string countString("People:" + std::to_string(numPeopleSeen) +
                                " (Max:" + std::to_string(_dVars.maxFacesSeen) + ")" );
  GetBEI().GetVisionComponent().SetMirrorModeDisplayString(countString, NamedColors::YELLOW);
  
  if(_iConfig.waitTimeBetweenImages_sec > 0.f)
  {
    WaitAction* waitAction = new WaitAction(_iConfig.waitTimeBetweenImages_sec);
    DelegateIfInControl(waitAction, &BehaviorBoxDemoCountPeople::TransitionToWaitingForPeople);
  }
  else
  {
    TransitionToWaitingForPeople();
  }
}

}
}
