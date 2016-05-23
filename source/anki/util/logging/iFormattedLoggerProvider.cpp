/**
 * File: iFormattedLoggerProvider
 *
 * Author: Trevor Dasch
 * Created: 2/10/16
 *
 * Description: abstract class
 * that formats the log into a c-string
 * and calls its Log function.
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "util/logging/iFormattedLoggerProvider.h"
#include "util/math/numericCast.h"
#include <sstream>
#include <string>
#include <iomanip>

#define PRINT_TID 1
#define PRINT_DAS_EXTRAS_BEFORE_EVENT 0
#define PRINT_DAS_EXTRAS_AFTER_EVENT 0

#if (PRINT_TID)
#include <stdarg.h>
#include <cstdint>
#include <ctime>
#include <thread>
#include <atomic>
#include <cassert>
#include <pthread.h>
#endif

namespace Anki {
  namespace Util {
    
#if (PRINT_TID)
    static std::atomic<uint32_t> thread_max {0};
    static pthread_key_t thread_id_key;
    static pthread_once_t thread_id_once = PTHREAD_ONCE_INIT;
    
    static void thread_id_init()
    {
      pthread_key_create(&thread_id_key, nullptr);
    }
#endif
    
    void IFormattedLoggerProvider::FormatAndLog(IFormattedLoggerProvider::LogLevel logLevel, const char* eventName,
                                      const std::vector<std::pair<const char*, const char*>>& keyValues,
                                      const char* eventValue)
    {
#if (PRINT_TID)
      pthread_once(&thread_id_once, thread_id_init);
      
      uint32_t thread_id = numeric_cast<uint32_t>((uintptr_t)pthread_getspecific(thread_id_key));
      if(0 == thread_id) {
        thread_id = ++thread_max;
        pthread_setspecific(thread_id_key, (void*)((uintptr_t)thread_id));
      }
#endif
      
#if (PRINT_DAS_EXTRAS_BEFORE_EVENT || PRINT_DAS_EXTRAS_AFTER_EVENT)
      const size_t kMaxStringBufferSize = 1024;
      char logString[kMaxStringBufferSize]{0};
      for (const auto& keyValuePair : keyValues) {
        snprintf(logString, kMaxStringBufferSize, "%s[%s: %s] ", logString, keyValuePair.first, keyValuePair.second);
      }
#endif
      std::ostringstream stream;
      
#if (PRINT_TID)
      stream << "(t:" << std::setw(2) << std::setfill('0') << thread_id << ") ";
#endif
      
      stream << "[" << GetLogLevelString(logLevel) << "]";
      
#if (PRINT_DAS_EXTRAS_BEFORE_EVENT)
      stream << " " << logString;
#endif
      
      stream << " " << eventName << " " << eventValue;
      
#if (PRINT_DAS_EXTRAS_AFTER_EVENT)
      stream << " " << logString;
#endif
      
      stream << std::endl;
      
      Log(logLevel, stream.str());
    }
    
  } // end namespace Util
} // end namespace Anki
