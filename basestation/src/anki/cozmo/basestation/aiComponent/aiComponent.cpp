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

#include "anki/cozmo/basestation/aiComponent/aiComponent.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/behaviorEventAnimResponseDirector.h"
#include "anki/cozmo/basestation/aiComponent/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/aiComponent/doATrickSelector.h"
#include "anki/cozmo/basestation/aiComponent/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/aiComponent/requestGameComponent.h"
#include "anki/cozmo/basestation/aiComponent/workoutComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace {
static const int kObsSampleWindow_ms          = 300;   // sample all measurements over this period
static const f32 kObsTriggerSensitivity       = 1.2;   // scaling factor for robot speed to object speed
static const f32 kObsMinObjectSpeed_mmps      = .1;    // prevents trigger if robot is stationary
static const f32 kObsMaxRotationVariance_deg2 = .25;   // variance of heading angle in degrees
static const int kObsMaxObjectDistance_mm     = 100;   // don't respond if sensor reading is too far
}

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::AIComponent(Robot& robot)
: _robot(robot)
, _suddenObstacleDetected(false)
, _aiInformationAnalyzer( new AIInformationAnalyzer() )
, _behaviorEventAnimResponseDirector(new BehaviorEventAnimResponseDirector())
, _behaviorHelperComponent( new BehaviorHelperComponent())
, _objectInteractionInfoCache(new ObjectInteractionInfoCache(robot))
, _whiteboard( new AIWhiteboard(robot) )
, _workoutComponent( new WorkoutComponent(robot) )
, _requestGameComponent(new RequestGameComponent(robot))
, _doATrickSelector(new DoATrickSelector(robot))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIComponent::~AIComponent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result AIComponent::Init()
{
  const CozmoContext* context = _robot.GetContext();

  if( !context ) {
    PRINT_NAMED_WARNING("AIComponent.Init.NoContext", "wont be able to load some componenets. May be OK in unit tests");
  }

  // initialize whiteboard
  assert( _whiteboard );
  _whiteboard->Init();

  // initialize workout component
  if( context) {
    assert( _workoutComponent );
    const Json::Value& workoutConfig = context->GetDataLoader()->GetRobotWorkoutConfig();

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
Result AIComponent::Update()
{
  // information analyzer should run before behaviors so that they can feed off its findings
  _aiInformationAnalyzer->Update(_robot);

  _whiteboard->Update();
  
  _behaviorHelperComponent->Update(_robot);
  
  CheckForSuddenObstacle();
   
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
void AIComponent::CheckForSuddenObstacle()
{  
  // calculate average sensor value over several samples
  f32 avgProxValue_mm       = 0;  // average forward sensor value
  f32 avgRobotSpeed_mmps    = 0;  // average foward wheel speed
  f32 avgRotation_deg       = 0;  // average heading angle
  f32 varRotation_degsq     = 0;  // variance of heading angle
  
  const auto        states  = _robot.GetStateHistory()->GetRawPoses();
  const TimeStamp_t endTime = _robot.GetLastMsgTimestamp() - kObsSampleWindow_ms;
  
  int n = 0;
  for(auto st = states.rbegin(); st != states.rend() && st->first > endTime; ++st) {
    Pose2d tempPos      = st->second.GetPose();
    avgProxValue_mm    += st->second.GetProxSensorVal_mm();
    avgRobotSpeed_mmps += fabs(st->second.GetLeftWheelSpeed_mmps() + st->second.GetRightWheelSpeed_mmps()) / 2;
    avgRotation_deg    += RAD_TO_DEG(tempPos.GetAngle().ToFloat());
    varRotation_degsq  += powf(RAD_TO_DEG(tempPos.GetAngle().ToFloat()), 2);
    n++;
  }
  
  if (0 == n) {    // not enough robot history to calculate
    _suddenObstacleDetected = false;
    return;
  }
  
  avgRobotSpeed_mmps /= n;
  avgProxValue_mm    /= n;
  avgRotation_deg    /= n;
  varRotation_degsq  /= n;
  
  varRotation_degsq = fabs(varRotation_degsq - powf(avgRotation_deg, 2));
                 
  f32 avgObjectSpeed_mmps = 2 * fabs(avgProxValue_mm - _robot.GetForwardSensorValue()) / kObsSampleWindow_ms;

  
  // only trigger if sensor is changing faster than the robot speed, robot is
  // not turning, and sensor is not being changed by noise
  _suddenObstacleDetected = (avgObjectSpeed_mmps >= kObsTriggerSensitivity * avgRobotSpeed_mmps) &&
                            (varRotation_degsq   <= kObsMaxRotationVariance_deg2) &&
                            (avgObjectSpeed_mmps >= kObsMinObjectSpeed_mmps) &&
                            (avgProxValue_mm     <= kObsMaxObjectDistance_mm);                   
  
  if (_suddenObstacleDetected) {
    Anki::Util::sEventF("robot.obstacle_detected", {}, 
                        "dist: %4.2f objSpeed: %4.2f roboSpeed: %4.2f", 
                        avgProxValue_mm, avgObjectSpeed_mmps, avgRobotSpeed_mmps);
    PRINT_NAMED_INFO("AIComponent.Update.CheckForSuddenObstacle","SuddenObstacleDetected");
  } 
}

}
}
