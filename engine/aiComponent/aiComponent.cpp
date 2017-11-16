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

#include "anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/doATrickSelector.h"
#include "engine/aiComponent/feedingSoundEffectManager.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/workoutComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
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
: _suddenObstacleDetected(false)
, _aiInformationAnalyzer(new AIInformationAnalyzer() )
, _freeplayDataTracker(new FreeplayDataTracker() )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Init(Robot& robot, BehaviorComponent*& customBehaviorComponent)
{
  {
    _aiComponents.reset(new ComponentWrappers::AIComponentComponents(robot));
    _objectInteractionInfoCache.reset(new ObjectInteractionInfoCache(robot));
    _whiteboard.reset(new AIWhiteboard(robot) );
    _workoutComponent.reset(new WorkoutComponent(robot) );
    _doATrickSelector.reset(new DoATrickSelector(robot.GetContext()->GetDataLoader()->GetDoATrickWeightsConfig()));
    _severeNeedsComponent.reset(new SevereNeedsComponent(robot));
  }
  
  
  const CozmoContext* context = robot.GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }
  
  if(customBehaviorComponent != nullptr) {
    _behaviorComponent.reset(customBehaviorComponent);
    customBehaviorComponent = nullptr;
  }else{
    _behaviorComponent = std::make_unique<BehaviorComponent>();
    _behaviorComponent->Init(BehaviorComponent::GenerateComponents(robot));
  }
  
  // Toggle flag to "start" the tracking process - legacy assumption that freeeplay is not
  // active on app start - full fix requires a deeper update to the data tracking system
  // that is outside of scope for this PR but should be addressed in VIC-626
  _freeplayDataTracker->SetFreeplayPauseFlag(true, FreeplayPauseFlag::OffTreads);
  _freeplayDataTracker->SetFreeplayPauseFlag(false, FreeplayPauseFlag::OffTreads);
  
  _requestGameComponent = std::make_unique<RequestGameComponent>(robot.HasExternalInterface() ? robot.GetExternalInterface() : nullptr,
                                                                 robot.GetContext()->GetDataLoader()->GetGameRequestWeightsConfig());
  
  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();
  
  assert(_severeNeedsComponent);
  _severeNeedsComponent->Init();
  
  RobotDataLoader* dataLoader = nullptr;
  if(context){
    dataLoader = robot.GetContext()->GetDataLoader();
  }
  

  // initialize workout component
  if(dataLoader != nullptr){
    assert( _workoutComponent );
    const Json::Value& workoutConfig = dataLoader->GetRobotWorkoutConfig();

    const Result res = _workoutComponent->InitConfiguration(workoutConfig);
    if( res != RESULT_OK ) {
      PRINT_NAMED_ERROR("AIComponent.Init.FailedToInitWorkoutComponent",
                        "Couldn't init workout component, deleting");
      return res;      
    }
  }
  
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Update(Robot& robot, std::string& currentActivityName,
                                         std::string& behaviorDebugStr)
{
  // information analyzer should run before behaviors so that they can feed off its findings
  _aiInformationAnalyzer->Update(robot);

  _whiteboard->Update();
  _severeNeedsComponent->Update();
  
  _behaviorComponent->Update(robot, currentActivityName, behaviorDebugStr);

  _freeplayDataTracker->Update();
  
  CheckForSuddenObstacle(robot);

  return RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotDelocalized()
{
  GetWhiteboard().OnRobotDelocalized();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIComponent::OnRobotRelocalized()
{
  GetWhiteboard().OnRobotRelocalized();
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

  const u16 latestDistance_mm = robot.GetProxSensorComponent().GetLatestDistance_mm();
  f32 avgObjectSpeed_mmps = 2 * fabs(avgProxValue_mm - latestDistance_mm) / kObsSampleWindow_ms;

  
  // only trigger if sensor is changing faster than the robot speed, robot is
  // not turning, and sensor is not being changed by noise
  _suddenObstacleDetected = (avgObjectSpeed_mmps >= kObsTriggerSensitivity * avgRobotSpeed_mmps) &&
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

// Support legacy code until move helper comp into delegate component
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BehaviorHelperComponent& AIComponent::GetBehaviorHelperComponent() const
{
  assert(_behaviorComponent);
  return _behaviorComponent->GetBehaviorHelperComponent();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperComponent& AIComponent::GetBehaviorHelperComponent()
{
  assert(_behaviorComponent);
  return _behaviorComponent->GetBehaviorHelperComponent();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& AIComponent::GetBehaviorContainer() {
  return _behaviorComponent->GetBehaviorContainer();
}


} // namespace Cozmo
} // namespace Anki
