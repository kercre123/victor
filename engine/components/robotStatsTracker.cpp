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
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/components/jdocsManager.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/entityComponent/dependencyManagedEntity.h"

#define LOG_CHANNEL "RobotStatsTracker"

namespace Anki {
namespace Vector {

namespace {

static const char* kStimCategory = "Stim";
static const char* kActiveFeatureCategory = "Feature";
static const char* kActiveFeatureTypeCategory = "FeatureType";
static const char* kBehaviorStatCategory = "BStat";
static const char* kFacesCategory = "Face";
static const char* kOdomCategory = "Odom";
static const char* kLifetimeAliveCategory = "Alive";
static const char* kPettingDurationCategory = "Pet";

static const char* kRobotStatsSeparator = ".";

static const float kStimFixedPoint = 1000.0f;
static const float kOdomFixedPoint = 100000.0f;

#if REMOTE_CONSOLE_ENABLED
// hacks to allow console funcs to interact
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

static std::function<void(void)> sRobotStatsFlushFunction;

void FlushRobotStatsToDisk( ConsoleFunctionContextRef context )
{
  if( sRobotStatsFlushFunction ) {
    sRobotStatsFlushFunction();
  }
}


#endif // REMOTE_CONSOLE_ENABLED

}

#define CONSOLE_GROUP "RobotStats"

CONSOLE_VAR( f32, kRobotStats_AliveUpdatePeriod_s, CONSOLE_GROUP, 10.0f );

CONSOLE_FUNC( ResetRobotStats, CONSOLE_GROUP, const char* typeResetToConfirm );
CONSOLE_FUNC( FlushRobotStatsToDisk, CONSOLE_GROUP );

RobotStatsTracker::RobotStatsTracker()
  : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RobotStatsTracker)
  , UnreliableComponent<BCComponentID>(this, BCComponentID::RobotStatsTracker)
{
#if REMOTE_CONSOLE_ENABLED
  if( !sRobotStatsResetAllFunction ) {
    sRobotStatsResetAllFunction = [this](){ ResetAllStats(); };
  }
  if( !sRobotStatsFlushFunction ) {
    sRobotStatsFlushFunction = [this](){
      const bool saveToDiskImmediately = true;
      const bool saveToCloudImmediately = false;
      UpdateStatsJdoc(saveToDiskImmediately, saveToCloudImmediately);
    };
  }
#endif
}

RobotStatsTracker::~RobotStatsTracker()
{
#if REMOTE_CONSOLE_ENABLED
  sRobotStatsResetAllFunction = nullptr;
  sRobotStatsFlushFunction = nullptr;
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
  else
  {
    if (_jdocsManager->JdocNeedsMigration(external_interface::JdocType::ROBOT_LIFETIME_STATS))
    {
      DoJdocFormatMigration();
      static const bool kSaveToDiskImmediately = true;
      static const bool kSaveToCloudImmediately = false;
      UpdateStatsJdoc(kSaveToDiskImmediately, kSaveToCloudImmediately);
    }
  }

  _jdocsManager->RegisterFormatMigrationCallback(external_interface::JdocType::ROBOT_LIFETIME_STATS, [this]() {
    DoJdocFormatMigration();
  });

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeOfNextAliveTimeCheck = currTime_s + kRobotStats_AliveUpdatePeriod_s;
}

// hack for jira VIC-7922
float RobotStatsTracker::GetNumHoursAlive() const
{
  const char* key = "Alive.seconds";
  auto& statsJson = *_jdocsManager->GetJdocBodyPointer(external_interface::ROBOT_LIFETIME_STATS);
  if( statsJson.isMember(key) ) {
    const float sec = statsJson[key].asFloat();
    return sec / (3600.0f);
  }

  // not present, return 0
  return 0.0f;
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

  // also log usage by type, to serve as a "summary" of voice commands
  ActiveFeatureType featureType = GetActiveFeatureType(feature, ActiveFeatureType::Invalid);
  if( featureType != ActiveFeatureType::Invalid &&
      intentSource!= ActiveFeatureComponent::kIntentSourceAI) {
    IncreaseHelper( kActiveFeatureTypeCategory, ActiveFeatureTypeToString(featureType), 1);
  }
}

void RobotStatsTracker::IncrementPettingDuration(const float secondsPet)
{
  const int time_ms = (int)std::round(secondsPet * 1000.0f);
  IncreaseHelper(kPettingDurationCategory, "ms", time_ms);
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
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( currTime_s > _timeOfNextAliveTimeCheck ) {
    const auto secondsElapsed = static_cast<uint64_t>(kRobotStats_AliveUpdatePeriod_s);
    IncreaseHelper( kLifetimeAliveCategory, "seconds", secondsElapsed );
    _timeOfNextAliveTimeCheck += kRobotStats_AliveUpdatePeriod_s;
  }

  // Update jdoc if there were change(s) this tick
  // Note: In this case (robot lifetime stats), the call to UpdateStatsJdoc is very quick
  // because the body of the jdoc is owned by the JdocsManager, and we are not saving to
  // disk or submitting to cloud at this time
  if (_dirtyJdoc)
  {
    static const bool kSaveToDiskImmediately = false;
    UpdateStatsJdoc(kSaveToDiskImmediately);
    _dirtyJdoc = false;
  }
}


Json::Value RobotStatsTracker::FilterStatsForApp(const Json::Value& json)
{
  Json::Value jsonOut;

  for (Json::ValueConstIterator it = json.begin(); it != json.end(); ++it)
  {
    if (it.name() == "Stim.CumlPosDelta" ||
        it.name() == "BStat.ReactedToTriggerWord" ||
        it.name() == "Odom.Body" ||
        it.name() == "FeatureType.Utility" ||
        it.name() == "Pet.ms" ||
        it.name() == "Alive.seconds")
    {
      jsonOut[it.memberName()] = (*it);
    }
  }

  return jsonOut;
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


void RobotStatsTracker::DoJdocFormatMigration()
{
  const auto jdocType = external_interface::JdocType::ROBOT_LIFETIME_STATS;
  const auto docFormatVersion = _jdocsManager->GetJdocFmtVersion(jdocType);
  const auto curFormatVersion = _jdocsManager->GetCurFmtVersion(jdocType);
  LOG_INFO("RobotStatsTracker.DoJdocFormatMigration",
           "Migrating user entitlements jdoc from format version %llu to %llu",
           docFormatVersion, curFormatVersion);
  if (docFormatVersion > curFormatVersion)
  {
    LOG_ERROR("RobotStatsTracker.DoJdocFormatMigration.Error",
              "Jdoc format version is newer than what victor code can handle; no migration possible");
    return;
  }

  // When we change 'format version' on this jdoc, migration
  // to a newer format version is performed here
  
  // Note that with the RobotStatsTracker, the JdocsManager owns the body of the jdoc,
  // so any changes to the jdoc body should be done by first getting a pointer to the
  // body with _jdocsManager->GetJdocBodyPointer, and then manipulating the body json
  // directly

  // Now update the format version of this jdoc to the current format version
  _jdocsManager->SetJdocFmtVersionToCurrent(jdocType);
}


}
}

