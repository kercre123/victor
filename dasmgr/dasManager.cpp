/**
* File: victor/dasmgr/dasManager.cpp
*
* Description: DASManager class declarations
*
* Copyright: Anki, inc. 2018
*
*/

#include "dasManager.h"
#include "dasConfig.h"

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
using LogLevel = Anki::Util::LogLevel;

// Log options
#define LOG_CHANNEL "DASManager"

// Local constants

namespace {



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

namespace Anki {
namespace Victor {

DASManager::DASManager(const DASConfig & dasConfig)
{
  _url = dasConfig.GetURL();
  _queue_threshold_size = dasConfig.GetQueueThresholdSize();
  _max_deferrals_size = dasConfig.GetMaxDeferralsSize();
}

void DASManager::Serialize(std::ostringstream & ostr, const DASEvent & event)
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

void DASManager::Serialize(std::ostringstream & ostr, const DASEventQueue & eventQueue)
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
// Attempt to upload queued events.
// This is called on the worker thread.
// Return true on successful upload.
//
bool DASManager::PostToServer(const DASEventQueue & eventQueue)
{
  const size_t nEvents = eventQueue.size();

  LOG_DEBUG("DASManager.PostToServer", "Upload %zu events to %s", nEvents, _url.c_str());

  std::ostringstream ostr;
  std::string response;

  Serialize(ostr, eventQueue);

  const bool success = DAS::PostToServer(_url, ostr.str(), response);
  if (success) {
    LOG_DEBUG("DASManager.PostToServer.UploadSuccess", "Uploaded %zu events", nEvents);
    ++_workerSuccessCount;
    DASMSG(dasmgr_upload_success, "dasmgr.upload.success", "Sent after each successful upload");
    DASMSG_SET(i1, _workerSuccessCount, "Worker success count");
    DASMSG_SET(i2, _workerFailCount, "Worker fail count");
    DASMSG_SET(i3, _workerDeferralCount, "Worker deferral count");
    DASMSG_SET(i4, _workerDroppedCount, "Worker dropped count");
    DASMSG_SEND();
    return true;
  } else {
    LOG_ERROR("DASManager.PostToServer.UploadFailed", "Failed to upload %zu events (%s)", nEvents, response.c_str());
    ++_workerFailCount;
    DASMSG(dasmgr_upload_failed, "dasmgr.upload.failed", "Sent after each failed upload");
    DASMSG_SET(s1, response, "HTTP response");
    DASMSG_SET(i1, _workerSuccessCount, "Worker success count");
    DASMSG_SET(i2, _workerFailCount, "Worker fail count");
    DASMSG_SET(i3, _workerDeferralCount, "Worker deferral count");
    DASMSG_SET(i4, _workerDroppedCount, "Worker dropped count");
    DASMSG_SEND();
    return false;
  }
}

//
// Process queue of deferred events until empty or failure.
// In case of failure, events are left at front of queue.
//
void DASManager::PerformDeferredUploads()
{
  while (!_workerDeferrals.empty()) {
    const auto front = _workerDeferrals.front();
    const bool success = PostToServer(*front);
    if (success) {
      LOG_DEBUG("DASManager.PerformDeferredUploads.UploadSuccess", "Uploaded %zu events", front->size());
      _workerDeferrals.pop_front();
    } else {
      LOG_DEBUG("DASManager.PerformDeferredUploads.UploadFailed", "Failed to upload %zu events", front->size());
      break;
    }
  }
}

//
// Move event queue into deferral queue.  If deferral queue is at capacity,
// discard the oldest data.
//
void DASManager::PerformDeferral(DASEventQueuePtr eventQueue)
{
  // Preconditions
  DEV_ASSERT(_workerDeferrals.size() <= _max_deferrals_size, "DASManager.PerformDeferral.InvalidDeferralQueue");

  if (_workerDeferrals.size() >= _max_deferrals_size) {
    LOG_ERROR("DASManager.PerformDeferral", "Dropping %zu deferred events", _workerDeferrals.front()->size());
    _workerDeferrals.pop_front();
    ++_workerDroppedCount;
  }
  _workerDeferrals.push_back(eventQueue);
  ++_workerDeferralCount;
}

//
// Upload events to DAS endpoint
// This is called on background worker thread to avoid blocking main read loop.
//
void DASManager::PerformUpload(DASEventQueuePtr eventQueue)
{
  // Preconditions
  DEV_ASSERT(eventQueue != nullptr, "DASManager.PerformUpload.InvalidQueue");
  DEV_ASSERT(eventQueue->size() > 0, "DASManager.PerformUpload.InvalidQueueSize");

  // Attempt to post event queue to server
  const bool success = PostToServer(*eventQueue);
  if (success) {
    // Attempt any deferred uploads
    PerformDeferredUploads();
  } else {
    // Try again later
    PerformDeferral(eventQueue);
  }

}

void DASManager::EnqueueForUpload(DASEventQueuePtr eventQueue)
{
  // Preconditions
  DEV_ASSERT(eventQueue != nullptr, "DASManager.EnqueueForUpload.InvalidQueue");

  LOG_DEBUG("DASManager.EnqueueForUpload", "Enqueue %zu events for upload", eventQueue->size());

  auto task = [this, eventQueue]() {
    PerformUpload(eventQueue);
  };

  _worker.Wake(task, "EnqueueForUpload");
}

// Process an event struct
void DASManager::ProcessEvent(DASEvent && event)
{
  // Preconditions
  DEV_ASSERT(_eventQueue != nullptr, "DASManager.ProcessEvent.InvalidQueue");

  _eventQueue->push_back(std::move(event));
  ++_eventCount;

  // If we have reached threshold size, move active queue to worker thread and start a new queue
  if (_eventQueue->size() >= _queue_threshold_size) {
    EnqueueForUpload(_eventQueue);
    _eventQueue = std::make_shared<DASEventQueue>();
  }
}

//
// Parse a log entry
bool DASManager::ParseLogEntry(const AndroidLogEntry & logEntry, DASEvent & event)
{
  const char * tag = logEntry.tag;
  const char * message = logEntry.message;

  // These values are always set by library so we don't need to check them
  DEV_ASSERT(tag != nullptr, "DASManager.ParseLogEntry.InvalidTag");
  DEV_ASSERT(message != nullptr, "DASManager.ParseLogEntry.InvalidMessage");

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
  event.seq = (int64_t) _seq++;
  event.source = tag;
  event.profile_id = _profile_id;
  event.ts = GetTimeStamp(logEntry);
  event.level = GetLogLevel(logEntry);
  event.name = std::move(strings[0]);
  event.s1 = std::move(strings[1]);
  event.s2 = std::move(strings[2]);
  event.s3 = std::move(strings[3]);
  event.s4 = std::move(strings[4]);
  event.i1 = std::atoll(strings[5].c_str());
  event.i2 = std::atoll(strings[6].c_str());
  event.i3 = std::atoll(strings[7].c_str());
  event.i4 = std::atoll(strings[8].c_str());

  // Is this the start of a new feature?
  if (event.name == DASMSG_FEATURE_START) {
    _feature_run_id = event.s3;
    _feature_type = event.s4;
  }

  event.feature_run_id = _feature_run_id;
  event.feature_type = _feature_type;

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
  DASEvent event;
  if (!ParseLogEntry(logEntry, event)) {
    LOG_ERROR("DASManager.ProcessLogEntry", "Unable to parse event from tag %s", tag);
    return;
  }

  ProcessEvent(std::move(event));
}


//
// Log some process stats
//
void DASManager::ProcessStats()
{
  LOG_DEBUG("DASManager.ProcessStats.QueueStats",
            "entries=%llu events=%llu sleeps=%llu "
            "workerSuccess=%u workerFail=%u workerDeferral=%u workerDropped=%u",
            _entryCount, _eventCount, _sleepCount,
            (uint32_t) _workerSuccessCount,
            (uint32_t) _workerFailCount,
            (uint32_t) _workerDeferralCount,
            (uint32_t) _workerDroppedCount);

  static struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) == 0) {
    /* maxrss = maximum resident set size */
    /* ixrss = integral shared memory size */
    /* idrss = integral unshared data size */
    /* isrss = integral unshared stack size */
    LOG_DEBUG("DASManager.ProcessStats.MemoryStats",
      "maxrss=%ld ixrss=%ld idrss=%ld isrss=%ld",
      ru.ru_maxrss, ru.ru_ixrss, ru.ru_idrss, ru.ru_isrss);
  }
}

//
// Enqueue any outstanding events
//
void DASManager::FlushEventQueue()
{
  DEV_ASSERT(_eventQueue != nullptr, "DASManager.FlushEventQueue.InvalidQueue");
  const auto nEvents = _eventQueue->size();
  if (nEvents > 0) {
    LOG_DEBUG("DASManager.FlushEventQueue", "Flush %zu pending events", nEvents);
    EnqueueForUpload(_eventQueue);
    _eventQueue.reset();
  }
}

//
// Attempt to complete any pending uploads
//
void DASManager::FlushUploadQueue()
{
  //
  // Create a task to block until pending uploads have been processed.
  // TO DO: How can we put a time bound on this?
  //
  LOG_DEBUG("DASManager.FlushUploadQueue", "Waiting for uploads to complete");

  auto task = []() {
    // Task is blocked until it reaches front of queue
    LOG_DEBUG("DASManager.FlushUploadQueue", "Upload queue is clear");
  };

  _worker.WakeSync(task, "FlushUploadQueue");
}
//
// Process log entries until shutdown flag becomes true.
// The shutdown flag is declared const because this function
// doesn't change it, but it may be changed by an asynchronous event
// such as a signal handler.
//
Result DASManager::Run(const bool & shutdown)
{
  // Validate configuration
  if (_url.empty()) {
    LOG_ERROR("DASManager.Run.InvalidURL", "Invalid URL");
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  LOG_DEBUG("DASManager.Run", "Begin reading loop");

  {
    auto * osState = Anki::Cozmo::OSState::getInstance();
    DEV_ASSERT(osState != nullptr, "DASManager.Run.InvalidOSState");
    if (osState->HasValidEMR()) {
      _robot_id = osState->GetSerialNumberAsString();
    } else {
      LOG_ERROR("DASManager.Run.InvalidEMR", "INVALID EMR - NO ESN");
    }
    _robot_version = osState->GetOSBuildVersion() + "@" + osState->GetBuildSha();
    _boot_id = osState->GetBootID();
    Cozmo::OSState::removeInstance();
  }

  _profile_id = "system";
  _feature_type = "system";
  _feature_run_id = Anki::Util::GetUUIDString();

  // Log global parameters so we can match up log data with DAS records
  LOG_INFO("DASManager.Run", "robot_id=%s robot_version=%s boot_id=%s feature_run_id=%s",
           _robot_id.c_str(), _robot_version.c_str(), _boot_id.c_str(), _feature_run_id.c_str());

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
  Result result = RESULT_OK;

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

    // Print stats at regular intervals
    if (_entryCount % PROCESS_STATS_INTERVAL == 0) {
      ProcessStats();
    }

  }

  LOG_DEBUG("DASManager.Run", "End reading loop");

  // Clean up
  LOG_DEBUG("DASManager.Run", "Cleaning up");
  android_logger_list_close(log);
  FlushEventQueue();
  FlushUploadQueue();

  // Report final stats
  ProcessStats();

  LOG_DEBUG("DASManager.Run", "Done(result %d)", result);
  return result;
}

} // end namespace Victor
} // end namespace Anki
