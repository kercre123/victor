/**
 * File: BehaviorRobustChargerObservation.cpp
 *
 * Author: Arjun Menon
 * Created: 2019-03-05
 *
 * Description: Under certain lighting conditions, this behavior 
 * will handle choosing the appropriate vision modes to maximize 
 * probability of observing the charger.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorRobustChargerObservation.h"

#include "engine/actions/basicActions.h"
#include "engine/actions/animActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/components/visionComponent.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/vision/visionProcessingResult.h"
#include "engine/vision/imageSaver.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kNumImagesToWaitForKey = "numImagesToWaitFor";

  // Enable for debug, to save images during WaitForImageAction
  CONSOLE_VAR(bool, kRobustChargerObservation_SaveImages, "Behaviors.RobustChargerObservation", false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRobustChargerObservation::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRobustChargerObservation::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRobustChargerObservation::BehaviorRobustChargerObservation(const Json::Value& config)
 : ICozmoBehavior(config)
{
  std::string debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig.numImagesToWaitFor = JsonTools::ParseInt32(config, kNumImagesToWaitForKey, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorRobustChargerObservation::~BehaviorRobustChargerObservation()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRobustChargerObservation::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers,        EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingIllumination,   EVisionUpdateFrequency::High });
  
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kNumImagesToWaitForKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // Subscribe to vision processing result callbacks
  // - use this callback to track DetectingMarker frames where the image quality was TooDark
  if(GetBEI().HasVisionComponent()) {
    std::function<VisionComponent::VisionResultCallback> func = std::bind(&BehaviorRobustChargerObservation::CheckVisionProcessingResult, this, std::placeholders::_1);
    _dVars.visionResultSignalHandle = GetBEI().GetVisionComponent().RegisterVisionResultCallback(func);
  }
  
  TransitionToIlluminationCheck();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::OnBehaviorDeactivated()
{
  // Destroy signal handle for the vision processing result.
  //  This ensures that we do not get any more callbacks
  //  while the behavior is not activated.
  _dVars.visionResultSignalHandle.reset();

  // Unsubscribe from these vision modes since we enabled them specifically for Look
  GetBEI().GetVisionScheduleMediator().RemoveVisionModeSubscriptions(this, {
    VisionMode::CyclingExposure, 
    VisionMode::CompositingImages,
    VisionMode::FullFrameMarkerDetection,
    VisionMode::MeteringFromChargerOnly,
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::CheckVisionProcessingResult(const VisionProcessingResult& result)
{
  if(result.modesProcessed.Contains(VisionMode::DetectingIllumination) && 
     result.illumination.state != IlluminationState::Unknown) {
    // We only use the Illumination detector if it has a definite response to the current scene illumination
    _dVars.useImageCompositing = (result.illumination.state == IlluminationState::Darkened);
  }

  if(result.modesProcessed.Contains(VisionMode::DetectingMarkers)) {
    if(result.imageQuality == Vision::ImageQuality::TooDark) {
      // Redundant check here for turning on ImageCompositing
      // Some scenarios are IllumState::Unknown but we have a
      //  definite reading from ImageQuality measurement that
      //  indicates we may want to use ImageCompositing
      if(!_dVars.useImageCompositing) {
        _dVars.useImageCompositing = true;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::TransitionToIlluminationCheck()
{
  WaitForImagesAction* waitForIllumState = new WaitForImagesAction(2, VisionMode::DetectingIllumination);
  DelegateIfInControl(waitForIllumState, [this](){
    
    // Request the following vision modes to maximize chance of observation
    //  based on preceeding illumination measurement data
    if(_dVars.useImageCompositing) {
      GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, {
        {VisionMode::CompositingImages,        EVisionUpdateFrequency::High},
        {VisionMode::FullFrameMarkerDetection, EVisionUpdateFrequency::High},
        {VisionMode::MeteringFromChargerOnly,  EVisionUpdateFrequency::High},
      });
    } else {
      GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, {
        {VisionMode::CyclingExposure,          EVisionUpdateFrequency::High},
        {VisionMode::FullFrameMarkerDetection, EVisionUpdateFrequency::High},
        {VisionMode::MeteringFromChargerOnly,  EVisionUpdateFrequency::High},
      });
    }

    TransitionToObserveCharger();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::TransitionToObserveCharger()
{
  CompoundActionSequential* compoundAction = new CompoundActionSequential();
  compoundAction->AddAction(new MoveHeadToAngleAction(0.f));

  const auto currMood = GetBEI().GetMoodManager().GetSimpleMood();
  const bool isHighStim = (currMood == SimpleMoodType::HighStim);
  
  auto* waitAction = new WaitForImagesAction(_iConfig.numImagesToWaitFor, VisionMode::DetectingMarkers);

  if(kRobustChargerObservation_SaveImages) {
    const std::string path = GetBEI().GetRobotInfo().GetDataPlatform()->pathToResource(
                              Util::Data::Scope::Cache,
                              "robustChargerObsImages");
    ImageSaverParams params(path, ImageSaverParams::Mode::Stream, -1);  // Quality: save PNGs
    waitAction->SetSaveParams(params);
  }

  // Wrap the Look action with the appropriate animation, given the current mood
  if (isHighStim) {
    compoundAction->AddAction(waitAction);
  } else {
    compoundAction->AddAction(new LoopAnimWhileAction(waitAction, AnimationTrigger::ChargerDockingSearchWaitForImages));
  }

  DelegateIfInControl(compoundAction); // terminal action after this the behavior exits
}

}
}
