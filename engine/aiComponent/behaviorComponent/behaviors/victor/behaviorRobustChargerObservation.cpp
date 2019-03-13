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
#include "engine/blockWorld/blockWorld.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/visionComponent.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/vision/imageSaver.h"
#include "engine/robot.h"

#include "util/logging/DAS.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "Behaviors"
  
namespace {
  const char* kNumImageCompositingFramesToWaitForKey = "numImageCompositingFramesToWaitFor";
  const char* kNumCyclingExposureFramesToWaitForKey = "numCyclingExposureFramesToWaitFor";

  const LCDBrightness kNormalLCDBrightness = LCDBrightness::LCDLevel_5mA;
  const LCDBrightness kMaxLCDBrightness = LCDBrightness::LCDLevel_20mA;

  // Enable for debug, to save images during WaitForImageAction
  CONSOLE_VAR(bool, kRobustChargerObservation_SaveImages, "Behaviors.RobustChargerObservation", false);
  CONSOLE_VAR(bool, kFakeLowlightCondition, "Behaviors.RobustChargerObservation", false);
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
  _iConfig.numImageCompositingFramesToWaitFor = JsonTools::ParseInt32(config, kNumImageCompositingFramesToWaitForKey, debugName);
  _iConfig.numCyclingExposureFramesToWaitFor = JsonTools::ParseInt32(config, kNumCyclingExposureFramesToWaitForKey, debugName);

  SubscribeToTags({
    EngineToGameTag::RobotProcessedImage,
  });
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
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers,           EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingIllumination,      EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::FullFrameMarkerDetection,   EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::MeteringFromChargerOnly,    EVisionUpdateFrequency::High });
  
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert(kNumImageCompositingFramesToWaitForKey);
  expectedKeys.insert(kNumCyclingExposureFramesToWaitForKey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::OnBehaviorActivated() 
{
  _dVars = DynamicVariables();
  
  TransitionToIlluminationCheck();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::OnBehaviorDeactivated()
{
  if(_dVars.isLowlight) {
    // Previously played the special Getin and set LCD brightness
    //  so now play the getout and restore the LCD brightness
    CompoundActionSequential* resetAction = new CompoundActionSequential();
    resetAction->AddAction(new TriggerAnimationAction(AnimationTrigger::LowlightChargerSearchGetout));
    resetAction->AddAction(GetLCDBrightnessChangeAction(kNormalLCDBrightness));
    DelegateNow(resetAction);
  }

  DASMSG(robust_observe_charger_stats, "robust_observe_charger.stats", "Vision stats for RobustChargerObservation behavior");
  DASMSG_SET(i1, _dVars.numFramesOfDetectingMarkers, "Count of total number of processed image frames searching for Markers");
  DASMSG_SET(i2, _dVars.numFramesOfImageTooDark, "Count of number of processed image frames (searching for Markers) that are TooDark");
  DASMSG_SET(i3, _dVars.isLowlight, "Whether image compositing or cycling exposure was used here (1=compositing, 0=cycling).");
  DASMSG_SEND();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::RobotProcessedImage:
    {
      if(IsActivated() && GetBEI().HasVisionComponent()) {
        _dVars.numFramesOfDetectingMarkers++;
        if(GetBEI().GetVisionComponent().GetLastImageQuality() == Vision::ImageQuality::TooDark) {
          _dVars.numFramesOfImageTooDark++;
        }
      }
      break;
    }
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorRobustChargerObservation::IsLowLightVision() const
{
  bool isLowlight = false;

  if(GetBEI().HasVisionComponent()) {
    const auto& visionComponent = GetBEI().GetVisionComponent();
    
    auto illumState = visionComponent.GetLastIlluminationState();
    
    // Redundant check here for turning on ImageCompositing
    // Some scenarios are IllumState::Unknown but we have a
    //  definite reading from ImageQuality measurement that
    //  indicates we may want to use ImageCompositing
    isLowlight = (illumState == IlluminationState::Darkened) || 
                 (illumState == IlluminationState::Unknown && 
                 (visionComponent.GetLastImageQuality() == Vision::ImageQuality::TooDark));
  }

  return isLowlight || kFakeLowlightCondition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::TransitionToIlluminationCheck()
{
  WaitForImagesAction* waitForIllumState = new WaitForImagesAction(2, VisionMode::DetectingIllumination);
  DelegateIfInControl(waitForIllumState, &BehaviorRobustChargerObservation::TransitionToObserveCharger);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorRobustChargerObservation::TransitionToObserveCharger()
{
  _dVars.isLowlight = IsLowLightVision();
  LOG_INFO("BehaviorRobustChargerObservation.TransitionToObserveCharger", 
           "Mode = %s", _dVars.isLowlight ? "Compositing" : "CyclingExposure");
  
  CompoundActionSequential* compoundAction = new CompoundActionSequential();

  compoundAction->AddAction(new MoveHeadToAngleAction(0.f));

  WaitForImagesAction* waitAction = nullptr;
  if(_dVars.isLowlight) {
    // Use image compositing
    // - prepend a getin animation and set LCD brightness
    // - wait for images with the appropriate looping animation
    CompoundActionParallel* getinAndSetLcd = new CompoundActionParallel();
    getinAndSetLcd->AddAction(new TriggerAnimationAction(AnimationTrigger::LowlightChargerSearchGetin));
    getinAndSetLcd->AddAction(GetLCDBrightnessChangeAction(kMaxLCDBrightness));
    compoundAction->AddAction(getinAndSetLcd);
    waitAction = new WaitForImagesAction(_iConfig.numImageCompositingFramesToWaitFor, VisionMode::CompositingImages);
    compoundAction->AddAction(new LoopAnimWhileAction(waitAction, AnimationTrigger::LowlightChargerSearchLoop));
  } else {
    // Use cycling exposure instead
    waitAction = new WaitForImagesAction(_iConfig.numCyclingExposureFramesToWaitFor, VisionMode::CyclingExposure);
    const auto currMood = GetBEI().GetMoodManager().GetSimpleMood();
    const bool isHighStim = (currMood == SimpleMoodType::HighStim);
    if (isHighStim) {
      compoundAction->AddAction(waitAction);
    } else {
      compoundAction->AddAction(new LoopAnimWhileAction(waitAction, AnimationTrigger::ChargerDockingSearchWaitForImages));
    }
  }

  if(kRobustChargerObservation_SaveImages) {
    const std::string path = GetBEI().GetRobotInfo().GetDataPlatform()->pathToResource(
                              Util::Data::Scope::Cache,
                              "robustChargerObsImages");
    ImageSaverParams params(path, ImageSaverParams::Mode::Stream, -1);  // Quality: save PNGs
    waitAction->SetSaveParams(params);
  }

  DelegateIfInControl(compoundAction); // Behavior exits after this point
}

WaitForLambdaAction* BehaviorRobustChargerObservation::GetLCDBrightnessChangeAction(const LCDBrightness level) const
{
  return new WaitForLambdaAction([level](Robot& robot) {
                                  robot.GetRobotMessageHandler()->SendMessage(
                                    RobotInterface::EngineToRobot(
                                      RobotInterface::SetLCDBrightnessLevel(level)));
                                  return true;
                                  });
}

}
}
