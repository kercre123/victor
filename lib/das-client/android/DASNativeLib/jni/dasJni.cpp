/**
 * File: dasJni.cpp
 *
 * Author: seichert
 * Created: 07/09/2014
 *
 * Description: Connect Java DAS functions to C++ implementations
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include <jni.h>
#include <pthread.h>
#include "DAS.h"
#include "DASPrivate.h"
#include "dasPlatform_android.h"

#include <cstring>
#include <string>
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_anki_daslib_DAS
 * Method:    nativeLog
 * Signature: (ILjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_nativeLog(JNIEnv *env, jclass clazz,
                                   jint dasLogLevel, jstring jEventName, jstring jEventValue) {
  const char* eventName = env->GetStringUTFChars(jEventName, nullptr);
  const char* eventValue = env->GetStringUTFChars(jEventValue, nullptr);

  _DAS_Log((DASLogLevel) dasLogLevel, eventName, eventValue, nullptr, nullptr, -1, nullptr);

  env->ReleaseStringUTFChars(jEventName, eventName);
  env->ReleaseStringUTFChars(jEventValue, eventValue);
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    IsEventEnabledForLevel
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL
Java_com_anki_daslib_DAS_IsEventEnabledForLevel(JNIEnv *env, jclass clazz,
                                                jstring jEventName, jint dasLogLevel) {
  const char *eventName = env->GetStringUTFChars(jEventName, nullptr);
  int result = _DAS_IsEventEnabledForLevel(eventName, (DASLogLevel) dasLogLevel);
  env->ReleaseStringUTFChars(jEventName, eventName);
  return result ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    Configure
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_Configure(JNIEnv *env, jclass clazz,
                                   jstring jConfigurationXMLFilePath,
                                   jstring jLogDirPath,
                                   jstring jGameLogDirPath) {
  const char* configurationXMLFilePath = env->GetStringUTFChars(jConfigurationXMLFilePath, nullptr);
  const char* logDirPath = env->GetStringUTFChars(jLogDirPath, nullptr);
  const char* gameLogDirPath = env->GetStringUTFChars(jGameLogDirPath, nullptr);
  DASConfigure(configurationXMLFilePath, logDirPath, gameLogDirPath);
  env->ReleaseStringUTFChars(jConfigurationXMLFilePath, configurationXMLFilePath);
  env->ReleaseStringUTFChars(jLogDirPath, logDirPath);
  env->ReleaseStringUTFChars(jGameLogDirPath, gameLogDirPath);
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_Close(JNIEnv *env, jclass clazz) {
  DASClose();
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    SetGlobal
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_SetGlobal(JNIEnv *env, jclass clazz,
                                   jstring jKey, jstring jValue) {
  const char* key = env->GetStringUTFChars(jKey, nullptr);
  const char* value = (jValue != nullptr) ? env->GetStringUTFChars(jValue, nullptr) : nullptr;
  DAS_SetGlobal(key, value);
  env->ReleaseStringUTFChars(jKey, key);
  if (jValue != nullptr) {
    env->ReleaseStringUTFChars(jValue, value);
  }
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    EnableNetwork
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_EnableNetwork(JNIEnv *env, jclass clazz, jint jReason)
{
  DASEnableNetwork((DASDisableNetworkReason) jReason);
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    DisableNetwork
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_DisableNetwork(JNIEnv *env, jclass clazz, jint jReason)
{
  DASDisableNetwork((DASDisableNetworkReason) jReason);
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    GetNetworkingDisabled
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_com_anki_daslib_DAS_GetNetworkingDisabled(JNIEnv *env, jclass clazz)
{
  return (jint) DASNetworkingDisabled;
}

/*
 * Class:     com_anki_daslib_DAS
 * Method:    SetLevel
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL
Java_com_anki_daslib_DAS_SetLevel(JNIEnv *env, jclass clazz,
                                  jstring jEventName, jint dasLogLevel) {
  const char *eventName = env->GetStringUTFChars(jEventName, nullptr);
  _DAS_SetLevel(eventName, (DASLogLevel) dasLogLevel);
  env->ReleaseStringUTFChars(jEventName, eventName);
}


static JavaVM* sJvm = nullptr;
static jclass sDASClass = nullptr;
static jmethodID sPostToServerMethodID = nullptr;
static pthread_key_t sCleanupKey;

void dasThreadDestructor(void* arg)
{
  JNIEnv* env = (JNIEnv *) arg;

  if (nullptr != env) {
    JavaVM* jvm = nullptr;
    (void) env->GetJavaVM(&jvm);
    if (nullptr != jvm) {
      jvm->DetachCurrentThread();
    }
  }
}

JNIEnv* getJniEnv()
{
  JNIEnv* env = nullptr;

  if (nullptr != sJvm) {
    sJvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (nullptr == env) {
      (void) sJvm->AttachCurrentThread(&env, nullptr);

      // This will call dasThreadDestructor above when the thread exits to guarantee
      // that DetachCurrentThread is called.
      (void) pthread_setspecific(sCleanupKey, env);
    }
  }

  return env;
}

// JNI_OnLoad is called when the client app loads das_shared.so
// At that time, the classloader is fully available and we can find
// the DAS class and most importantly the postToServer method.
// If we wait to try and find the method in dasPostToServer (below),
// we will not have a classloader available and will not be available to find
// the DAS class.  Roughly speaking this is because, we call dasPostToServer
// on a thread that has never previously been attached to the JVM.  Even
// with attaching it as shown above in getJniEnv(), FindClass still returns
// nullptr.
jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
  sJvm = vm;
  DAS::DASPlatform_Android::SetJVM(vm);

  JNIEnv* env = getJniEnv();
  jclass localClass = env->FindClass("com/anki/daslib/DAS");
  sDASClass = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
  sPostToServerMethodID = env->GetStaticMethodID(sDASClass, "postToServer",
                                                 "(Ljava/lang/String;Ljava/lang/String;Ljava/nio/ByteBuffer;)Z");
  (void) pthread_key_create(&sCleanupKey, dasThreadDestructor);
  return JNI_VERSION_1_6;
}

bool dasPostToServer(const std::string& url, const std::string& postBody, std::string& out_response)
{
  JNIEnv* env = getJniEnv();

  if (nullptr == env) {
    return false;
  }

  jstring jUrl = env->NewStringUTF(url.c_str());
  jstring jPostBody = env->NewStringUTF(postBody.c_str());

  const int buff_size = 1024 * 5;
  char buff[buff_size];
  buff[0] = '\0';

  jboolean ret = env->CallStaticBooleanMethod(sDASClass, sPostToServerMethodID, jUrl, jPostBody,
                                              env->NewDirectByteBuffer(buff, buff_size));

  const size_t strLen = std::strlen(buff);
  if (strLen > 0) {
    out_response = std::string(buff);
  }

  env->DeleteLocalRef(jUrl);
  env->DeleteLocalRef(jPostBody);

  return (bool) ret;
}


#ifdef __cplusplus
}
#endif
