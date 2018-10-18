/**
 * File: BehaviorReactToBrightColor.cpp
 *
 * Author: Patrick Doran
 * Created: 2018-10-16
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "behaviorReactToBrightColor.h"

#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/settingsManager.h"

#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/colorPixelTypes.h"

#include <algorithm>
#include <functional>

// TODO: PDoran remove this
#include <random>

namespace Anki {
namespace Vector {

namespace {
const char* const kEyeColorBehaviorKey = "eyeColorBehavior";
const char* const kNumHuesKey = "numHues";

/**
 * @brief Modify the Vision::SalientPoint such that color is rounded to the nearest Hue
 */
void RoundToNearestHueImpl(u32 numHues, Vision::SalientPoint& pt) {
  ColorRGBA color(pt.color_rgba);
  Vision::PixelHSV hsv(Vision::PixelRGB(color.r(),color.g(),color.b()));
  hsv.h() = std::roundf(hsv.h() * numHues)/numHues;
  Vision::PixelRGB rgb = hsv.ToPixelRGB();
  pt.color_rgba = ColorRGBA(rgb.r(),rgb.g(),rgb.b(),255).AsRGBA();
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string& debugName = "BehaviorReactToBrightColor.InstanceConfig.LoadConfig";
  eyeColorBehaviorStr = JsonTools::ParseString(config, kEyeColorBehaviorKey, debugName);
  numHues = JsonTools::ParseUInt32(config, kNumHuesKey, debugName);
  DEV_ASSERT(numHues > 0,"Number of hues must be greater than 0");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::DynamicVariables::DynamicVariables()
{
  Reset(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::DynamicVariables::Reset(bool keepPersistent)
{
  prevEyeColor = 0;
  if (! keepPersistent) {
    persistent.lastSeenTimeStamp = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::BehaviorReactToBrightColor(const Json::Value& config)
 : ICozmoBehavior(config), _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::~BehaviorReactToBrightColor()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBrightColor::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.eyeColorBehavior = BC.FindBehaviorByID(
      BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.eyeColorBehaviorStr));
  DEV_ASSERT(_iConfig.eyeColorBehavior != nullptr,
             "BehaviorReactToBody.InitBehavior.NullEyeColorBehavior");
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActivatableScope->insert({VisionMode::DetectingBrightColors, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.eyeColorBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
#if 1
  const char* list[] = {
    kEyeColorBehaviorKey,
    kNumHuesKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorActivated() 
{
  // reset dynamic variables
  const bool kKeepPersistent = true;
  _dVars.Reset(kKeepPersistent);
  
  // TODO: the behavior is active now, time to do something!
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","PDORAN 1");


  // TODO: Replace this with std::optional like semantics
  Vision::SalientPoint next;
  bool found = FindNewColor(next);
  if (!found)
  {
    PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","No new bright colors to act on");
    return;
  }

  // Found a new color to react to, do something!
  ColorRGBA color(next.color_rgba);
  Vision::PixelHSV hsv(Vision::PixelRGB(color.r(),color.g(),color.b()));
  // TODO: Figure out how to get this to show up in webots at DEBUG level
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","Reacting to bright color Hue: %d, Score: %d",
                    static_cast<s32>(std::roundf(hsv.h()*360.f)),
                    static_cast<s32>(std::roundf(next.score)));


  // TODO: Set color using the color of the salient point
  //    1) Convert to HSV (PixelHSV has all values [0,1]
  //    2) Find dominant color: closest to 360/N where N is the number of colors
  //        - Example N=6: [0,60,120,180,240,300] = [ Red, Yellow, Green, Cyan, Blue, Magenta]
  //    3) Convert to RGB
  // TODO: Make this a configurable range mapping

  // Experiment with eye color
#if 0
  _dVars.prevEyeColor = GetBEI().GetSettingsManager().GetRobotSettingAsUInt(external_interface::RobotSetting::eye_color);
//  u32 color = rand() % 7;
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> uni(0,7);
  int color = uni(rng);
  bool ignored = false;
  GetBEI().GetSettingsManager().SetRobotSetting(external_interface::RobotSetting::eye_color, Json::Value(color), true, ignored);

//  DelegateIfInControl(_iConfig.eyeColorBehavior.get(),[](){
//    PRINT_NAMED_ERROR("BehaviorReactToBrightColor.BehaviorActivated","PDORAN 4");
//  });
#endif


  // Experiment with backpack color
#if 0

  BackpackLightAnimation::BackpackAnimation lights = {
    .onColors               = {{NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{1000,1000,1000}},
    .offPeriod_ms           = {{100,100,100}},
    .transitionOnPeriod_ms  = {{450,450,450}},
    .transitionOffPeriod_ms = {{450,450,450}},
    .offset                 = {{0,0,0}}
  };

  GetBEI().GetBackpackLightComponent().SetBackpackAnimation(lights);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorDeactivated()
{
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorDeactivated","PDORAN 3");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
  }
  // TODO: delete this function if you don't need it
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.BehaviorUpdate","PDORAN 2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::RoundToNearestHue(std::list<Vision::SalientPoint>& points)
{
  std::for_each(points.begin(), points.end(), std::bind(RoundToNearestHueImpl, _iConfig.numHues, std::placeholders::_1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::FindCurrentColor ()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBrightColor::FindNewColor (Vision::SalientPoint& point)
{
  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestBrightColors;
  component.GetSalientPointSinceTime(latestBrightColors,Vision::SalientPointType::BrightColors,
                                     _dVars.persistent.lastSeenTimeStamp);

  // No bright colors since last time
  if (latestBrightColors.empty())
  {
    return false;
  }

  // Color will fluctuate a lot, so we group BrightColor salient points into their nearest hue so we can act on
  // those salient points as if they were the same object.
  RoundToNearestHue(latestBrightColors);

  // TODO: In order to not repeatedly react to the same color, we put the color on cooldown and remove it from
  // the list of SalientPoints that we're looking at.
  auto IsOnCooldown = [](const Vision::SalientPoint& pt) -> bool {
    return false;
  };
  std::remove_if(latestBrightColors.begin(), latestBrightColors.end(), IsOnCooldown);

  // Finally choose the brightest SalientPoint
  auto IsScoreLessThan = [](const Vision::SalientPoint& a, const Vision::SalientPoint& b) -> bool {
    return a.score < b.score;
  };
  auto iter = std::max_element(latestBrightColors.begin(), latestBrightColors.end(), IsScoreLessThan);

  if (iter == latestBrightColors.end())
  {
    // No BrightColors to choose from
    return false;
  }
  else
  {
    // Found a bright color!
    point = *iter;
    return true;
  }
}

}
}
