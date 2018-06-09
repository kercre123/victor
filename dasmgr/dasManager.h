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
    int64_t ts;
    int64_t seq;
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

  using EventQueue = std::deque<LogEvent>;

  // Global state
  std::string _robot_id;
  std::string _robot_version;
  std::string _boot_id;
  std::string _profile_id;
  std::string _feature_type;
  std::string _feature_run_id;

  uint64_t _seq = 0;

  // Event queue
  EventQueue _eventQueue;

  // Bookkeeping
  uint64_t _entryCount = 0;
  uint64_t _eventCount = 0;
  uint64_t _sleepCount = 0;
  uint64_t _uploadSuccessCount = 0;
  uint64_t _uploadFailCount = 0;

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

  // Event serialization
  void Serialize(std::ostringstream & ostr, const LogEvent & event);
  void Serialize(std::ostringstream & ostr, const EventQueue & eventQueue);

};

} // end namespace Victor
} // end namespace Anki

#endif // __platform_dasmgr_dasManager_h
