/**
 * File: messageProfiler
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "util/global/globalDefinitions.h"

#if ANKI_PROFILING_ENABLED
  #define ANKI_MESSAGE_PROFILER_ENABLED 1
#else
  #define ANKI_MESSAGE_PROFILER_ENABLED 0
#endif

#ifndef __Util_MessageProfiler_MessageProfiler_H__
#define __Util_MessageProfiler_MessageProfiler_H__

#include <string>
#include <time.h>

namespace Anki {
namespace Util {

#if defined(ANKI_MESSAGE_PROFILER_ENABLED)

  // ================================================================================
  // MessageProfiler
  
  class MessageProfiler {
  public:
    MessageProfiler(const std::string& prefix);
    void Update(int msg, size_t size);
    void ReportOnFailure();
    void Report();

  private:
    static const constexpr int kMaxMessages = 256;

    std::string m_prefix;
    clock_t m_start;
    bool m_started;
    bool m_failed;

    int m_count[kMaxMessages];
    size_t m_size[kMaxMessages];
  };

#else // ANKI_MESSAGE_PROFILER_ENABLED

  class MessageProfiler {
  public:
    MessageProfiler(const std::string& prefix);
    void Update(int msg, size_t size);
    void ReportOnFailure();
    void Report();
  };

#endif // ANKI_MESSAGE_PROFILER_ENABLED

} // end namespace Util
} // end namespace Anki

#endif // __Util_MessageProfiler_MessageProfiler_H__
