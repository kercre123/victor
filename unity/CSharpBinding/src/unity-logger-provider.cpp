/**
* File: printfLoggerProvider
*
* Author: damjan stulic
* Created: 4/25/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#include "unity-logger-provider.h"

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
#include <string>
#include <pthread.h>
#endif

using namespace Anki::Cozmo;

#if (PRINT_TID)
static std::atomic<uint32_t> thread_max {0};
static pthread_key_t thread_id_key;
static pthread_once_t thread_id_once = PTHREAD_ONCE_INIT;

static void thread_id_destroy(void* thread_id)
{
  delete static_cast<uint32_t*>(thread_id);
}

static void thread_id_init()
{
  pthread_key_create(&thread_id_key, thread_id_destroy);
}
#endif

void UnityLoggerProvider::Write(int logLevel, const char* logLevelName, const char* eventName,
  const std::vector<std::pair<const char*, const char*>>& keyValues,
  const char* eventValue)
{
#if (PRINT_TID)
  pthread_once(&thread_id_once, thread_id_init);

  uint32_t* thread_id = (uint32_t*)pthread_getspecific(thread_id_key);
  if(nullptr == thread_id) {
    thread_id = new uint32_t {++thread_max};
    pthread_setspecific(thread_id_key, thread_id);
  }
#endif

const size_t kMaxFullSize = 1024;
char fullMessage[kMaxFullSize]{0};
#if (PRINT_TID)
  printf("(t:%02d) [%s] %s : %s",
    *thread_id, logLevelName, eventName, eventValue);
  int length = snprintf(fullMessage, kMaxFullSize, "(t:%02d) [%s] %s : %s",
    *thread_id, logLevelName, eventName, eventValue);
#else
  printf("[%s] %s : %s",
    logLevelName, eventName, eventValue);
  int length = snprintf(fullMessage, kMaxFullSize, "[%s] %s : %s",
    logLevelName, eventName, eventValue);
#endif
  
  if (length < 0) {
    printf("Error logging to unity: %s\n", eventName);
    return;
  }
  if (length >= kMaxFullSize) {
    length = kMaxFullSize - 1;
    fullMessage[length] = 0;
  }
  std::lock_guard<std::mutex> lock(_mutex);
  if (logCallback != nullptr) {
    logCallback(logLevel, fullMessage);
  }
}
