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

//
// This file is not used on victor. Contents are preserved for reference.
//
#if !defined(VICOS_LA) && !defined(VICOS_LE)

#include "engine/cozmoAPI/csharp-binding/android/android-binding.h"
#include "engine/cozmoAPI/csharp-binding/csharp-binding.h"
#include "engine/cozmoAPI/csharp-binding/breakpad/google_breakpad.h"
#include "coretech/common/shared/types.h"
#include "clad/externalInterface/messageGameToEngine.h"
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

JNIEXPORT void JNICALL
Java_com_anki_cozmoengine_Standalone_startCozmoEngine(JNIEnv* env,
                                                      jobject thiz,
                                                      jobject activity,
                                                      jstring configuration)
{
  Anki::Util::JNIUtils::SetCurrentActivity(env, activity);
  std::string configurationString = Anki::Util::JNIUtils::getStringFromJString(env, configuration);
  (void) cozmo_startup(configurationString.c_str());

//  // Yes this is a hack but this is all temporary code to get a standalone version of the engine running
//  bool isExternalSdkMode = true;
//  uint8_t enter_sdk_mode_message[2] =
//    { static_cast<uint8_t>(Anki::Cozmo::ExternalInterface::MessageGameToEngineTag::EnterSdkMode),
//      static_cast<uint8_t>(isExternalSdkMode) };
//  cozmo_transmit_game_to_engine(enter_sdk_mode_message, sizeof(enter_sdk_mode_message));
}

JNIEXPORT void JNICALL
Java_com_anki_cozmoengine_Standalone_stopCozmoEngine(JNIEnv* env,
                                                     jobject thiz)
{
  uint8_t exit_sdk_mode_message[1] =
    { static_cast<uint8_t>(Anki::Cozmo::ExternalInterface::MessageGameToEngineTag::ExitSdkMode) };
  cozmo_transmit_game_to_engine(exit_sdk_mode_message, sizeof(exit_sdk_mode_message));
  cozmo_shutdown();
}

JNIEXPORT void JNICALL
Java_com_anki_cozmo_CozmoActivity_installBreakpad(JNIEnv* env, jclass thiz, jstring jpath)
{
  const char* path = env->GetStringUTFChars(jpath, nullptr);
  Anki::Cozmo::AndroidBinding::InstallGoogleBreakpad(path);
  env->ReleaseStringUTFChars(jpath, path);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  // Use one of our java project classes to acquire the class loader that can be used on other threads
  Anki::Util::JNIUtils::SetJvm(vm, "com/anki/util/PermissionUtil");

  return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif

#endif
