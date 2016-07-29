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
#include "util/logging/logging.h"
#include <jni.h>

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
  return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif
