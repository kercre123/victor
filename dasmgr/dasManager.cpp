/**
* File: victor/dasmgr/dasManager.cpp
*
* Description: DASManager class declarations
*
* Copyright: Anki, inc. 2018
*
*/

#include "dasManager.h"

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

//
// Victor CSV log helpers
//
// Note that format here must match the format used by VictorLogger!
// Optional numeric fields are parsed as strings because they may be omitted from the log record.
// See also: util/logging/victorLogger_vicos.cpp
//
//static constexpr const char * EVENT_FORMAT = "@%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s";

// How often do we process queued events? Counted by log records.
static constexpr int PROCESS_QUEUE_INTERVAL = 1000;

// How often do we process statistics? Counted by log records.
static constexpr int PROCESS_STATS_INTERVAL = 1000;

// Convert android log timestamp to Anki DAS equivalent
static inline uint64_t GetTimeStamp(const AndroidLogEntry& logEntry)
{
  return (logEntry.tv_sec * 1000) + (logEntry.tv_nsec / 1000000);
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
    strings.push_back(std::string(pos, end-pos));
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
  logEvent.source = tag;
  logEvent.profile_id = _profile_id;
  logEvent.feature_type = _feature_type;
  logEvent.feature_run_id = _feature_run_id;
  logEvent.seq = _seq++;
  logEvent.ts = GetTimeStamp(logEntry);
  logEvent.level = GetLogLevel(logEntry);
  logEvent.name = std::move(strings[0]);
  logEvent.s1 = std::move(strings[2]);
  logEvent.s2 = std::move(strings[3]);
  logEvent.s3 = std::move(strings[4]);
  logEvent.s4 = std::move(strings[5]);
  logEvent.i1 = std::atoll(strings[6].c_str());
  logEvent.i2 = std::atoll(strings[7].c_str());
  logEvent.i3 = std::atoll(strings[8].c_str());
  logEvent.i4 = std::atoll(strings[9].c_str());

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

//
// Process queued entries
void DASManager::ProcessQueue()
{
  if (!_eventQueue.empty()) {
    LOG_DEBUG("DASManager.ProcessQueue", "Processing %zu events", _eventQueue.size());
    _eventQueue.clear();
  }
}

//
// Log some process stats
void DASManager::ProcessStats()
{
  LOG_DEBUG("DASManager.ProcessStats", "%llu entries %llu events %llu sleeps", _entryCount, _eventCount, _sleepCount);

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
      _robot_id = "invalid";
    }
  }

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
