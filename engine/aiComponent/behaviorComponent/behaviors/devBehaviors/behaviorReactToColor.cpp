/**
 * File: BehaviorReactToColor.cpp
 *
 * Author: Patrick Doran
 * Created: 2018/11/19
 *
 * Description: React when a bright color is detected
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "behaviorReactToColor.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/salientPointsComponent.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/settingsManager.h"

#include "clad/types/animationTrigger.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/engine/colorPixelTypes.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "coretech/common/engine/utils/timer.h"

#include <algorithm>
#include <functional>

namespace Anki {
namespace Vector {

namespace {
const char* const kNumHuesKey = "numHues";
const char* const kTargetDistanceKey = "targetDistance_mm";
const char* const kCelebrationSpinsKey = "celebrationSpins";
const char* const kAnimDetectKey = "animDetect";
const char* const kAnimReactColorAKey = "animReactColorA";
const char* const kAnimReactColorBKey = "animReactColorB";
const char* const kAnimGiveUpKey = "animGiveUp";
const char* const kSalientPointAgeKey = "salientPointAge_ms";

/**
 * @brief Modify the Vision::SalientPoint such that color is rounded to the nearest Hue
 */
void RoundToNearestHueImpl(u32 numHues, Vision::SalientPoint& pt) {
  ColorRGBA color(pt.color_rgba);
  Vision::PixelHSV hsv(Vision::PixelRGB(color.r(),color.g(),color.b()));
  hsv.h() = std::roundf(hsv.h() * numHues)/numHues;
  hsv.s() = 1.f;
  hsv.v() = 1.f;
  Vision::PixelRGB rgb = hsv.ToPixelRGB();
  pt.color_rgba = ColorRGBA(rgb.r(),rgb.g(),rgb.b(),255).AsRGBA();
}

float GetHue(uint32_t color_rgba){
  ColorRGBA rgba(color_rgba);
  Vision::PixelHSV hsv;
  hsv.FromPixelRGB(Vision::PixelRGB(rgba.r(), rgba.g(), rgba.b()));
  return hsv.h();
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToColor::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const std::string& debugName = "BehaviorReactToColor.InstanceConfig.LoadConfig";
  numHues = JsonTools::ParseUInt32(config, kNumHuesKey, debugName);
  DEV_ASSERT(numHues > 0,"Number of hues must be greater than 0");

  // Because we divide the incoming hues into numHues and round the incoming SalientPoint colors to the nearest hue
  // we really don't need an epsilon since they *should* be equal. In reality we're comparing floating points so
  // we need an epsilon, so it might as well be _most_ of the range. By doing 0.4 (or 40% of the space between the
  // hues that we round to), we're capturing anything within a 40% error. Again, this should not matter because we
  // are rounding all the incoming colors to the nearest hue. The maximum value for this scalar is 0.5f, otherwise
  // the error range starts to overlap with the error range for neighboring hues.
  closeHueEpsilon = (1.f/numHues)*0.4f;

  targetDistance_mm = JsonTools::ParseUInt32(config, kTargetDistanceKey, debugName);
  celebrationSpins = JsonTools::ParseUInt32(config, kCelebrationSpinsKey, debugName);
  salientPointAge_ms = JsonTools::ParseUInt32(config, kSalientPointAgeKey, debugName);

  animDetect = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimDetectKey, animDetect, debugName, false);

  animReactColorA = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimReactColorAKey, animReactColorA, debugName, false);

  animReactColorB = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimReactColorBKey, animReactColorB, debugName, false);

  animGiveUp = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimGiveUpKey, animGiveUp, debugName, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToColor::DynamicVariables::DynamicVariables()
{
  Reset(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::DynamicVariables::Reset(bool keepPersistent)
{
  if (! keepPersistent) {
    persistent.lastSeenTimeStamp = 0;
  }
  point.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToColor::BehaviorReactToColor(const Json::Value& config)
 : ICozmoBehavior(config), _iConfig(config)
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.BehaviorReactToColor","PDORAN 0");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToColor::~BehaviorReactToColor()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToColor::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::InitBehavior()
{
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.GetBehaviorOperationModifiers","PDORAN 0");
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.visionModesForActivatableScope->insert({VisionMode::DetectingColors, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kNumHuesKey,
    kTargetDistanceKey,
    kCelebrationSpinsKey,
    kAnimDetectKey,
    kAnimReactColorAKey,
    kAnimReactColorBKey,
    kAnimGiveUpKey,
    kSalientPointAgeKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::OnBehaviorActivated() 
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.OnBehaviorActivated","PDORAN 0");
  // reset dynamic variables
  const bool kKeepPersistent = true;
  _dVars.Reset(kKeepPersistent);

  // TODO: Replace this with std::optional like semantics
  Vision::SalientPoint next;
  bool found = FindNewColor(next);
  if (found)
  {
    _dVars.point = std::make_shared<Vision::SalientPoint>(next);
    TransitionToStart();
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorReactToColor.OnBehaviorActivated","No new colors to act on");
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::OnBehaviorDeactivated()
{
  TurnOffLights();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
  }
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToColor::CheckIfShouldStop()
{

  // pickup
  {
    const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

    if (pickedUp) {
      PRINT_NAMED_ERROR( "BehaviorReactToColor.BehaviorUpdate.PickedUp",
                         "Robot has been picked up");
      return true;
    }
  }

  // cliff
  {
    const CliffSensorComponent& cliff = GetBEI().GetCliffSensorComponent();
    if (cliff.IsCliffDetected()) {
      PRINT_NAMED_ERROR("BehaviorReactToColor.BehaviorUpdate.CliffSensed",
                        "There's a cliff, not moving");
      return true;
    }
  }

  // note: if an obstacle is in front of the robot, it can still look for a face
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::RoundToNearestHue(std::list<Vision::SalientPoint>& points)
{
  std::for_each(points.begin(), points.end(), std::bind(RoundToNearestHueImpl, _iConfig.numHues, std::placeholders::_1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToColor::FindCurrentColor(Vision::SalientPoint& point)
{
  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestColors;

  const RobotTimeStamp_t timestamp = _dVars.point->timestamp;
  component.GetSalientPointSinceTime(latestColors, Vision::SalientPointType::Color, timestamp);

  // No colors since salient point
  if (latestColors.empty())
  {
    return false;
  }

  float currHue = GetHue(_dVars.point->color_rgba);

  RoundToNearestHue(latestColors);
  auto colorCloseEnough = [&](const Vision::SalientPoint& pt) -> bool {
    float otherHue = GetHue(pt.color_rgba);
    return Util::IsNear(currHue, otherHue, _iConfig.closeHueEpsilon);
  };
  auto iter = std::find_if(latestColors.begin(), latestColors.end(), colorCloseEnough);

  if (iter == latestColors.end())
  {
    return false;
  }
  else
  {
    point = *iter;
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToColor::FindNewColor (Vision::SalientPoint& point)
{
  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestColors;
  const RobotTimeStamp_t timestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _iConfig.salientPointAge_ms;
  component.GetSalientPointSinceTime(latestColors,Vision::SalientPointType::Color, timestamp);

  // No colors since last time
  if (latestColors.empty())
  {
    return false;
  }

  // Color will fluctuate a lot, so we group Color salient points into their nearest hue so we can act on
  // those salient points as if they were the same object.
  RoundToNearestHue(latestColors);

  // TODO: In order to not repeatedly react to the same color, we put the color on cooldown and remove it from
  // the list of SalientPoints that we're looking at.
  auto IsOnCooldown = [](const Vision::SalientPoint& pt) -> bool {
    return false;
  };
  std::remove_if(latestColors.begin(), latestColors.end(), IsOnCooldown);

  // Finally choose the largest and brightest SalientPoint
  auto IsScoreLessThan = [](const Vision::SalientPoint& a, const Vision::SalientPoint& b) -> bool {
    return (a.score*a.area_fraction) < (b.score*b.area_fraction);
  };
  auto iter = std::max_element(latestColors.begin(), latestColors.end(), IsScoreLessThan);

  if (iter == latestColors.end())
  {
    // No Colors to choose from
    return false;
  }
  else
  {
    // Found a bright color!
    point = *iter;
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TurnOnLights(const Vision::SalientPoint& point)
{
  BackpackLightAnimation::BackpackAnimation lights = {
    .onColors               = {{point.color_rgba, point.color_rgba, point.color_rgba}},
    .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
    .onPeriod_ms            = {{300,300,300}},
    .offPeriod_ms           = {{150,150,150}},
    .transitionOnPeriod_ms  = {{50,50,50}},
    .transitionOffPeriod_ms = {{50,50,50}},
    .offset                 = {{100,50,0}}
  };

  GetBEI().GetBackpackLightComponent().SetBackpackAnimation(lights);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TurnOffLights()
{
  GetBEI().GetBackpackLightComponent().ClearAllBackpackLightConfigs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToStart()
{
  // Initial behavior:
  // Turn on the backpack lights
  // Turn towards the point
  // Drive forward

  DEV_ASSERT(_dVars.point != nullptr, "ReactToColor started without a point to react to");

  ColorRGBA color(_dVars.point->color_rgba);
  Vision::PixelHSV hsv(Vision::PixelRGB(color.r(),color.g(),color.b()));
  // TODO: Figure out how to get this to show up in webots at DEBUG level
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToStart","Reacting to bright color Hue: %d, Score: %d",
                    static_cast<s32>(std::roundf(hsv.h()*360.f)),
                    static_cast<s32>(std::roundf(_dVars.point->score)));

  // TODO: Set color using the color of the salient point
  //    1) Convert to HSV (PixelHSV has all values [0,1]
  //    2) Find dominant color: closest to 360/N where N is the number of colors
  //        - Example N=6: [0,60,120,180,240,300] = [ Red, Yellow, Green, Cyan, Blue, Magenta]
  //    3) Convert to RGB
  // TODO: Make this a configurable range mapping
  TurnOnLights(*_dVars.point); // No Callback, just turns on backpack

  TransitionToNoticedColor();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToNoticedColor()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToNoticedColor","");
#if 1
  auto* action = new TriggerAnimationAction(_iConfig.animDetect);
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToTurnTowardsPoint);
#elif 0
  TransitionToTurnTowardsPoint();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToTurnTowardsPoint()
{
  DEV_ASSERT(_dVars.point != nullptr, "Turning towards point without a point to turn towards");
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToTurnTowardsPoint","");

#if 0
  DEV_ASSERT(_dVars.point != nullptr, "Turning towards point without a point to turn towards");
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToTurnTowardsPoint","");
  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction(new TurnTowardsImagePointAction(*_dVars.point));
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToLookForCurrentColor);

#elif 1
  // We need a more recent point because we just played an animation and may have lost the point due
  // to looking away or simply having lost the history of the robot state due to age. Without that
  // then we don't know where to turn.
  Vision::SalientPoint curr;
  bool found = FindCurrentColor(curr);
  if (found)
  {
    // Update the current point to be the latest
    _dVars.point = std::make_shared<Vision::SalientPoint>(curr);

    CompoundActionSequential* action = new CompoundActionSequential();
    action->AddAction(new TurnTowardsImagePointAction(*_dVars.point));

    DelegateIfInControl(action, &BehaviorReactToColor::TransitionToLookForCurrentColor);
  }
  else
  {
    TransitionToGiveUpLostColor();
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToLookForCurrentColor()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToLookForColor","");
  Vision::SalientPoint curr;
  bool found = FindCurrentColor(curr);
  if (found)
  {
    // Update the current point to be the latest
    _dVars.point = std::make_shared<Vision::SalientPoint>(curr);
    TransitionToDriveTowardsPoint();
  }
  else
  {
    TransitionToGiveUpLostColor();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToDriveTowardsPoint()
{
  // Compute how far to drive forward
  DEV_ASSERT(_dVars.point != nullptr, "Driving towards point without a point to drive towards");
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToDriveTowardsPoint","");

  bool shouldGoStraight = false;
  const ProxSensorComponent &prox = GetBEI().GetRobotInfo().GetProxSensorComponent();
  u16 distance_mm = 0;
  f32 targetDistance_mm = 0;
  if (prox.GetLatestDistance_mm(distance_mm))
  {
    if (distance_mm <= _iConfig.targetDistance_mm)
    {
      // Close enough, don't move
      shouldGoStraight = false;
      targetDistance_mm = 0.f;
    }
    else
    {
      // Can see obstacle, get closer
      shouldGoStraight = true;
      targetDistance_mm = distance_mm - _iConfig.targetDistance_mm;
    }

    if (shouldGoStraight)
    {
      CompoundActionSequential * action;
      // Far enough away that we should drive closer
      action = new CompoundActionSequential({ new DriveStraightAction(targetDistance_mm) });
      CancelDelegates(false);
      DelegateIfInControl(action, &BehaviorReactToColor::TransitionToCheckGotToObject);
    }
    else
    {
      // Close enough that we can celebrate
      TransitionToCelebrate();
    }
  }
  else
  {
    // Prox sensor says there is nothing close enough to be considered valid. So just give up.
    TransitionToGiveUpTooFarAway();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToCheckGotToObject()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToCheckGotToObject","");
  const ProxSensorComponent &prox = GetBEI().GetRobotInfo().GetProxSensorComponent();
  u16 distance_mm = 0;
  if (prox.GetLatestDistance_mm(distance_mm))
  {
    if (distance_mm <= _iConfig.targetDistance_mm)
    {
      // Close enough
      TransitionToCelebrate();
    }
    else
    {
      // Not close enough, but still in valid range
      TransitionToGiveUpLostObject();
    }
  } else {
    // Not close enough, outside valid range
    TransitionToGiveUpLostObject();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToCelebrate()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToCelebrate","");
  const float hue = GetHue(_dVars.point->color_rgba);
  // Spint on warm colors vs cool colors
  const bool isWarm = (hue < 0.25 || hue > 0.75);
  const AnimationTrigger trigger = (isWarm)? _iConfig.animReactColorA : _iConfig.animReactColorB;
  auto* action = new TriggerAnimationAction(trigger);
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToGiveUpLostColor()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToGiveUpLostColor","");
  auto* action = new TriggerAnimationAction(_iConfig.animGiveUp);
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToGiveUpTooFarAway()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToGiveUpTooFarAway","");
  auto* action = new TriggerAnimationAction(_iConfig.animGiveUp);
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToGiveUpLostObject()
{
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToGiveUpLostObject","");
  auto* action = new TriggerAnimationAction(_iConfig.animGiveUp);
  DelegateIfInControl(action, &BehaviorReactToColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToColor::TransitionToCompleted()
{
  // TODO: Figure out how to get this to show up in webots at DEBUG level
  PRINT_NAMED_ERROR("BehaviorReactToColor.TransitionToCompleted", "");

  CancelDelegates(false);
}

} /* namespace vector */
} /* namespace anki */
