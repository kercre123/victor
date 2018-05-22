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

#include "util/logging/iLoggerProvider.h"  // Anki LogLevel
#include "coretech/common/shared/types.h"  // Anki Result
#include <string>
#include <deque>

// Forward declarations
typedef struct AndroidLogEntry_t AndroidLogEntry;

namespace Anki {
namespace Victor {

class DASManager {
public:
  // Run until error or shutdown flag becomes true
  // Returns 0 on successful termination, else error code
  Result Run(const bool & shutdown);

private:
  using LogLevel = Anki::Util::ILoggerProvider::LogLevel;

  //
  // Internal representation of a single log event
  // See also:
  // https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/221151429/DAS+Event+Fields+for+Victor+Robot
  // Yes, it could be more efficient
  //
  // Note some fields (robot_id, robot_version) are static for the lifetime
  // of the service.  These fields are not tracked for each event.
  //
  struct LogEvent
  {
    std::string name;
    uint64_t ts;
    uint64_t seq;
    LogLevel level;
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

  // Global state
  std::string _robot_id;
  std::string _robot_version;
  std::string _profile_id = "system";
  std::string _feature_type = "system";
  std::string _feature_run_id = "system";
  uint64_t _seq = 0;

  // Event queue
  std::deque<LogEvent> _eventQueue;

  // Bookkeeping
  uint64_t _entryCount = 0;
  uint64_t _eventCount = 0;
  uint64_t _sleepCount = 0;

  // Process a log message
  void ProcessLogEntry(const AndroidLogEntry & logEntry);

  // Parse an event struct
  bool ParseLogEntry(const AndroidLogEntry & logEntry, LogEvent & event);

// Process an event struct
  void ProcessEvent(LogEvent && logEvent);

  // Process queued events
  void ProcessQueue();

  // Log stats
  void ProcessStats();

};

} // end namespace Victor
} // end namespace Anki

#endif // __platform_dasmgr_dasManager_h
