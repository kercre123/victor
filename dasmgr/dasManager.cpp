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
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"
#include "util/string/stringUtils.h"

#include <log/logger.h>
#include <log/logprint.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <algorithm>
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

static inline void serialize(std::ostream & ostr, const std::string & key, const std::string & val)
{
  // TO DO: Guard against embedded quotes in value
  ostr << '"' << key << '"';
  ostr << ':';
  ostr << '"' << val << '"';
}

static inline void serialize(std::ostream & ostr, const std::string & key, int64_t val)
{
  ostr << '"' << key << '"';
  ostr << ':';
  ostr << val;
}

static inline void serialize(std::ostream & ostr, const std::string & key, LogLevel val)
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
  _dasConfig = dasConfig;
  _logFilePath = Util::FileUtils::FullFilePath({_dasConfig.GetStoragePath(), "das.log"});
}

//
// Attempt to upload a JSON log file
// This is called on the worker thread.
// Return true on successful upload.
//
bool DASManager::PostToServer(const std::string& pathToLogFile)
{
  if (_exiting) {
    return false;
  }
  std::string json = Util::FileUtils::ReadFile(pathToLogFile);
  if (json.empty()) {
    return true;
  }
  std::string response;

  const bool success = DAS::PostToServer(_dasConfig.GetURL(), json, response);
  if (success) {
    LOG_DEBUG("DASManager.PostToServer.UploadSuccess", "Uploaded json of length %zu", json.length());
    ++_workerSuccessCount;
    if (!(_workerSuccessCount % 10)) {
      DASMSG(dasmgr_upload_success, "dasmgr.upload.stats", "Sent after every 10 successful uploads");
      DASMSG_SET(i1, _workerSuccessCount, "Worker success count");
      DASMSG_SET(i2, _workerFailCount, "Worker fail count");
      DASMSG_SET(i3, _workerDroppedCount, "Worker dropped count");
      DASMSG_SEND();
    }
    return true;
  } else {
    LOG_ERROR("DASManager.PostToServer.UploadFailed", "Failed to upload json of length %zu", json.length());
    ++_workerFailCount;
    DASMSG(dasmgr_upload_failed, "dasmgr.upload.failed", "Sent after each failed upload");
    DASMSG_SET(s1, response, "HTTP response");
    DASMSG_SET(i1, _workerSuccessCount, "Worker success count");
    DASMSG_SET(i2, _workerFailCount, "Worker fail count");
    DASMSG_SET(i3, _workerDroppedCount, "Worker dropped count");
    DASMSG_SEND();
    return false;
  }
}

void DASManager::PostLogsToServer()
{
  std::vector<std::string> directories = {_dasConfig.GetStoragePath(), _dasConfig.GetBackupPath()};

  for (auto const& dir : directories) {
    std::vector<std::string> jsonFiles = GetJsonFiles(dir);
    for (auto const& jsonFile : jsonFiles) {
      bool result = PostToServer(jsonFile);
      if (result) {
        Util::FileUtils::DeleteFile(jsonFile);
      } else {
        return;
      }
    }
  }
}

void DASManager::BackupLogFiles()
{
  std::vector<std::string> jsonFiles = GetJsonFiles(_dasConfig.GetStoragePath());
  for (auto const& jsonFile : jsonFiles) {
    // Create the directory that will hold the json
    if (!Util::FileUtils::CreateDirectory(_dasConfig.GetBackupPath(), false, true)) {
      LOG_ERROR("DASManager.BackupLogFiles.CreateBackupDir",
                "Failed to create Backup Path");
      return;
    }
    if (Util::FileUtils::GetDirectorySize(_dasConfig.GetBackupPath()) > (ssize_t) _dasConfig.GetBackupQuota()) {
      LOG_INFO("DASManager.BackupLogFiles.QuotaExceeded", "Exceeded quota for %s",
               _dasConfig.GetBackupPath().c_str());
      return;
    }
    LOG_DEBUG("DASManager.BackupLogFiles.MovingFile", "Moving %s into to %s/",
              jsonFile.c_str(), _dasConfig.GetBackupPath().c_str());
    (void) Util::FileUtils::MoveFile(_dasConfig.GetBackupPath(), jsonFile);
  }
}

std::string DASManager::ConvertLogEntryToJson(const AndroidLogEntry & logEntry)
{
  // These values are always set by library so we don't need to check them
  DEV_ASSERT(logEntry.tag != nullptr, "DASManager.ParseLogEntry.InvalidTag");
  DEV_ASSERT(logEntry.message != nullptr, "DASManager.ParseLogEntry.InvalidMessage");

  // Anki::Util::StringSplit ignores trailing separator so don't use it
  std::vector<std::string> values;
  const char * pos = logEntry.message+1; // (skip leading event marker)
  while (1) {
    const char * end = strchr(pos, Anki::Util::DAS::FIELD_MARKER);
    if (end == nullptr) {
      values.push_back(std::string(pos));
      break;
    }
    values.push_back(std::string(pos, (size_t)(end-pos)));
    pos = end+1;
  }

  if (values.size() != Anki::Util::DAS::FIELD_COUNT) {
    LOG_ERROR("DASManager.ConvertLogEntry", "Unable to parse %s from %s (%zu != %d)",
              logEntry.message, logEntry.tag, values.size(), Anki::Util::DAS::FIELD_COUNT);
    return "";
  }

  // If field count changes, we need to update this code
  static_assert(Anki::Util::DAS::FIELD_COUNT == 9, "DAS field count does not match declarations");

  std::string name = values[0];

  if (name.empty()) {
    LOG_ERROR("DASManager.ConvertLogEntryToJson", "Missing event name");
    return "";
  }

  if (name == DASMSG_FEATURE_START) {
    _feature_run_id = values[3]; // s3
    _feature_type = values[4]; // s4
  }

  std::ostringstream ostr;
  ostr << '{';
  serialize(ostr, "source", logEntry.tag);
  ostr << ',';
  serialize(ostr, "ts", GetTimeStamp(logEntry));
  ostr << ',';
  serialize(ostr, "seq", (int64_t) _seq++);
  ostr << ',';
  serialize(ostr, "level", GetLogLevel(logEntry));
  ostr << ',';
  serialize(ostr, "robot_id", _robot_id);
  ostr << ',';
  serialize(ostr, "robot_version", _robot_version);
  ostr << ',';
  serialize(ostr, "boot_id", _boot_id);
  ostr << ',';
  serialize(ostr, "profile_id", _profile_id);
  ostr << ',';
  serialize(ostr, "feature_type", _feature_type);
  ostr << ',';
  serialize(ostr, "feature_run_id", _feature_run_id);

  static const std::vector<std::string> keys =
    {"event", "s1", "s2", "s3", "s4", "i1", "i2", "i3", "i4"};

  for (unsigned int i = 0 ; i < keys.size(); i++) {
    if (!values[i].empty()) {
      ostr << ',';
      if (keys[i][0] == 'i') {
        serialize(ostr, keys[i], std::atoll(values[i].c_str()));
      } else {
        serialize(ostr, keys[i], values[i]);
      }
    }
  }

  ostr << '}';
  return ostr.str();
}
//
// Process a log entry
//
void DASManager::ProcessLogEntry(const AndroidLogEntry & logEntry)
{
  const char * message = logEntry.message;
  assert(message != nullptr);

  ++_entryCount;

  // Does this record look like a DAS entry?
  if (*message != Anki::Util::DAS::EVENT_MARKER) {
    return;
  }

  _eventCount++;

  std::string json = ConvertLogEntryToJson(logEntry);
  if (json.empty()) {
    return;
  }

  // Create the directory that will hold the json
  if (!Util::FileUtils::CreateDirectory(_dasConfig.GetStoragePath(), false, true)) {
    LOG_ERROR("DASManager.ProcessLogEntry.CreateStoragePathFailure",
              "Failed to create Storage Path");
    return;
  }

  // Make sure we are not over quota
  if (Util::FileUtils::GetDirectorySize(_dasConfig.GetStoragePath()) > (ssize_t) _dasConfig.GetStorageQuota()) {
    LOG_INFO("DASManager.ProcessLogEntry.OverQuota", "We have exceeded the quota for %s",
             _dasConfig.GetStoragePath().c_str());
    return;
  }

  // Append the JSON object to the array stored in the logfile
  if (!_logFile.is_open()) {
    _logFile.open(_logFilePath, std::ios::out | std::ofstream::binary | std::ofstream::ate);
  }
  off_t logFilePos = (off_t) _logFile.tellp();
  if (logFilePos == 0) {
    // This is a new file. Start with '[' to open the array
    _logFile << '[';
  } else {
    // Rewind 1 spot and replace the ']' with a ',' to continue the array
    _logFile.seekp(-1, std::ofstream::cur);
    _logFile << ',';
  }

  // Append the JSON object and close the array
  _logFile << json << ']';

}

void DASManager::RollLogFile() {
  _logFile.close();
  const std::string& fileName = GetPathNameForNextJsonLogFile();
  Util::FileUtils::MoveFile(fileName, _logFilePath);
  _last_flush_time = std::chrono::steady_clock::now();

  if (!_uploading) {
    auto uploadTask = [this]() {
      _uploading = true;
      PostLogsToServer();
      _uploading = false;
    };

    _worker.Wake(uploadTask, "uploadTask");
  }
}


//
// Log some process stats
//
void DASManager::ProcessStats()
{
  LOG_DEBUG("DASManager.ProcessStats.QueueStats",
            "entries=%llu events=%llu "
            "workerSuccess=%u workerFail=%u workerDropped=%u",
            _entryCount, _eventCount,
            (uint32_t) _workerSuccessCount,
            (uint32_t) _workerFailCount,
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
// Get Seconds elapsed since last upload to server
//
uint32_t DASManager::GetSecondsSinceLastFlush()
{
  auto end = std::chrono::steady_clock::now();
  if ((TimePoint) _last_flush_time >= end) {
    return 0;
  }
  auto diff = end - (TimePoint) _last_flush_time;
  return (uint32_t) std::chrono::duration_cast<std::chrono::seconds>(diff).count();
}

std::vector<std::string> DASManager::GetJsonFiles(const std::string& path)
{
  std::vector<std::string> jsonFiles =
    Util::FileUtils::FilesInDirectory(path, true, ".json", false);

  std::sort(jsonFiles.begin(), jsonFiles.end());

  return jsonFiles;
}

uint32_t DASManager::GetNextIndexForJsonFile()
{
  uint32_t index = 0;
  std::vector<std::string> storagePaths =
    {_dasConfig.GetStoragePath(), _dasConfig.GetBackupPath()};

  for (auto const& path : storagePaths) {
    std::vector<std::string> jsonFiles = GetJsonFiles(path);

    if (!jsonFiles.empty()) {
      const std::string& lastFile = jsonFiles.back();
      const std::string& filename = Util::FileUtils::GetFileName(lastFile, true, true);
      uint32_t next_index = (uint32_t) (std::atol(filename.c_str()) + 1);
      if (next_index > index) {
        index = next_index;
      }
    }
  }

  return index;
}

std::string DASManager::GetPathNameForNextJsonLogFile()
{
  uint32_t index = GetNextIndexForJsonFile();

  char filename[19] = {'\0'};
  std::snprintf(filename, sizeof(filename) - 1, "%012u.json", index);
  return Util::FileUtils::FullFilePath({_dasConfig.GetStoragePath(), filename});
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
  if (_dasConfig.GetURL().empty()) {
    LOG_ERROR("DASManager.Run.InvalidURL", "Invalid URL");
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  if (_dasConfig.GetStoragePath().empty()) {
    LOG_ERROR("DASManager.Run.InvalidStoragePath", "Invalid Storage Path");
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
    _robot_version = osState->GetRobotVersion();
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
  _last_flush_time = std::chrono::steady_clock::now();

  while (!shutdown) {
    struct log_msg logmsg;
    int rc = android_logger_list_read(log, &logmsg);
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
    // TODO: need to check our storage quota so we don't exceed it
    ProcessLogEntry(logEntry);

    // If we have exceeded the threshold size or gone over the flush interval,
    // time to roll this log file
    if ( (GetSecondsSinceLastFlush() > _dasConfig.GetFlushInterval())
         || (_logFile.tellp() > _dasConfig.GetFileThresholdSize()) ){
      RollLogFile();
    }

    // Print stats at regular intervals
    if (_entryCount % PROCESS_STATS_INTERVAL == 0) {
      ProcessStats();
    }

  }
  _exiting = true;
  LOG_DEBUG("DASManager.Run", "End reading loop");

  // Clean up
  LOG_DEBUG("DASManager.Run", "Cleaning up");
  android_logger_list_close(log);

  RollLogFile();

  auto shutdownTask = [this]() {
    BackupLogFiles();
  };

  _worker.WakeSync(shutdownTask, "shutdownTask");

  // Report final stats
  ProcessStats();

  LOG_DEBUG("DASManager.Run", "Done(result %d)", result);
  return result;
}

} // end namespace Victor
} // end namespace Anki
