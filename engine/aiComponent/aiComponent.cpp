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
#include "engine/aiComponent/templatedImageCache.h"
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


namespace ComponentWrappers{
AIComponentComponents::AIComponentComponents(Robot&                      robot,
                                             BehaviorComponent*&         behaviorComponent,
                                             ContinuityComponent*        continuityComponent,
                                             FaceSelectionComponent*     faceSelectionComponent,
                                             FreeplayDataTracker*        freeplayDataTracker,
                                             AIInformationAnalyzer*      infoAnalyzer,
                                             ObjectInteractionInfoCache* objectInteractionInfoCache,
                                             PuzzleComponent*            puzzleComponent,
                                             TemplatedImageCache*        templatedImageCache,
                                             TimerUtility*               timerUtility,
                                             AIWhiteboard*               aiWhiteboard)
:_robot(robot)
,_components({
  {AIComponentID::BehaviorComponent,          ComponentWrapper(behaviorComponent, true)},
  {AIComponentID::ContinuityComponent,        ComponentWrapper(continuityComponent, true)},
  {AIComponentID::FaceSelection,              ComponentWrapper(faceSelectionComponent, true)},
  {AIComponentID::FreeplayDataTracker,        ComponentWrapper(freeplayDataTracker, true)},
  {AIComponentID::InformationAnalyzer,        ComponentWrapper(infoAnalyzer, true)},
  {AIComponentID::ObjectInteractionInfoCache, ComponentWrapper(objectInteractionInfoCache, true)},
  {AIComponentID::Puzzle,                     ComponentWrapper(puzzleComponent, true)},
  {AIComponentID::TemplatedImageCache,        ComponentWrapper(templatedImageCache, true)},
  {AIComponentID::TimerUtility,               ComponentWrapper(timerUtility, true)},
  {AIComponentID::Whiteboard,                 ComponentWrapper(aiWhiteboard, true)}
}){}

AIComponentComponents::~AIComponentComponents()
{
  
}

}


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
Result AIComponent::Init(Robot* robot, BehaviorComponent*& customBehaviorComponent)
{
  const CozmoContext* context = robot->GetContext();

  if(context == nullptr ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }

  {
    BehaviorComponent* behaviorComponent = nullptr;
    bool behaviorCompInitRequired = false;
    if(customBehaviorComponent != nullptr) {
      behaviorComponent = customBehaviorComponent;
      customBehaviorComponent = nullptr;
    }else{
      behaviorComponent = new BehaviorComponent();
      behaviorCompInitRequired = true;
    }

    _aiComponents = std::make_unique<ComponentWrappers::AIComponentComponents>(
          *robot,
          behaviorComponent,
          new ContinuityComponent(*robot),
          new FaceSelectionComponent(*robot, robot->GetFaceWorld(), robot->GetMicDirectionHistory()),
          new FreeplayDataTracker(),
          new AIInformationAnalyzer(),
          new ObjectInteractionInfoCache(*robot),
          new PuzzleComponent(*robot),
          new TemplatedImageCache(robot->GetContext()->GetDataLoader()->GetFacePNGPaths()),
          new TimerUtility(),
          new AIWhiteboard(*robot)
    );

    if(behaviorCompInitRequired){
      auto& behaviorComp = GetComponent<BehaviorComponent>(AIComponentID::BehaviorComponent);
      auto entity = std::make_unique<BehaviorComponent::EntityType>();
      entity->AddDependentComponent(BCComponentID::AIComponent, this, false);
      BehaviorComponent::GenerateManagedComponents(*robot, entity);
      behaviorComp.Init(robot, std::move(entity));
    }
  }
  
  
  auto& dataTracker = GetComponent<FreeplayDataTracker>(AIComponentID::FreeplayDataTracker);
  
  // Toggle flag to "start" the tracking process - legacy assumption that freeeplay is not
  // active on app start - full fix requires a deeper update to the data tracking system
  // that is outside of scope for this PR but should be addressed in VIC-626
  dataTracker.SetFreeplayPauseFlag(true, FreeplayPauseFlag::OffTreads);
  dataTracker.SetFreeplayPauseFlag(false, FreeplayPauseFlag::OffTreads);
    
  // initialize whiteboard
  auto& whiteBoard = GetComponent<AIWhiteboard>(AIComponentID::Whiteboard);
  whiteBoard.Init();
  
  RobotDataLoader* dataLoader = nullptr;
  if(context){
    dataLoader = robot->GetContext()->GetDataLoader();
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Update(Robot& robot, std::string& currentActivityName,
                                         std::string& behaviorDebugStr)
{


  // information analyzer should run before behaviors so that they can feed off its findings
  {
    auto& infoAnalyzer = GetComponent<AIInformationAnalyzer>(AIComponentID::InformationAnalyzer);
    infoAnalyzer.Update(robot);
  }
  {
    auto& whiteboard = GetComponent<AIWhiteboard>(AIComponentID::Whiteboard);
    whiteboard.Update();
  }
  
  // Update continuity component before behavior component to ensure behaviors don't
  // re-gain control when an action is pending
  {
    auto& contComponent = GetComponent<ContinuityComponent>(AIComponentID::ContinuityComponent);
    contComponent.Update();
  }
  
  {
    auto& behaviorComponent = GetComponent<BehaviorComponent>(AIComponentID::BehaviorComponent);
    behaviorComponent.Update(robot, currentActivityName, behaviorDebugStr);
  }

  {
    auto& freeplayTracker = GetComponent<FreeplayDataTracker>(AIComponentID::FreeplayDataTracker);
    freeplayTracker.Update();
  }
  
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer& AIComponent::GetBehaviorContainer() 
{
  auto& behaviorComponent = GetComponent<BehaviorComponent>(AIComponentID::BehaviorComponent);
  return behaviorComponent.GetBehaviorContainer();
}


} // namespace Cozmo
} // namespace Anki
