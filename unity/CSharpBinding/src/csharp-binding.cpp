//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

#include "csharp-binding.h"

#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"

#include <algorithm>
#include <string>
#include <vector>

#if __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#define USE_IOS
#endif
#endif

#ifdef USE_IOS
#include "ios/ios-binding.h"
#endif

using namespace Anki;
using namespace Anki::Cozmo;
#ifdef USE_IOS
using namespace Anki::Cozmo::CSharpBinding;
#endif

bool initialized = false;

void Unity_DAS_Event(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sEventF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogE(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sErrorF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogW(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sWarningF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogI(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sInfoF(eventName, keyValues, "%s", eventValue);
}

void Unity_DAS_LogD(const char* eventName, const char* eventValue, const char** keys, const char** values, unsigned keyValueCount) {
  std::vector<std::pair<const char*, const char *>> keyValues;
  for(int i = 0; i < keyValueCount; ++i) {
    keyValues.push_back(std::pair<const char *, const char *>(keys[i], values[i]));
  }
  Anki::Util::sDebugF(eventName, keyValues, "%s", eventValue);
}

int cozmo_startup(const char *configuration_data)
{
    int result = (int)RESULT_OK;
    
#ifdef USE_IOS
    result = cozmo_engine_create(configuration_data);
#endif
    
    return result;
}

int cozmo_shutdown()
{
    int result = (int)RESULT_OK;
    
#ifdef USE_IOS
    result = cozmo_engine_destroy();
#endif
    
    return result;
}

int cozmo_wifi_setup(const char* wifiSSID, const char* wifiPasskey)
{
  int result = (int)RESULT_OK;
  
#ifdef USE_IOS
  result = cozmo_engine_wifi_setup(wifiSSID, wifiPasskey);
#endif
  
  return result;
}

void cozmo_send_to_clipboard(const char* log) {
  cozmo_engine_send_to_clipboard(log);
}
