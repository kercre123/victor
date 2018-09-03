/**
* File: victor/dasmgr/dasManager.h
*
* Description: DASManager class declarations
*
* Copyright: Anki, inc. 2018
*
*/

#ifndef __victor_dasmgr_dasManager_h
#define __victor_dasmgr_dasManager_h

#include "dasConfig.h"
#include "coretech/common/shared/types.h" // Anki Result
#include "util/dispatchQueue/taskExecutor.h" // Anki TaskExecutor
#include "util/logging/logtypes.h" // Anki LogLevel

#include <chrono>
#include <deque>
#include <fstream>
#include <memory>
#include <string>

// Forward declarations
typedef struct AndroidLogEntry_t AndroidLogEntry;

namespace Anki {
namespace Victor {

class DASManager {
public:
  // Class constructor
  DASManager(const DASConfig & dasConfig);

  // Run until error or shutdown flag becomes true
  // Returns 0 on successful termination, else error code
  Result Run(const bool & shutdown);

private:
  using LogLevel = Anki::Util::LogLevel;
  using TaskExecutor = Anki::Util::TaskExecutor;
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

  DASConfig _dasConfig;

  // Global event fields
  uint64_t _seq = 0;
  std::string _robot_id;
  std::string _robot_version;
  std::string _boot_id;
  std::string _profile_id;
  std::string _feature_type;
  std::string _feature_run_id;
  std::string _ble_conn_id;
  std::string _wifi_conn_id;

  // Runtime state
  std::atomic<TimePoint> _last_flush_time;
  bool _allow_upload = false;
  bool _purge_backup_files = false;
  bool _exiting = false;
  bool _uploading = false;
  std::string _logFilePath;
  std::ofstream _logFile;

  // Worker thread and thread-safe counters
  TaskExecutor _worker;
  std::atomic_uint32_t _workerSuccessCount = {0};
  std::atomic_uint32_t _workerFailCount = {0};
  std::atomic_uint32_t _workerDroppedCount = {0};

  // Bookkeeping
  uint64_t _entryCount = 0;
  uint64_t _eventCount = 0;

  // Called from uploader thread
  bool PostToServer(const std::string& pathToLogFile);
  void PostLogsToServer();
  void BackupLogFiles();
  void PurgeBackupFiles();
  void EnforceStorageQuota();

  std::string ConvertLogEntryToJson(const AndroidLogEntry & logEntry);

  // Process a log message
  void ProcessLogEntry(const AndroidLogEntry & logEntry);

  // Rename das.log to 00000000000X.json for the uploader task to pick up
  void RollLogFile();

  // Log some process stats.
  // This is called on the main thread at regular intervals.
  void ProcessStats();

  // Get Time Elapsed in seconds since last flush
  uint32_t GetSecondsSinceLastFlush();

  // Get the sorted list of files containing JSON to be uploaded
  std::vector<std::string> GetJsonFiles(const std::string& path);

  // We don't want to overwrite existing files, get the next index to use
  uint32_t GetNextIndexForJsonFile();
  std::string GetPathNameForNextJsonLogFile();

  void LoadTransientGlobals(const std::string & path);
  void LoadPersistentGlobals(const std::string & path);
  void LoadGlobalState();

  void SaveTransientGlobals(const std::string & path);
  void SavePersistentGlobals(const std::string & path);
  void SaveGlobalState();

};

} // end namespace Victor
} // end namespace Anki

#endif // __platform_dasmgr_dasManager_h
