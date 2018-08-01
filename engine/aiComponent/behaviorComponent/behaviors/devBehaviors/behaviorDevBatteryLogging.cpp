/**
 * File: BehaviorDevBatteryLogging.cpp
 *
 * Author: Kevin Yoon
 * Date:   04/06/2018
 *
 * Description: Behavior for logging battery data under a variety of test conditions
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevBatteryLogging.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/actions/basicActions.h"
#include "engine/components/animationComponent.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/robot.h"

// audio
#include "engine/audio/engineRobotAudioClient.h"
#include "clad/audio/audioEventTypes.h"

#include "osState/osState.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/engine/image.h"
#include "util/fileUtils/fileUtils.h"
#include "util/console/consoleInterface.h"

#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <vector>

namespace Anki {
namespace Cozmo {

using AMD_GE_GE = AudioMetaData::GameEvent::GenericEvent;
using AMD_GOT = AudioMetaData::GameObjectType;

namespace {
  std::string _logFile = "";

  const u32 LOGGING_PERIOD_MS = 5000;
  EngineTimeStamp_t _nextLogTime_ms = 0;

  static const char* kWheelSpeedKey = "wheelSpeed_mmps";
  static const char* kLiftSpeedKey = "liftSpeed_degps";
  static const char* kHeadSpeedKey = "headSpeed_degps";
  static const char* kStartMovingVoltageKey = "startMovingVoltage";

  static const char* kStressCPU = "stressCPU";
  static const char* kStressSpeaker = "stressSpeaker";
  static const char* kDriveOffChargerWhenFull = "driveOffChargerWhenFull";

  bool _startMovingVoltageReached = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDevBatteryLogging::InstanceConfig::InstanceConfig()
{
  wheelSpeed_mmps = 0.f;
  liftSpeed_radps = 0.f;
  headSpeed_radps = 0.f;

  startMovingVoltage = 0.f;

  stressWifi = false;
  stressCPU = false;
  stressSpeaker = false;

  driveOffChargerWhenFull = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
BehaviorDevBatteryLogging::BehaviorDevBatteryLogging(const Json::Value& config)
: ICozmoBehavior(config)
{ 
  _iConfig.wheelSpeed_mmps = config.get(kWheelSpeedKey, 0.f).asInt();
  _iConfig.liftSpeed_radps = DEG_TO_RAD(config.get(kLiftSpeedKey, 0.f).asInt());
  _iConfig.headSpeed_radps = DEG_TO_RAD(config.get(kHeadSpeedKey, 0.f).asInt());

  _iConfig.startMovingVoltage = config.get(kStartMovingVoltageKey, 0.f).asFloat();
  _iConfig.stressCPU = config.get(kStressCPU, false).asBool();
  _iConfig.stressSpeaker = config.get(kStressSpeaker, false).asBool();

  _iConfig.driveOffChargerWhenFull = config.get(kDriveOffChargerWhenFull, false).asBool();
}

void BehaviorDevBatteryLogging::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kWheelSpeedKey,
    kLiftSpeedKey,
    kHeadSpeedKey,
    kStartMovingVoltageKey,
    kStressCPU,
    kStressSpeaker,
    kDriveOffChargerWhenFull
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
bool BehaviorDevBatteryLogging::WantsToBeActivatedBehavior() const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::InitBehavior()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::OnBehaviorActivated()
{
  // Stop all actions?
  //Robot& robot = GetBEI().GetRobotInfo()._robot;
  //robot.GetActionList().Cancel();

  _startMovingVoltageReached = false;

  InitLog();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::EnqueueMotorActions()
{
  if (!NEAR_ZERO(_iConfig.wheelSpeed_mmps)) {
    auto& moveComp = GetBEI().GetRobotInfo()._robot.GetMoveComponent();    
    moveComp.DriveWheels(_iConfig.wheelSpeed_mmps, _iConfig.wheelSpeed_mmps, 0.f, 0.f);
  }
  
  static bool goingUp = true;
  auto liftHeight = goingUp ? LIFT_HEIGHT_CARRY : LIFT_HEIGHT_LOWDOCK;
  auto headAngle  = goingUp ? MAX_HEAD_ANGLE    : MIN_HEAD_ANGLE;

  std::list<IActionRunner*> actionList;
  if (!NEAR_ZERO(_iConfig.liftSpeed_radps)) {
    MoveLiftToHeightAction* liftAction = new MoveLiftToHeightAction(liftHeight);
    liftAction->SetMaxLiftSpeed(_iConfig.liftSpeed_radps);
    actionList.push_back(liftAction);
  }

  if (!NEAR_ZERO(_iConfig.headSpeed_radps)) {
    MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction(headAngle);
    headAction->SetMaxSpeed(_iConfig.headSpeed_radps);
    actionList.push_back(headAction);
  }

  IActionRunner* allMotors = new CompoundActionParallel(actionList);

  DelegateIfInControl(allMotors, &BehaviorDevBatteryLogging::EnqueueMotorActions);

  goingUp = !goingUp;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::BehaviorUpdate()
{  
  if(!IsActivated()){
    return;
  }

  // Drive off charger when full
  const auto& battComp = GetBEI().GetRobotInfo()._robot.GetBatteryComponent();
  if (_iConfig.driveOffChargerWhenFull && battComp.IsBatteryFull()) {
    DriveStraightAction* driveAction = new DriveStraightAction(40.f);
    DelegateIfInControl(driveAction);
  }

  // Start motors if voltage threshold has been reached
  if (!_startMovingVoltageReached ) {
    const auto& battComp = GetBEI().GetRobotInfo()._robot.GetBatteryComponent();
    f32 battVoltage = battComp.GetBatteryVolts();
    if (battVoltage < _iConfig.startMovingVoltage ) {
      _startMovingVoltageReached = true;

      if (_iConfig.wheelSpeed_mmps != 0 || _iConfig.liftSpeed_radps != 0 || _iConfig.headSpeed_radps != 0) {
        EnqueueMotorActions();
      }

      if (_iConfig.stressCPU) {
        GetBEI().GetVisionScheduleMediator().SetVisionModeSubscriptions(this, {
          {VisionMode::DetectingFaces, EVisionUpdateFrequency::High},
          {VisionMode::DetectingMarkers, EVisionUpdateFrequency::High},
          {VisionMode::DetectingMotion, EVisionUpdateFrequency::High}
         });
      }

      if (_iConfig.stressSpeaker) {
        GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Loop_Play, AMD_GOT::Behavior);
      }

    }
  }

  if (_startMovingVoltageReached) {
    if (_iConfig.stressCPU) {
      auto& animComp = GetBEI().GetRobotInfo()._robot.GetAnimationComponent();
      Vision::ImageRGB565 img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
      img.FillWith({255, 255, 255});
      animComp.DisplayFaceImage(img, BS_TIME_STEP_MS, true);
    }
  }

  LogData();
}


void BehaviorDevBatteryLogging::InitLog()
{
  // Initialize log file
  std::string outputPath = GetBEI().GetRobotInfo().GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "battery_logs");
  Util::FileUtils::CreateDirectory(outputPath); 

  // Get robot serial number
  std::stringstream ss;
  ss << outputPath << "/BatteryLog_" << std::hex << GetBEI().GetRobotInfo().GetHeadSerialNumber() << "_";

  // Get current date time
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");

  _logFile = ss.str() + ".txt";

  int cnt=1;
  while (Util::FileUtils::FileExists(_logFile)) {
    _logFile = ss.str() + "(" + std::to_string(cnt++) + ")";
  }

  // Add header
  std::ofstream fs;
  fs.open(_logFile.c_str(), std::ios_base::app);
  fs << "Time_ms, "
      << "BatteryVolts, "
      << "BatteryVolts_filtered, "
      << "txBytes, "
      << "rxBytes, "
      << "CPU_activeTime), "
      << "CPU_idleTime)\n";

  PRINT_NAMED_INFO("BehaviorDevBatteryLogging.InitLog.StartingLog", "%s", _logFile.c_str());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::LogData() const
{
  EngineTimeStamp_t currTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  if (currTime_ms > _nextLogTime_ms) {
    _nextLogTime_ms += LOGGING_PERIOD_MS;

    // Get battery voltage
    const auto& battComp = GetBEI().GetRobotInfo()._robot.GetBatteryComponent();
    f32 battVoltage = battComp.GetBatteryVolts();
    f32 battVoltageRaw = battComp.GetBatteryVoltsRaw();
    
    // Get CPU usage
    int activeTime = 0;
    int idleTime = 0; 
    auto& cpuTimeStatsStrVec = OSState::getInstance()->GetCPUTimeStats();
    if (cpuTimeStatsStrVec.size() > 0) {      
      std::stringstream ss(cpuTimeStatsStrVec[0]);  // First line is the overall CPU stats
      std::string junk;
      ss >> junk; // Toss out "cpu" at front
      std::vector<int> cpuTimeStats;
      int num;
      while (ss >> num) {
        cpuTimeStats.push_back(num);
      }
      idleTime = cpuTimeStats[3] + cpuTimeStats[4];  // idle and iowait time
      activeTime = std::accumulate(cpuTimeStats.begin(), cpuTimeStats.end(), 0) - idleTime;
    }

    // Get wifi usage
    auto txBytes = OSState::getInstance()->GetWifiTxBytes();
    auto rxBytes = OSState::getInstance()->GetWifiRxBytes();

    // Write stats to file
    std::ofstream fs;
    fs.open(_logFile.c_str(), std::ios_base::app);
    fs << currTime_ms << ", "
      << battVoltageRaw << ", "
      << battVoltage << ", "
      << txBytes << ", "
      << rxBytes << ", "
      << activeTime << ", " 
      << idleTime << ", "
      << "\n";
    fs.close();
    sync();
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
void BehaviorDevBatteryLogging::OnBehaviorDeactivated()
{
  // Stop motors
  CancelDelegates(false);
  if (!NEAR_ZERO(_iConfig.wheelSpeed_mmps)) {
    auto& moveComp = GetBEI().GetRobotInfo()._robot.GetMoveComponent();
    moveComp.StopBody();
  }

  if (_iConfig.stressSpeaker) {
    GetBEI().GetRobotAudioClient().PostEvent(AMD_GE_GE::Play__Robot_Vic_Sfx__Purr_Loop_Play, AMD_GOT::Behavior);
  }

  _logFile = "";
}

} // Cozmo
} // Anki
