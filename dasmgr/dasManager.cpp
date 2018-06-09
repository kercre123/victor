/**
* File: victor/dasmgr/dasManager.cpp
*
* Description: DASManager class declarations
*
* Copyright: Anki, inc. 2018
*
*/

#include "dasManager.h"
#include "DAS/DAS.h"

#include "osState/osState.h"

#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/string/stringUtils.h"

#include <log/logger.h>
#include <log/logprint.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <chrono>
#include <cstdlib>
#include <thread>

// Imported types
using LogLevel = Anki::Util::ILoggerProvider::LogLevel;

// Log options
#define LOG_CHANNEL "DASManager"

// Local constants

namespace {

  // Which DAS endpoint do we use?
  constexpr const char * DAS_URL = "https://sqs.us-west-2.amazonaws.com/792379844846/DasInternal-dasinternalSqs-1HN6JX3NZPGNT";

  // How many events do we buffer before send? Counted by events.
  constexpr const size_t QUEUE_THRESHOLD_SIZE = 256;

  // How often do we process queued events? Counted by log records.
  constexpr const int PROCESS_QUEUE_INTERVAL = 1000;

  // How often do we process statistics? Counted by log records.
  constexpr const int PROCESS_STATS_INTERVAL = 1000;
}

//
// Victor CSV log helpers
//
// Note that format here must match the format used by VictorLogger!
// Optional numeric fields are parsed as strings because they may be omitted from the log record.
// See also: util/logging/victorLogger_vicos.cpp
//
//static constexpr const char * EVENT_FORMAT = "@%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s";
//

// Convert android log timestamp to Anki DAS equivalent
static inline int64_t GetTimeStamp(const AndroidLogEntry& logEntry)
{
  const int64_t tv_sec = logEntry.tv_sec;
  const int64_t tv_nsec = logEntry.tv_nsec;
  const int64_t ts = (tv_sec * 1000) + (tv_nsec / 1000000);
  return ts;
}

// Convert android log level to Anki DAS equivalent
inline LogLevel GetLogLevel(const AndroidLogEntry& logEntry)
{
  switch (logEntry.priority)
  {
    case ANDROID_LOG_SILENT:
    case ANDROID_LOG_DEFAULT:
    case ANDROID_LOG_VERBOSE:
    case ANDROID_LOG_DEBUG:
      return LogLevel::LOG_LEVEL_DEBUG;
    case ANDROID_LOG_INFO:
      return LogLevel::LOG_LEVEL_INFO;
    case ANDROID_LOG_WARN:
      return LogLevel::LOG_LEVEL_WARN;
    case ANDROID_LOG_ERROR:
    case ANDROID_LOG_FATAL:
    case ANDROID_LOG_UNKNOWN:
      return LogLevel::LOG_LEVEL_ERROR;
  }

  // This shouldn't happen, but if it does it's an error
  assert(false);
  return LogLevel::LOG_LEVEL_ERROR;
}

namespace Anki {
namespace Victor {

// Process an event struct
void DASManager::ProcessEvent(LogEvent && logEvent)
{
  // Record global state
  _eventQueue.push_back(std::move(logEvent));
  ++_eventCount;

}

//
// Parse a log entry
bool DASManager::ParseLogEntry(const AndroidLogEntry & logEntry, LogEvent & logEvent)
{
  const char * tag = logEntry.tag;
  const char * message = logEntry.message;

  // These values are always set by library so we don't need to check them
  assert(tag != nullptr);
  assert(message != nullptr);

  // Anki::Util::StringSplit ignores trailing separator so don't use it
  std::vector<std::string> strings;
  const char * pos = message+1; // (skip leading event marker)
  while (1) {
    const char * end = strchr(pos, Anki::Util::DAS::FIELD_MARKER);
    if (end == nullptr) {
      strings.push_back(std::string(pos));
      break;
    }
    strings.push_back(std::string(pos, (size_t)(end-pos)));
    pos = end+1;
  }

  if (strings.size() != Anki::Util::DAS::FIELD_COUNT) {
    LOG_ERROR("DASManager.ParseLogEntry", "Unable to parse %s from %s (%zu != %d)",
              message, tag, strings.size(), Anki::Util::DAS::FIELD_COUNT);
    return false;
  }

  // If field count changes, we need to update this code
  static_assert(Anki::Util::DAS::FIELD_COUNT == 9, "DAS field count does not match declarations");

  // Populate event struct
  // Note that DAS uses SIGNED int64 for timestamp and sequence.
  // Unsigned values are coerced to match the spec.
  logEvent.seq = (int64_t) _seq++;
  logEvent.source = tag;
  logEvent.profile_id = _profile_id;
  logEvent.ts = GetTimeStamp(logEntry);
  logEvent.level = GetLogLevel(logEntry);
  logEvent.name = std::move(strings[0]);
  logEvent.s1 = std::move(strings[1]);
  logEvent.s2 = std::move(strings[2]);
  logEvent.s3 = std::move(strings[3]);
  logEvent.s4 = std::move(strings[4]);
  logEvent.i1 = std::atoll(strings[5].c_str());
  logEvent.i2 = std::atoll(strings[6].c_str());
  logEvent.i3 = std::atoll(strings[7].c_str());
  logEvent.i4 = std::atoll(strings[8].c_str());

  // Is this the start of a new feature?
  if (logEvent.name == DASMSG_FEATURE_START) {
    _feature_run_id = logEvent.s3;
    _feature_type = logEvent.s4;
  }

  logEvent.feature_run_id = _feature_run_id;
  logEvent.feature_type = _feature_type;

  return true;
}
//
// Process a log entry
//
void DASManager::ProcessLogEntry(const AndroidLogEntry & logEntry)
{
  const char * message = logEntry.message;
  const char * tag = logEntry.tag;
  assert(message != nullptr);
  assert(tag != nullptr);

  ++_entryCount;

  // Does this record look like a DAS entry?
  if (*message != Anki::Util::DAS::EVENT_MARKER) {
    return;
  }

  // Can we parse it?
  LogEvent logEvent;
  if (!ParseLogEntry(logEntry, logEvent)) {
    LOG_ERROR("DASManager.ProcessLogEntry", "Unable to parse event from tag %s", tag);
    return;
  }

  ProcessEvent(std::move(logEvent));
}

static inline void serialize(std::ostringstream & ostr, const std::string & key, const std::string & val)
{
  // TO DO: Guard against embedded quotes in value
  ostr << '"' << key << '"';
  ostr << ':';
  ostr << '"' << val << '"';
}

static inline void serialize(std::ostringstream & ostr, const std::string & key, int64_t val)
{
  ostr << '"' << key << '"';
  ostr << ':';
  ostr << val;
}

static inline void serialize(std::ostringstream & ostr, const std::string & key, LogLevel val)
{
  ostr << '"' << key << '"';
  ostr << ':';
  ostr << '"';
  switch (val)
  {
    case LogLevel::LOG_LEVEL_ERROR:
      ostr << "error";
      break;
    case LogLevel::LOG_LEVEL_WARN:
      ostr << "warning";
      break;
    case LogLevel::LOG_LEVEL_EVENT:
      ostr << "event";
      break;
    case LogLevel::LOG_LEVEL_INFO:
      ostr << "info";
      break;
    case LogLevel::LOG_LEVEL_DEBUG:
      ostr << "debug";
      break;
    case LogLevel::_LOG_LEVEL_COUNT:
      ostr << "count";
      break;
  }
  ostr << '"';
}


void DASManager::Serialize(std::ostringstream & ostr, const LogEvent & event)
{
  //
  // TO DO: robot_id, boot_id, and robot_version are constant for lifetime of the service.
  // Feature_run_id and feature_type only change is response to specific events.
  // These fields could be preformatted for efficiency.
  //
  ostr << '{';
  serialize(ostr, "source", event.source);
  ostr << ',';
  serialize(ostr, "event", event.name);
  ostr << ',';
  serialize(ostr, "ts", event.ts);
  ostr << ',';
  serialize(ostr, "seq", event.seq);
  ostr << ',';
  serialize(ostr, "level", event.level);
  ostr << ',';
  serialize(ostr, "robot_id", _robot_id);
  ostr << ',';
  serialize(ostr, "robot_version", _robot_version);
  ostr << ',';
  serialize(ostr, "boot_id", _boot_id);
  ostr << ',';
  serialize(ostr, "profile_id", event.profile_id);
  ostr << ',';
  serialize(ostr, "feature_type", event.feature_type);
  ostr << ',';
  serialize(ostr, "feature_run_id", event.feature_run_id);

  // S1-S4 fields are OPTIONAL and may be empty.
  // If they are empty, there's no need to send them.
  // DAS ingestion does not distinguish between empty string and null.
  if (!event.s1.empty()) {
    ostr << ',';
    serialize(ostr, "s1", event.s1);
  }
  if (!event.s2.empty()) {
    ostr << ',';
    serialize(ostr, "s2", event.s2);
  }
  if (!event.s3.empty()) {
    ostr << ',';
    serialize(ostr, "s3", event.s3);
  }
  if (!event.s4.empty()) {
    ostr << ',';
    serialize(ostr, "s4", event.s4);
  }


  // I1-I4 fields are OPTIONAL and may be empty.
  // If they are empty, there's no need to send them.
  // DAS ingestion does not distinguish between 0 and null.
  if (event.i1 != 0) {
    ostr << ',';
    serialize(ostr, "i1", event.i1);
  }
  if (event.i2 != 0) {
    ostr << ',';
    serialize(ostr, "i2", event.i2);
  }
  if (event.i3 != 0) {
    ostr << ',';
    serialize(ostr, "i3", event.i3);
  }
  if (event.i4 != 0) {
    ostr << ',';
    serialize(ostr, "i4", event.i4);
  }
  ostr << '}';
}

void DASManager::Serialize(std::ostringstream & ostr, const EventQueue & eventQueue)
{
  ostr << '[';
  for (const auto & event : eventQueue) {
      Serialize(ostr, event);
      ostr << ',';
  }
  // Remove trailing separator?
  if (!eventQueue.empty()) {
    ostr.seekp(-1, std::ostringstream::cur);
  }
  ostr << ']';

}
//
// Process queued entries
void DASManager::ProcessQueue()
{
  const size_t nEvents = _eventQueue.size();
  if (nEvents < QUEUE_THRESHOLD_SIZE) {
    // Keep filling current buffer
    return;
  }

  LOG_DEBUG("DASManager.ProcessQueue", "Post %zu events to %s", nEvents, DAS_URL);

  std::ostringstream ostr;
  Serialize(ostr, _eventQueue);

  std::string response;
  const bool success = DAS::PostToServer(DAS_URL, ostr.str(), response);
  if (success) {
    ++_uploadSuccessCount;
  } else {
    // TO DO: Retry after fail? Write to backing store?
    LOG_ERROR("DASManager.ProcessQueue", "Failed to upload %zu events (%s)", nEvents, response.c_str());
    ++_uploadFailCount;
  }

  _eventQueue.clear();
}

//
// Log some process stats
void DASManager::ProcessStats()
{
  LOG_DEBUG("DASManager.ProcessStats",
            "%llu entries %llu events %llu sleeps %llu uploadSuccess %llu uploadFail",
            _entryCount, _eventCount, _sleepCount, _uploadSuccessCount, _uploadFailCount);

  static struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
    /* maxrss = maximum resident set size */
    /* ixrss = integral shared memory size */
    /* idrss = integral unshared data size */
    /* isrss = integral unshared stack size */
    LOG_DEBUG("DASManager.ProcessStats",
      "maxrss=%ld ixrss=%ld idrss=%ld isrss=%ld",
      ru.ru_maxrss, ru.ru_ixrss, ru.ru_idrss, ru.ru_isrss);
  }
}

//
// Process log entries until shutdown flag becomes true.
// The shutdown flag is declared const because this function
// doesn't change it, but it may be changed by an asynchronous event
// such as a signal handler.
//
Result DASManager::Run(const bool & shutdown)
{
  Result result = RESULT_OK;

  LOG_DEBUG("DASManager.Run", "Start");

  {
    auto * osState = Anki::Cozmo::OSState::getInstance();
    DEV_ASSERT(osState != nullptr, "DASManager.Run.InvalidOSState");
    if (osState->HasValidEMR()) {
      _robot_id = osState->GetSerialNumberAsString();
    } else {
      LOG_ERROR("DASManager.Run.InvalidEMR", "INVALID EMR - NO ESN");
    }
    _robot_version = osState->GetOSBuildVersion() + "@" + osState->GetBuildSha();
    Cozmo::OSState::removeInstance();
  }

  // TO DO VIC-2925: Track profile_id, feature_type, _feature_run_id
  _boot_id = Anki::Util::GetUUIDString();
  _profile_id = "system";
  _feature_type = "system";
  _feature_run_id = _boot_id;

  //
  // Android log API is documented here:
  // https://android.googlesource.com/platform/system/core/+/master/liblog/README
  //

  // Open the log buffer
  struct logger_list * log = android_logger_list_open(LOG_ID_MAIN, ANDROID_LOG_RDONLY, 0, 0);
  if (log == nullptr) {
    // If we can't open the log buffer, return an appropriate error code.
    LOG_ERROR("DASManager.Run", "Unable to open android logger (errno %d)", errno);
    return RESULT_FAIL_FILE_OPEN;
  }

  //
  // Read log records until shutdown flag becomes true.
  //
  while (!shutdown) {
    struct log_msg logmsg;
    int rc = android_logger_list_read(log, &logmsg);
    if (rc == -EAGAIN) {
      // Nothing to read at this time. Sleep and try again.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      ++_sleepCount;
      continue;
    }

    if (rc <= 0 ) {
      // If we can't read the log buffer, return an appropriate error.
      LOG_ERROR("DASManager.Run", "Log read error %d (%s)", rc, strerror(errno));
      result = RESULT_FAIL_FILE_READ;
      break;
    }

    AndroidLogEntry logEntry;
    rc = android_log_processLogBuffer(&logmsg.entry_v1, &logEntry);
    if (rc != 0) {
      // Malformed log entry? Report the problem but keep reading.
      LOG_ERROR("DASManager.Run", "Unable to process log buffer (error %d)", rc);
      continue;
    }

    // Dispose of this log entry
    ProcessLogEntry(logEntry);

    // Process queue at regular intervals
    if (_entryCount % PROCESS_QUEUE_INTERVAL == 0) {
      ProcessQueue();
    }

    // Print stats at regular intervals
    if (_entryCount % PROCESS_STATS_INTERVAL == 0) {
      ProcessStats();
    }

  }

  // Clean up
  LOG_DEBUG("DASManager.Run", "Stop (result %d)", result);
  android_logger_list_close(log);

  return result;
}

} // end namespace Victor
} // end namespace Anki
