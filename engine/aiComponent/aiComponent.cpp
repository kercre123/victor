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
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/continuityComponent.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/puzzleComponent.h"
#include "engine/aiComponent/compositeImageCache.h"
#include "engine/aiComponent/timerUtility.h"
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
static const int kObsSampleWindow_ms          = 300;   // sample all measurements over this period
static const f32 kObsTriggerSensitivity       = 1.2;   // scaling factor for robot speed to object speed
static const f32 kObsMinObjectSpeed_mmps      = .1;    // prevents trigger if robot is stationary
static const f32 kObsMaxRotationVariance_rad2 = DEG_TO_RAD(.25);  // variance of heading angle in degrees (stored as radians)
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
void AIComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  const CozmoContext* context = robot->GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }
  
  {
    _aiComponents = std::make_unique<EntityType>();
    {
      auto* faceSelectionComp = new FaceSelectionComponent(*robot, robot->GetFaceWorld(), robot->GetMicDirectionHistory());
      auto* compositeImageCache = new CompositeImageCache(robot->GetContext()->GetDataLoader()->GetFacePNGPaths());
      
      _aiComponents->AddDependentComponent(AIComponentID::BehaviorComponent,          new BehaviorComponent());
      _aiComponents->AddDependentComponent(AIComponentID::ContinuityComponent,        new ContinuityComponent(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::FaceSelection,              faceSelectionComp);
      _aiComponents->AddDependentComponent(AIComponentID::FreeplayDataTracker,        new FreeplayDataTracker());
      _aiComponents->AddDependentComponent(AIComponentID::InformationAnalyzer,        new AIInformationAnalyzer());
      _aiComponents->AddDependentComponent(AIComponentID::ObjectInteractionInfoCache, new ObjectInteractionInfoCache(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::Puzzle,                     new PuzzleComponent(*robot));
      _aiComponents->AddDependentComponent(AIComponentID::CompositeImageCache,        compositeImageCache);
      _aiComponents->AddDependentComponent(AIComponentID::TimerUtility,               new TimerUtility());
      _aiComponents->AddDependentComponent(AIComponentID::Whiteboard,                 new AIWhiteboard(*robot));
    }

    _aiComponents->InitComponents(robot);
  }  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::UpdateDependent(const RobotCompMap& dependentComponents)
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
  f32 avgRobotSpeed_mmps    = 0;  // average foward wheel speed
  f32 avgRotation_rad       = 0;  // average heading angle
  f32 varRotation_radsq     = 0;  // variance of heading angle
  
  const auto&       states  = robot.GetStateHistory()->GetRawPoses();
  const TimeStamp_t endTime = robot.GetLastMsgTimestamp() - kObsSampleWindow_ms;
  
  int n = 0;
  for(auto st = states.rbegin(); st != states.rend() && st->first > endTime; ++st) {
    const auto& state = st->second;
    const float angle_rad = state.GetPose().GetRotationAngle<'Z'>().ToFloat();
    avgProxValue_mm    += state.GetProxSensorVal_mm();
    avgRobotSpeed_mmps += fabs(state.GetLeftWheelSpeed_mmps() + state.GetRightWheelSpeed_mmps());
    avgRotation_rad    += angle_rad;
    varRotation_radsq  += (angle_rad * angle_rad);
    n++;
  }
  // Divide by two here, instead of inside the loop every iteration
  avgRobotSpeed_mmps *= 0.5f;

  if (0 == n) {    // not enough robot history to calculate
    _suddenObstacleDetected = false;
    return;
  }

  avgRobotSpeed_mmps /= n;
  avgProxValue_mm    /= n;
  avgRotation_rad    /= n;
  varRotation_radsq  /= n;
  
  varRotation_radsq = fabs(varRotation_radsq - (avgRotation_rad * avgRotation_rad));

  u16 latestDistance_mm = 0;
  const bool readingIsValid = robot.GetProxSensorComponent().GetLatestDistance_mm(latestDistance_mm);
  
  f32 avgObjectSpeed_mmps = 2 * fabs(avgProxValue_mm - latestDistance_mm) / kObsSampleWindow_ms;
  
  // only trigger if sensor is changing faster than the robot speed, robot is
  // not turning, and sensor is not being changed by noise
  _suddenObstacleDetected = readingIsValid &&
                            (avgObjectSpeed_mmps >= kObsTriggerSensitivity * avgRobotSpeed_mmps) &&
                            (varRotation_radsq   <= kObsMaxRotationVariance_rad2) &&
                            (avgObjectSpeed_mmps >= kObsMinObjectSpeed_mmps) &&
                            (avgProxValue_mm     <= kObsMaxObjectDistance_mm);                   
  
  if (_suddenObstacleDetected) {
    Anki::Util::sEventF("robot.obstacle_detected", {}, 
                        "dist: %4.2f objSpeed: %4.2f roboSpeed: %4.2f", 
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
