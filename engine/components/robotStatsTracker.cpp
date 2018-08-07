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
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

namespace {

static const char* kStatsFolder = "robotStats";
static const char* kJsonStatsFilename = "stats.json";

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
CONSOLE_VAR_RANGED( f32, kRobotStatsWritePeriod_s, CONSOLE_GROUP, 60.0f, 0.01f, 300.0f );

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
  const auto& context = dependentComps.GetComponent<ContextWrapper>().context;
  const auto* platform = context->GetDataPlatform();

  const std::string& folder = platform->pathToResource( Util::Data::Scope::Persistent, kStatsFolder );
  if( Util::FileUtils::DirectoryDoesNotExist( folder ) ) {
    Util::FileUtils::CreateDirectory( folder );
  }

  _filename = Util::FileUtils::AddTrailingFileSeparator( folder ) + kJsonStatsFilename;

  // if file exists, attempt to read it
  if( Util::FileUtils::FileExists(_filename) ) {
    ReadStatsFromFile();
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
  std::string key = prefix + kRobotStatsSeparator + stat;
  
  auto it = _stats.find(key);
  if( it == _stats.end() ) {
    it = _stats.emplace(key, 0.0).first;
    PRINT_CH_INFO("RobotStats", "RobotStatsTracker.Increase.NewStat",
                  "Beginning to track value '%s'",
                  key.c_str());
  }

  it->second += delta;

  _dirtySinceWrite = true;
}

void RobotStatsTracker::UpdateDependent(const RobotCompMap& dependentComps)
{
  // TODO:(bn) need to get a hook for cleanup so I can dump the file before shutdown / reboot
  
  // check if it's time to write to disk (and we have any new data to write)
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _dirtySinceWrite &&
      (_lastFileWriteTime_s + kRobotStatsWritePeriod_s <= currTime_s ) ) {
    WriteStatsToFile();
  }
}

void RobotStatsTracker::WriteStatsToFile()
{
  if( !_filename.empty()) {
    Json::Value toSave = Json::Value(Json::objectValue);
    
    for( const auto& stat : _stats ) {
      toSave[stat.first] = stat.second;
    }
    
    Util::FileUtils::WriteFile( _filename, toSave.toStyledString() );
  }
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastFileWriteTime_s = currTime_s;
  _dirtySinceWrite = false;
}

void RobotStatsTracker::ReadStatsFromFile()
{
  Json::Value data;
  Json::Reader dataReader;
  dataReader.parse(Util::FileUtils::ReadFile(_filename), data);

  if( ANKI_VERIFY( data.isObject(), "RobotStatsTracker.SavedJsonInvalid",
                   "must be an object" ) ) {
    for( auto it = data.begin();
         it != data.end();
         ++it ) {
      if( ANKI_VERIFY( it.key().isString() &&
                       it->isUInt64(),
                       "RobotStatsTracker.SavedJsonEntryInvalid",
                       "json error" ) ) {
        const std::string& key = it.key().asString();
        uint64_t val = it->asUInt64();
        
        _stats.emplace( key, val );

        PRINT_CH_INFO("RobotStats", "RobotStatsTracker.Init.Load",
                      "Loaded value '%s' = %llu",
                      key.c_str(),
                      val);
      }
    }
  }
}

void RobotStatsTracker::ResetAllStats()
{
  PRINT_NAMED_WARNING("RobotStatsTracker.ResetAllStats", "Resetting and wiping all stats");
  _stats.clear();
  WriteStatsToFile();
}

}
}

