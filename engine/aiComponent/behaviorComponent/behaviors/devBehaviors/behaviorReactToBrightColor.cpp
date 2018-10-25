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
const char* const kAnimDetectedBrightColorKey = "animDetectedBrightColor";
const char* const kAnimAtBrightColorKey = "animAtBrightColor";
const char* const kAnimTooFarAwayKey = "animTooFarAway";
const char* const kSalientPointAgeKey = "salientPointAge_ms";

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
  numHues = JsonTools::ParseUInt32(config, kNumHuesKey, debugName);
  DEV_ASSERT(numHues > 0,"Number of hues must be greater than 0");

  targetDistance_mm = JsonTools::ParseUInt32(config, kTargetDistanceKey, debugName);
  celebrationSpins = JsonTools::ParseUInt32(config, kCelebrationSpinsKey, debugName);
  salientPointAge_ms = JsonTools::ParseUInt32(config, kSalientPointAgeKey, debugName);

  animBrightColorDetection = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimDetectedBrightColorKey, animBrightColorDetection, debugName, false);

  animAtBrightColor = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimAtBrightColorKey, animAtBrightColor, debugName, false);

  animTooFarAway = AnimationTrigger::Count;
  JsonTools::GetCladEnumFromJSON(config, kAnimTooFarAwayKey, animTooFarAway, debugName, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToBrightColor::DynamicVariables::DynamicVariables()
{
  Reset(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::DynamicVariables::Reset(bool keepPersistent)
{
  if (! keepPersistent) {
    persistent.lastSeenTimeStamp = 0;
  }
  point.reset();
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
  return;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = false;
  modifiers.wantsToBeActivatedWhenOffTreads = false;
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.behaviorAlwaysDelegates = true;
  modifiers.visionModesForActivatableScope->insert({VisionMode::DetectingBrightColors, EVisionUpdateFrequency::High});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kNumHuesKey,
    kTargetDistanceKey,
    kCelebrationSpinsKey,
    kAnimDetectedBrightColorKey,
    kAnimAtBrightColorKey,
    kAnimTooFarAwayKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorActivated() 
{
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
    PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","No new bright colors to act on");
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::OnBehaviorDeactivated()
{
  TurnOffLights();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::BehaviorUpdate() 
{
  // TODO: monitor for things you care about here
  if( IsActivated() ) {
    // TODO: do stuff here if the behavior is active
  }
  // TODO: delete this function if you don't need it
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBrightColor::CheckIfShouldStop()
{

  // pickup
  {
    const bool pickedUp = GetBEI().GetRobotInfo().IsPickedUp();

    if (pickedUp) {
      PRINT_NAMED_ERROR( "BehaviorReactToBrightColor.BehaviorUpdate.PickedUp",
                         "Robot has been picked up");
      return true;
    }
  }

  // cliff
  {
    const CliffSensorComponent& cliff = GetBEI().GetCliffSensorComponent();
    if (cliff.IsCliffDetected()) {
      PRINT_NAMED_ERROR("BehaviorReactToBrightColor.BehaviorUpdate.CliffSensed",
                        "There's a cliff, not moving");
      return true;
    }
  }

  // note: if an obstacle is in front of the robot, it can still look for a face
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::RoundToNearestHue(std::list<Vision::SalientPoint>& points)
{
  std::for_each(points.begin(), points.end(), std::bind(RoundToNearestHueImpl, _iConfig.numHues, std::placeholders::_1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToBrightColor::FindCurrentColor(Vision::SalientPoint& point)
{
  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestBrightColors;

  const RobotTimeStamp_t timestamp = _dVars.point->timestamp;
  component.GetSalientPointSinceTime(latestBrightColors, Vision::SalientPointType::BrightColors, timestamp);

  // No bright colors since salient point
  if (latestBrightColors.empty())
  {
    return false;
  }

  auto getHue = [](uint32_t color_rgba) -> float {
    ColorRGBA rgba(color_rgba);
    Vision::PixelHSV hsv;
    hsv.FromPixelRGB(Vision::PixelRGB(rgba.r(), rgba.g(), rgba.b()));
    return hsv.h();
  };

  float currHue = getHue(_dVars.point->color_rgba);

  // Ten percent of the hue steps in each direction around the hue
  float closeEpsilon = (1.f/_iConfig.numHues)/10.f;

  auto colorCloseEnough = [&](const Vision::SalientPoint& pt) -> bool {
    float otherHue = getHue(pt.color_rgba);
    return Util::IsNear(currHue, otherHue, closeEpsilon);
  };
  auto iter = std::find_if(latestBrightColors.begin(), latestBrightColors.end(), colorCloseEnough);

  if (iter == latestBrightColors.end())
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
bool BehaviorReactToBrightColor::FindNewColor (Vision::SalientPoint& point)
{
  auto& component = GetAIComp<SalientPointsComponent>();
  std::list<Vision::SalientPoint> latestBrightColors;
  const RobotTimeStamp_t timestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _iConfig.salientPointAge_ms;
  component.GetSalientPointSinceTime(latestBrightColors,Vision::SalientPointType::BrightColors, timestamp);

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TurnOnLights(const Vision::SalientPoint& point)
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
void BehaviorReactToBrightColor::TurnOffLights()
{
  GetBEI().GetBackpackLightComponent().ClearAllBackpackLightConfigs();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToStart()
{
  // Initial behavior:
  // Turn on the backpack lights
  // Turn towards the point
  // Drive forward

  DEV_ASSERT(_dVars.point != nullptr, "ReactToBrightColor started without a point to react to");

  ColorRGBA color(_dVars.point->color_rgba);
  Vision::PixelHSV hsv(Vision::PixelRGB(color.r(),color.g(),color.b()));
  // TODO: Figure out how to get this to show up in webots at DEBUG level
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.ReactToPoint","Reacting to bright color Hue: %d, Score: %d",
                    static_cast<s32>(std::roundf(hsv.h()*360.f)),
                    static_cast<s32>(std::roundf(_dVars.point->score)));

  // TODO: Set color using the color of the salient point
  //    1) Convert to HSV (PixelHSV has all values [0,1]
  //    2) Find dominant color: closest to 360/N where N is the number of colors
  //        - Example N=6: [0,60,120,180,240,300] = [ Red, Yellow, Green, Cyan, Blue, Magenta]
  //    3) Convert to RGB
  // TODO: Make this a configurable range mapping
  TurnOnLights(*_dVars.point); // No Callback, just turns on backpack

  TransitionToNoticedBrightColor();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToNoticedBrightColor()
{
  auto* action = new TriggerAnimationAction(_iConfig.animBrightColorDetection);
  DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToTurnTowardsPoint);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToTurnTowardsPoint()
{
  // Compute angle that we need to turn
  DEV_ASSERT(_dVars.point != nullptr, "Turning towards point without a point to turn towards");

  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction(new TurnTowardsImagePointAction(*_dVars.point));

  DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToLookForCurrentColor);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToLookForCurrentColor()
{
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
    PRINT_NAMED_ERROR("BehaviorReactToBrightColor.OnBehaviorActivated","No new bright colors to act on");
    return;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToDriveTowardsPoint()
{
  // Compute how far to drive forward
  DEV_ASSERT(_dVars.point != nullptr, "Driving towards point without a point to drive towards");
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
      DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToCelebrate);
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
void BehaviorReactToBrightColor::TransitionToCelebrate()
{
#if 0
  CompoundActionSequential* sequence = new CompoundActionSequential();

  CompoundActionParallel* parallel = new CompoundActionParallel();
  parallel->AddAction(new MoveLiftToHeightAction(LIFT_HEIGHT_CARRY));
  parallel->AddAction(new TurnInPlaceAction(3.14159f*2*_iConfig.celebrationSpins, false));

  sequence->AddAction(parallel);
  sequence->AddAction(new MoveLiftToHeightAction(LIFT_PROTO_MIN_HEIGHT));

  DelegateIfInControl(sequence, &BehaviorReactToBrightColor::TransitionToCompleted);
#elif 1
  auto* action = new TriggerAnimationAction(_iConfig.animAtBrightColor);
  DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToCompleted);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToGiveUpLostColor()
{
  auto* action = new TriggerAnimationAction(_iConfig.animTooFarAway);
  DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToGiveUpTooFarAway()
{
  auto* action = new TriggerAnimationAction(_iConfig.animTooFarAway);
  DelegateIfInControl(action, &BehaviorReactToBrightColor::TransitionToCompleted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToBrightColor::TransitionToCompleted()
{
  CancelDelegates(false);
  // TODO: Figure out how to get this to show up in webots at DEBUG level
  PRINT_NAMED_ERROR("BehaviorReactToBrightColor.TransitionToCompleted", "");
}

}
}
