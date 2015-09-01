//
//  csharp-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 4/11/15.
//
//

#include "csharp-binding.h"

#include "anki/cozmo/game/cozmoGame.h"
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

void Unity_DAS_Event(const char* eventName, const char* eventValue) {
  PRINT_NAMED_EVENT(eventName, "%s", eventValue);
}

void Unity_DAS_LogE(const char* eventName, const char* eventValue) {
  PRINT_NAMED_ERROR(eventName, "%s", eventValue);
}

void Unity_DAS_LogW(const char* eventName, const char* eventValue) {
  PRINT_NAMED_WARNING(eventName, "%s", eventValue);
}

void Unity_DAS_LogI(const char* eventName, const char* eventValue) {
  PRINT_NAMED_INFO(eventName, "%s", eventValue);
}

void Unity_DAS_LogD(const char* eventName, const char* eventValue) {
  PRINT_NAMED_DEBUG(eventName, "%s", eventValue);
}

int cozmo_startup(const char *configuration_data)
{
    int result = (int)RESULT_OK;
    
#ifdef USE_IOS
    result = cozmo_game_create(configuration_data);
#endif
    
    return result;
}

int cozmo_shutdown()
{
    int result = (int)RESULT_OK;
    
#ifdef USE_IOS
    result = cozmo_game_destroy();
#endif
    
    return result;
}

int cozmo_update(float current_time)
{
    int result = (int)RESULT_OK;
    
#ifdef USE_IOS
    result = cozmo_game_update(current_time);
#endif

    return result;
}

