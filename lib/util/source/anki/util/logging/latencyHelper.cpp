/***********************************************************************************************************************
 *
 *  LatencyHelper
 *  Util
 *
 *  Created by Jarrod Hatfield on 01/28/19.
 *
 **********************************************************************************************************************/

#include "util/logging/latencyHelper.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "json/json.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>


#define LOG_CHANNEL "CpuProfiler"

namespace Anki {
namespace Util {


  // = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
  class TimeStamper
  {
  public:

    TimeStamper();
    void Init( const std::string& filepath );

    void BeginInterval( const std::string& id, size_t tick );
    void RecordTimeStep( const std::string& id, size_t tick );
    void EndInterval( const std::string& id, size_t tick );

    void ResetCurrentInterval();


  private:

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Type definitions ...

    using ClockType         = std::chrono::steady_clock;
    using TimePoint         = ClockType::time_point;

    struct TimeStepNode
    {
      TimeStepNode() = delete;
      TimeStepNode( const std::string&, TimePoint, size_t, uint8_t );

      std::string     name;
      TimePoint       timestamp;
      size_t          tick;
      uint8_t         threadId;
    };

    using TimeStepInterval  = std::vector<TimeStepNode>;
    using IntervalList      = std::vector<TimeStepInterval>;


    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Private Functions / Helpers ...

    void IncrementInterval();
    TimeStepInterval& GetCurrentInterval() { return _intervals.back(); }

    void SaveIntervals() const;
    void LoadIntervals();
    void LogAllIntervals() const;


    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Members ...

    mutable std::recursive_mutex  _dataMutex;
    IntervalList                  _intervals;
    std::atomic<bool>             _isRecording;
    std::string                   _filename;

    std::unordered_map<std::thread::id, uint8_t> _thread_map;
  };

static TimeStamper sLatencyLogger;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamper::TimeStepNode::TimeStepNode( const std::string& idString, TimePoint currentTime, size_t tickCount, uint8_t thread ) :
  name( idString ),
  timestamp( currentTime ),
  tick( tickCount ),
  threadId( thread )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamper::TimeStamper() :
  _isRecording( false )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::Init( const std::string& filepath  )
{
  _filename = filepath;

  _intervals.clear();
  _thread_map.clear();

  _isRecording = false;
  LoadIntervals();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::BeginInterval()
{
  if ( !_isRecording )
  {
    IncrementInterval();
    _isRecording = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::RecordTimeStep( const std::string& id, size_t tick )
{
  if ( _isRecording )
  {
    std::lock_guard<std::recursive_mutex> lock( _dataMutex );
    const TimePoint now = ClockType::now();

    const std::thread::id this_id = std::this_thread::get_id();
    const auto it = _thread_map.find( this_id );
    if ( it == _thread_map.end() )
    {
      _thread_map[this_id] = _thread_map.size();
    }

    const uint8_t thread_id = _thread_map[this_id];
    GetCurrentInterval().emplace_back( id, now, tick, thread_id );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::EndInterval()
{
  _isRecording = false;
  SaveIntervals();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::IncrementInterval()
{
  if ( !_isRecording )
  {
    std::lock_guard<std::recursive_mutex> lock( _dataMutex );
    _intervals.push_back( TimeStepInterval() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::LogAllIntervals() const
{
  using namespace std::chrono;

  int intervalId = 0;
  for ( const auto& interval : _intervals )
  {
    if ( !interval.empty() )
    {
      PRINT_NAMED_WARNING( "[WakewordLatency]", "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -");
      for ( const auto& step : interval )
      {
        PRINT_NAMED_WARNING( "[WakewordLatency]", "[%d] - [%d] [%d] [%dms] %s",
                             intervalId,
                             (int)step.threadId,
                             (int)step.tick,
                             (int)duration_cast<milliseconds>(step.timestamp.time_since_epoch()).count(),
                             step.name.c_str() );
      }
    }

    ++intervalId;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::SaveIntervals() const
{
  using namespace std::chrono;

  std::lock_guard<std::recursive_mutex> lock( _dataMutex );
  Json::Value intervalArray( Json::ValueType::arrayValue );

  for ( const auto& interval : _intervals )
  {
    Json::Value stepArray( Json::ValueType::arrayValue );

    for ( const auto& step : interval )
    {
      //static_cast<Json::UInt>

      Json::Value stepMessage;
      stepMessage["id"]       = step.name;
      stepMessage["time"]     = duration_cast<milliseconds>(step.timestamp.time_since_epoch()).count();
      stepMessage["tick"]     = step.tick;
      stepMessage["thread"]   = step.threadId;

      stepArray.append( stepMessage );
    }

    intervalArray.append( stepArray );
  }

  Util::FileUtils::WriteFile( _filename, intervalArray.toStyledString() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::LoadIntervals()
{
  Json::Reader reader;
  Json::Value intervalArray;

  // if the file exists, we should have some content
  const std::string fileContents = Util::FileUtils::ReadFile( _filename );
  if ( !fileContents.empty() && reader.parse( fileContents, intervalArray ) )
  {
    PRINT_NAMED_WARNING( "[WakewordLatency]", "Loading interval");

    for ( const Json::Value& stepArray : intervalArray )
    {
      IncrementInterval();

      for ( const Json::Value& stepMessage : stepArray )
      {
        const std::string id  = stepMessage["id"].asString();
        const TimePoint time( std::chrono::milliseconds(stepMessage["time"].asUInt()) );
        const size_t tick     = stepMessage["tick"].asUInt();
        uint8_t thread        = stepMessage["thread"].asUInt();
        GetCurrentInterval().emplace_back( id, time, tick, thread );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeStamper::ResetCurrentInterval()
{
  if ( _isRecording )
  {
    std::lock_guard<std::recursive_mutex> lock( _dataMutex );
    _intervals.pop_back();
    _isRecording = false;
  }
}


// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
namespace TimeStampedIntervals {

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void InitLatencyIntervals( const std::string& uniqueId )
  {
    sLatencyLogger.Init( uniqueId );
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void BeginInterval()
  {
    sLatencyLogger.BeginInterval();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void RecordTimeStep( const std::string& idString, size_t tick )
  {
    sLatencyLogger.RecordTimeStep( idString, tick );
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void EndInterval()
  {
    sLatencyLogger.EndInterval();
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void ResetCurrentInterval()
  {
    sLatencyLogger.ResetCurrentInterval();
  }


} // namespace LatencyDebug

} // namespace Util
} // namespace Anki
