/**
 * File: messageProfiler
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "messageProfiler.h"

#include "util/logging/logging.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "MessageProfiler"

namespace Anki {
namespace Util {

#if ANKI_MESSAGE_PROFILER_ENABLED

  CONSOLE_VAR_RANGED(float, kMessageProfilerDuration, "CpuProfiler", 0.f, 0.f, 60.f*60.f);

  // ================================================================================
  // MessageProfiler
  
  MessageProfiler::MessageProfiler(const std::string& prefix)
  : m_prefix(prefix)
  , m_start(clock())
  , m_failed(false) {
    memset(m_count, 0, sizeof(m_count));
  }

  void MessageProfiler::Update(int msg, size_t size) {
    if (msg < kMaxMessages) {
      ++m_count[msg];
      m_size[msg] = size;
    }

    clock_t end = clock();
    float duration = ((float)(end - m_start)) / CLOCKS_PER_SEC;
    if (kMessageProfilerDuration > 0.0f && duration >= kMessageProfilerDuration) {
      Report();
    }
  }

  void MessageProfiler::ReportOnFailure() {
    clock_t end = clock();
    float duration = ((float)(end - m_start)) / CLOCKS_PER_SEC;

    Report();

    if (duration > 10.0f) {
      // ignore errors in the first 10 seconds to allow for start-up issues
      m_failed = true;
    }
  }

  void MessageProfiler::Report() {
    if (m_failed) {
      // failed already, prevent double-reporting
      return;
    }
  
    clock_t end = clock();
    float duration = ((float)(end - m_start)) / CLOCKS_PER_SEC;
    if (duration == 0.0f) {
      // avoid division by zero later
      duration = 1.0f;
    }

    LOG_INFO(m_prefix.c_str(), "duration: %0.2f", duration);
    for(int i = 0; i < kMaxMessages; ++i) {
      if (m_count[i] > 0) {
        LOG_INFO(m_prefix.c_str(), "Tag: 0x%02X %0.2f messages/s = %zu bytes/message (%0.2f bytes/sec)",
                 i,
                 m_count[i] / duration,
                 m_size[i],
                 m_count[i] * m_size[i] / duration);
      }
    }
    
    m_start = end;
    memset(m_count, 0, sizeof(m_count));
  }

#else // ANKI_MESSAGE_PROFILER_ENABLED

  MessageProfilerMessage::Profiler(const std::string& /*prefix*/, float /*duration*/) {
  }

  void MessageProfiler::Update(int /*msg*/, size_t /*size*/) {
  }

  void MessageProfiler::ReportOnFailure() {
  }

  void Report() {
  }

#endif // ANKI_MESSAGE_PROFILER_ENABLED

} // end namespace Util
} // end namespace Anki
