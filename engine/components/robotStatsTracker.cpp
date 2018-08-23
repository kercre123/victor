/**
 * File: robotStatsTracker.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-06-07
 *
 * Description: Persistent storage of robot lifetime stats
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/robotStatsTracker.h"

#include "clad/types/behaviorComponent/activeFeatures.h"
#include "clad/types/behaviorComponent/behaviorStats.h"
#include "clad/types/behaviorComponent/userIntent.h"
#include "engine/components/jdocsManager.h"
#include "engine/robot.h"
#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"
#include "util/console/consoleInterface.h"


#define LOG_CHANNEL "RobotStatsTracker"

namespace Anki {
namespace Vector {

namespace {

static const char* kStimCategory = "Stim";
static const char* kActiveFeatureCategory = "Feature";
static const char* kBehaviorStatCategory = "BStat";
static const char* kFacesCategory = "Face";
static const char* kOdomCategory = "Odom";

static const char* kRobotStatsSeparator = ".";

static const float kStimFixedPoint = 1000.0f;
static const float kOdomFixedPoint = 100000.0f;

#if REMOTE_CONSOLE_ENABLED
// hack to allow a console func to reset the stats
static std::function<void(void)> sRobotStatsResetAllFunction;

void ResetRobotStats( ConsoleFunctionContextRef context )
{
  if( sRobotStatsResetAllFunction ) {

    // require a "reset" argument to avoid accidents
    const std::string confirmStr = ConsoleArg_Get_String(context, "typeResetToConfirm");
    if( confirmStr == "reset" ) {
      sRobotStatsResetAllFunction();
    }
    else {
      PRINT_NAMED_WARNING("ResetRobotStats.ConsoleFunc.Invalid",
                          "Must type 'reset' to reset");
    }
  }
}
#endif // REMOTE_CONSOLE_ENABLED

}

#define CONSOLE_GROUP "RobotStats"

CONSOLE_FUNC( ResetRobotStats, CONSOLE_GROUP, const char* typeResetToConfirm );

RobotStatsTracker::RobotStatsTracker()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RobotStatsTracker)
  , UnreliableComponent<BCComponentID>(this, BCComponentID::RobotStatsTracker)
{
#if REMOTE_CONSOLE_ENABLED
  if( !sRobotStatsResetAllFunction ) {
    sRobotStatsResetAllFunction = [this](){ ResetAllStats(); };
  }
#endif
}

RobotStatsTracker::~RobotStatsTracker()
{
#if REMOTE_CONSOLE_ENABLED
  sRobotStatsResetAllFunction = nullptr;
#endif
}

void RobotStatsTracker::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _jdocsManager = &robot->GetComponent<JdocsManager>();

  // Call the JdocsManager to see if our robot stats jdoc file exists
  if (_jdocsManager->JdocNeedsCreation(external_interface::JdocType::ROBOT_LIFETIME_STATS))
  {
    LOG_INFO("RobotStatsTracker.InitDependent.NoStatsJdocsFile",
             "Stats jdocs file not found; creating it now");
    _jdocsManager->ClearJdocBody(external_interface::JdocType::ROBOT_LIFETIME_STATS);
    static const bool kSaveToDiskImmediately = true;
    UpdateStatsJdoc(kSaveToDiskImmediately);
  }
}

void RobotStatsTracker::IncreaseStimulationSeconds(float delta)
{
  if( delta > 0.0f ) {
    IncreaseHelper(kStimCategory, "StimSeconds", static_cast<uint64_t>(delta * kStimFixedPoint));
  }
}

void RobotStatsTracker::IncreaseStimulationCumulativePositiveDelta(float delta)
{
  if( delta > 0.0f ) {
    IncreaseHelper(kStimCategory, "CumlPosDelta", static_cast<uint64_t>(delta * kStimFixedPoint));
  }
}

void RobotStatsTracker::IncrementActiveFeature(const ActiveFeature& feature, const std::string& intentSource)
{
  IncreaseHelper(kActiveFeatureCategory + std::string(kRobotStatsSeparator) + intentSource,
                 ActiveFeatureToString(feature), 1);
}


void RobotStatsTracker::IncrementBehaviorStat(const BehaviorStat& stat)
{
  IncreaseHelper(kBehaviorStatCategory, BehaviorStatToString(stat), 1);
}

void RobotStatsTracker::IncrementNamedFacesPerDay()
{
  IncreaseHelper(kFacesCategory, "NamedFacePerDay", 1);
}

void RobotStatsTracker::IncreaseOdometer(float lWheelDelta_mm, float rWheelDelta_mm, float bodyDelta_mm)
{
  if( lWheelDelta_mm > 0.0f ) {
    IncreaseHelper(kOdomCategory, "LWheel", static_cast<uint64_t>(lWheelDelta_mm * kOdomFixedPoint));
  }
  if( rWheelDelta_mm > 0.0f ) {
    IncreaseHelper(kOdomCategory, "RWheel", static_cast<uint64_t>(rWheelDelta_mm * kOdomFixedPoint));
  }
  if( bodyDelta_mm > 0.0f ) {
    IncreaseHelper(kOdomCategory, "Body", static_cast<uint64_t>(bodyDelta_mm * kOdomFixedPoint));
  }
}

void RobotStatsTracker::IncreaseHelper(const std::string& prefix, const std::string& stat, uint64_t delta)
{
  const std::string key = prefix + kRobotStatsSeparator + stat;

  auto& statsJson = *_jdocsManager->GetJdocBodyPointer(external_interface::ROBOT_LIFETIME_STATS);
  if (!statsJson.isMember(key)) {
    statsJson[key] = 0;
    PRINT_CH_INFO("RobotStats", "RobotStatsTracker.Increase.NewStat",
                  "Beginning to track value '%s'",
                  key.c_str());
  }

  statsJson[key] = statsJson[key].asUInt64() + delta;

  _dirtyJdoc = true;
}

void RobotStatsTracker::UpdateDependent(const RobotCompMap& dependentComps)
{
  // VIC-5804  TODO:(bn) need to get a hook for cleanup so I can dump the file before shutdown / reboot
  
  // Update jdoc if there were change(s) this tick
  if (_dirtyJdoc)
  {
    static const bool kSaveToDiskImmediately = false;
    UpdateStatsJdoc(kSaveToDiskImmediately);
    _dirtyJdoc = false;
  }
}

bool RobotStatsTracker::UpdateStatsJdoc(const bool saveToDiskImmediately,
                                        const bool saveToCloudImmediately)
{
  static const Json::Value* jdocBodyPtr = nullptr;
  const bool success = _jdocsManager->UpdateJdoc(external_interface::JdocType::ROBOT_LIFETIME_STATS,
                                                 jdocBodyPtr,
                                                 saveToDiskImmediately,
                                                 saveToCloudImmediately);
  return success;
}

void RobotStatsTracker::ResetAllStats()
{
  PRINT_NAMED_WARNING("RobotStatsTracker.ResetAllStats", "Resetting and wiping all stats");
  _jdocsManager->ClearJdocBody(external_interface::JdocType::ROBOT_LIFETIME_STATS);
  static const bool kSaveToDiskImmediately = true;
  static const bool kSaveToCloudImmediately = true;
  UpdateStatsJdoc(kSaveToDiskImmediately, kSaveToCloudImmediately);
}

}
}

