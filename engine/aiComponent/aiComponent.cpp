/**
 * File: aiComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-12-07
 *
 * Description: Component for all aspects of the higher level AI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/aiComponent.h"

#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/shared/radiansMath.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/continuityComponent.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/puzzleComponent.h"
#include "engine/aiComponent/salientPointsDetectorComponent.h"
#include "engine/aiComponent/timerUtility.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotStateHistory.h"

namespace {
static const int kLowQualityProxDistance_mm   = 300;   // Assumed distance if prox reading is low quality
static const int kObsSampleWindow_ms          = 300;   // sample all measurements over this period
static const f32 kObsSampleWindow_s           = Anki::Util::MilliSecToSec((f32)kObsSampleWindow_ms);
static const int kNumRequiredSamples          = static_cast<int>(kObsSampleWindow_s / 0.03f) - 1; // Require almost all samples to be valid
static const f32 kObsTriggerSensitivity       = 1.2;   // scaling factor for robot speed to object speed
static const f32 kObsMinObjectSpeed_mmps      = 50;    // prevents trigger if robot is stationary
static const f32 kObsMaxRotation_rad          = DEG_TO_RAD(3);  // max rotation amount across all samples before considered turning
static const int kObsMaxObjectDistance_mm     = 100;   // don't respond if sensor reading is too far
}

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::AIComponent()
: UnreliableComponent<BCComponentID>(this, BCComponentID::AIComponent)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::AIComponent)
, _suddenObstacleDetected(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{
  _aiComponents.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  const CozmoContext* context = robot->GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }

  {
    _aiComponents = std::make_unique<EntityType>();
    {
      const MicDirectionHistory& micDirectionHistory = dependentComps.GetComponent<MicComponent>().GetMicDirectionHistory();
      auto* faceSelectionComp = new FaceSelectionComponent(*robot, dependentComps.GetComponent<FaceWorld>(), 
                                                           micDirectionHistory, dependentComps.GetComponent<VisionComponent>());

      _aiComponents->AddDependentComponent(AIComponentID::BehaviorComponent,                 new BehaviorComponent());
      _aiComponents->AddDependentComponent(AIComponentID::ContinuityComponent,               new ContinuityComponent(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::FaceSelection,                     faceSelectionComp);
      _aiComponents->AddDependentComponent(AIComponentID::FreeplayDataTracker,               new FreeplayDataTracker());
      _aiComponents->AddDependentComponent(AIComponentID::InformationAnalyzer,               new AIInformationAnalyzer());
      _aiComponents->AddDependentComponent(AIComponentID::ObjectInteractionInfoCache,        new ObjectInteractionInfoCache(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::Puzzle,                            new PuzzleComponent(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::SalientPointsDetectorComponent,    new SalientPointsDetectorComponent());
      _aiComponents->AddDependentComponent(AIComponentID::TimerUtility,                      new TimerUtility());
      _aiComponents->AddDependentComponent(AIComponentID::Whiteboard,                        new AIWhiteboard(*robot));
    }

    _aiComponents->InitComponents(robot);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  _aiComponents->UpdateComponents();
  CheckForSuddenObstacle(*_robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotDelocalized()
{
  GetComponent<AIWhiteboard>().OnRobotDelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotRelocalized()
{
  GetComponent<AIWhiteboard>().OnRobotRelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::CheckForSuddenObstacle(Robot& robot)
{
  // calculate average sensor value over several samples
  f32 avgProxValue_mm       = 0;  // average forward sensor value
  f32 avgRobotSpeed_mmps    = 0;  // average forward wheel speed
  std::vector<Radians> angleVec;  // set of angles from history
  angleVec.reserve(kNumRequiredSamples);

  const auto&       states  = robot.GetStateHistory()->GetRawPoses();
  const TimeStamp_t endTime = robot.GetLastMsgTimestamp() - kObsSampleWindow_ms;

  int n = 0;
  for(auto st = states.rbegin(); st != states.rend() && st->first > endTime; ++st) {
    const auto& state = st->second;
    angleVec.push_back(state.GetPose().GetRotationAngle<'Z'>());
    const auto& proxData = state.GetProxSensorData();

    // Ignore readings where lift was in fov or it's too pitched
    if (proxData.isLiftInFOV || proxData.isTooPitched) {
      continue;
    }

    // If signal quality was low, then assume some fixed far
    // distance since the actual reading can vary wildly.
    avgProxValue_mm    += proxData.isValidSignalQuality ?
                          state.GetProxSensorVal_mm() :
                          kLowQualityProxDistance_mm;

    avgRobotSpeed_mmps += state.GetLeftWheelSpeed_mmps() + state.GetRightWheelSpeed_mmps();

    n++;
  }

  // Check that there are a sufficient number of samples
  if (n < kNumRequiredSamples) {
    _suddenObstacleDetected = false;
    return;
  }

  // Divide by two here, instead of inside the loop every iteration
  avgRobotSpeed_mmps *= 0.5f;

  avgRobotSpeed_mmps /= n;
  avgProxValue_mm    /= n;

  // Get the angular range of all angles in history
  Radians meanAngle;
  f32 distToMinAngle_rad;
  f32 distToMaxAngle_rad;
  RadiansMath::ComputeMinMaxMean(angleVec, meanAngle, distToMinAngle_rad, distToMaxAngle_rad);
  const f32 angleRange_rad = distToMaxAngle_rad - distToMinAngle_rad;

  // Get latest distance reading and assess validity
  const auto& proxData = robot.GetProxSensorComponent().GetLatestProxData();
  const u16 latestDistance_mm = proxData.distance_mm;
  const bool readingIsValid = proxData.isValidSignalQuality &&
                              !proxData.isLiftInFOV &&
                              !proxData.isTooPitched;

  // (Not-exactly) "average" speed at which object is approaching robot
  // If it was looking at nothing and then an obstacle appears in front of it,
  // this speed should be quite high
  f32 avgObjectSpeed_mmps = (avgProxValue_mm - latestDistance_mm) / kObsSampleWindow_s;

  // Sudden obstacle detection conditions
  // 1) Signal quality sufficiently indicates that something was detected
  // 2) Robot is stationary or moving forward
  // 3) Object according to sensor is approaching robot faster than it's moving
  //    (i.e. it's not a stationary object)
  // 4) Robot is not turning
  // 5) Object is moving faster than some min speed
  // 6) Last sensor reading is less than a certain distance that defines
  //    how close an obstacle needs to be in order for it to be sudden.
  _suddenObstacleDetected = readingIsValid &&
                            (avgRobotSpeed_mmps  >= 0.f) &&
                            (avgObjectSpeed_mmps >= kObsTriggerSensitivity * avgRobotSpeed_mmps) &&
                            (angleRange_rad <= kObsMaxRotation_rad) &&
                            (avgObjectSpeed_mmps >= kObsMinObjectSpeed_mmps) &&
                            (latestDistance_mm   <= kObsMaxObjectDistance_mm);

  if (_suddenObstacleDetected) {
    Anki::Util::sInfoF("robot.obstacle_detected", {},
                       "dist: %4.2f objSpeed: %4.2f robotSpeed: %4.2f",
                       avgProxValue_mm, avgObjectSpeed_mmps, avgRobotSpeed_mmps);
    PRINT_NAMED_INFO("AIComponent.Update.CheckForSuddenObstacle","SuddenObstacleDetected");
  }
}

#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& AIComponent::GetBehaviorContainer()
{
  auto& behaviorComponent = GetComponent<BehaviorComponent>();
  return behaviorComponent.GetBehaviorContainer();
}
#endif

} // namespace Cozmo
} // namespace Anki
