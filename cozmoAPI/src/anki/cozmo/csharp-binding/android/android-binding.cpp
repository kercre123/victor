/**
 * File: android-binding
 *
 * Author: baustin
 * Created: 07/14/16
 *
 * Description: Android-specific native hooks for UI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/csharp-binding/android/android-binding.h"
#include "anki/cozmo/csharp-binding/csharp-binding.h"
#include "anki/cozmo/csharp-binding/breakpad/google_breakpad.h"
#include "anki/common/types.h"
#include "util/jni/includeJni.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

#include <exception>
#include <signal.h>

using namespace Anki;

namespace Anki {
namespace Cozmo {
namespace AndroidBinding {

void InstallGoogleBreakpad(const char* path)
{
  GoogleBreakpad::InstallGoogleBreakpad(path);
}

void UnInstallGoogleBreakpad()
{
  GoogleBreakpad::UnInstallGoogleBreakpad();
}

static void DisableHockeyApp()
{
  //
  // HockeyApp SDK does not document any method to disable exception handler once it has been installed.
  // Best we can do is disable exception handlers, reset signal handlers, then hope for the best.
  // See also: https://github.com/bitstadium/HockeySDK-Android
  // See also: ios-binding.cpp, hockeyApp.mm
    
  std::set_terminate(NULL);
  
  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  signal(SIGTRAP, SIG_DFL);
  signal(SIGABRT, SIG_DFL);
  signal(SIGSYS, SIG_DFL);
  signal(SIGPIPE, SIG_DFL);
}
  

int cozmo_shutdown()
{
  DisableHockeyApp();
  return (int)RESULT_OK;
}


}
}
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL
Java_com_anki_cozmo_BackgroundConnectivity_ExecuteBackgroundTransfers(JNIEnv* env, jclass clazz)
{
  PRINT_NAMED_INFO("AndroidBinding.BackgroundConnectivity", "received transfer request");
  cozmo_execute_background_transfers();
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  Anki::Util::JNIUtils::SetJvm(vm);
  return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
