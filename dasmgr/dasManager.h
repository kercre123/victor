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

#include "coretech/common/shared/types.h" // Anki Result
#include "util/dispatchQueue/taskExecutor.h" // Anki TaskExecutor
#include "util/logging/logtypes.h" // Anki LogLevel

#include <deque>
#include <memory>
#include <string>

// Forward declarations
typedef struct AndroidLogEntry_t AndroidLogEntry;

namespace Anki {
  namespace Victor {
    class DASConfig;
  }
}

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

//
  // Internal representation of a single log event
  // See also:
  // https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/221151429/DAS+Event+Fields+for+Victor+Robot
  // Yes, it could be more efficient.
  //
  // Note some fields (robot_id, robot_version) are static for the lifetime
  // of the service. These fields are not tracked for each event.
  //
  struct DASEvent
  {
    std::string name;
    int64_t ts;
    int64_t seq;
    Anki::Util::LogLevel level;
    std::string profile_id;
    std::string feature_type;
    std::string feature_run_id;
    std::string source;
    std::string s1;
    std::string s2;
    std::string s3;
    std::string s4;
    int64_t i1;
    int64_t i2;
    int64_t i3;
    int64_t i4;
  };

  using DASEventQueue = std::deque<DASEvent>;
  using DASEventQueuePtr = std::shared_ptr<DASEventQueue>;
  using DASEventQueuePtrQueue = std::deque<DASEventQueuePtr>;
  using LogLevel = Anki::Util::LogLevel;
  using TaskExecutor = Anki::Util::TaskExecutor;

  // DASConfig parameters
  std::string _url;
  size_t _queue_threshold_size;
  size_t _max_deferrals_size;

  // Global state
  std::string _robot_id;
  std::string _robot_version;
  std::string _boot_id;
  std::string _profile_id;
  std::string _feature_type;
  std::string _feature_run_id;

  uint64_t _seq = 0;

  // Event queue
  DASEventQueuePtr _eventQueue = std::make_shared<DASEventQueue>();

  // Worker thread and thread-safe counters
  TaskExecutor _worker;
  std::atomic_uint32_t _workerSuccessCount = {0};
  std::atomic_uint32_t _workerFailCount = {0};
  std::atomic_uint32_t _workerDeferralCount = {0};
  std::atomic_uint32_t _workerDroppedCount = {0};

  // Queue of deferred uploads. This member is only accessed from
  // worker thread so it does not need a mutex.
  DASEventQueuePtrQueue _workerDeferrals;

  // Bookkeeping
  uint64_t _entryCount = 0;
  uint64_t _eventCount = 0;
  uint64_t _sleepCount = 0;

  // Event serialization
  void Serialize(std::ostringstream & ostr, const DASEvent & event);
  void Serialize(std::ostringstream & ostr, const DASEventQueue & eventQueue);

  // Attempt to upload a single event queue to DAS endpoint.
  // This is called on the worker thread.
  // Returns true on successful upload.
  bool PostToServer(const DASEventQueue & eventQueue);

  // Perform retry of deferred uploads.
  // This is called on the worker thread in response to a successful upload.
  // Failed retries are left at the front of the retry queue.
  void PerformDeferredUploads();

  // Enqueue events for delivery at some time in the future
  // This is called on the worker thread in response to a failed upload.
  // If deferral queue is full, the oldest events are discarded.
  void PerformDeferral(DASEventQueuePtr eventQueue);

  // Upload queued events to server.
  // This is called on the worker thread.
  void PerformUpload(DASEventQueuePtr eventQueue);

  // Submit queue for upload to server.
  // This is called on the main thread to process a queue of events.
  void EnqueueForUpload(DASEventQueuePtr eventQueue);

// Process an event struct
  void ProcessEvent(DASEvent && event);

// Parse an event struct
  bool ParseLogEntry(const AndroidLogEntry & logEntry, DASEvent & event);

// Process a log message
  void ProcessLogEntry(const AndroidLogEntry & logEntry);

  // Log some process stats.
  // This is called on the main thread at regular intervals.
  void ProcessStats();

  // Shutdown helpers. These are called on the main thread
  // when process is shutting down.
  void FlushEventQueue();
  void FlushUploadQueue();

};

} // end namespace Victor
} // end namespace Anki

#endif // __platform_dasmgr_dasManager_h
