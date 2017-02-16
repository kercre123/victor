/**
 * File: dasPlatform_android
 *
 * Author: baustin
 * Created: 07/05/16
 *
 * Description: Defines interface for platform-specific functionality
 *              needed to initialize DAS
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "dasPlatform_android.h"
#include <DAS/DAS.h>
#include <assert.h>

#define VALIDATE(a) if (!DASPlatform_Android_Validate((a), (#a))) return
#define JSTR "Ljava/lang/String;"
#define JACTIVITY "Landroid/app/Activity;"
#define JCONTEXT "Landroid/content/Context;"

namespace DAS {

JavaVM* DASPlatform_Android::_jvm = nullptr;

class JNIEnvWrapper {
public:
  JNIEnvWrapper(JavaVM* jvm) : _detachOnDestroy(false), _jvm(jvm), _env(nullptr) {
    if (_jvm == nullptr) {
      DASError("DASPlatform_Android.Init", "No JVM available");
      return;
    }

    // get JNI environment
    _jvm->GetEnv((void**)&_env, JNI_VERSION_1_6);
    if (_env == nullptr) {
      _detachOnDestroy = true;
      _jvm->AttachCurrentThread(&_env, nullptr);
    }
    if (_env == nullptr) {
      DASError("DASPlatform_Android.JNIEnvWrapper", "Could not attach to JVM");
      _detachOnDestroy = false;
    }
  }

  ~JNIEnvWrapper() {
    if (_detachOnDestroy) {
      _jvm->DetachCurrentThread();
    }
  }

  JNIEnv* GetEnv() { return _env; }

private:
  bool _detachOnDestroy;
  JavaVM* _jvm;
  JNIEnv* _env;
};


template <typename T>
static bool DASPlatform_Android_Validate(T object, const char* errorMessage)
{
  if (object == nullptr) {
    DASError("DASPlatform_Android.Init", "null java object: %s", errorMessage);
    assert(false);
    return false;
  }
  return true;
}

static std::string StringFromJString(JNIEnv* env, jobject stringObject)
{
  jstring jString = (jstring)stringObject;
  if (jString == nullptr) {
    return {};
  }
  const char *cPtr = env->GetStringUTFChars(jString, 0);
  std::string ret(cPtr);
  env->ReleaseStringUTFChars(jString, cPtr);
  env->DeleteLocalRef(jString);
  return ret;
}

void DASPlatform_Android::InitForUnityPlayer()
{
  JNIEnvWrapper jniWrapper{_jvm};
  JNIEnv* env = jniWrapper.GetEnv();
  if (env == nullptr) {
    assert(false);
    return;
  }

  jclass unityPlayer = env->FindClass("com/unity3d/player/UnityPlayer");
  VALIDATE(unityPlayer);
  jfieldID activityField = env->GetStaticFieldID(unityPlayer, "currentActivity", JACTIVITY);
  VALIDATE(activityField);
  jobject activity = env->GetStaticObjectField(unityPlayer, activityField);
  VALIDATE(activity);

  Init(env, activity);

  env->DeleteLocalRef(activity);
  env->DeleteLocalRef(unityPlayer);
}

void DASPlatform_Android::Init(jobject context)
{
  JNIEnvWrapper jniWrapper{_jvm};
  JNIEnv* env = jniWrapper.GetEnv();
  if (env == nullptr) {
    assert(false);
    return;
  }

  Init(env, context);
}

void DASPlatform_Android::Init(JNIEnv* env, jobject context)
{
  jclass dasClass = env->FindClass("com/anki/daslib/DAS");
  VALIDATE(dasClass);

  jmethodID deviceIdMethod = env->GetStaticMethodID(dasClass, "getDeviceID", "(" JSTR ")" JSTR);
  jmethodID combinedSystemVersionMethod = env->GetStaticMethodID(dasClass, "getCombinedSystemVersion", "()" JSTR);
  jmethodID deviceModelMethod = env->GetStaticMethodID(dasClass, "getModel", "()" JSTR);
  jmethodID osVersionMethod = env->GetStaticMethodID(dasClass, "getOsVersion", "()" JSTR);
  jmethodID platformMethod = env->GetStaticMethodID(dasClass, "getPlatform", "()" JSTR);
  jmethodID freeDiskSpaceMethod = env->GetStaticMethodID(dasClass, "getFreeDiskSpace", "()" JSTR);
  jmethodID totalDiskSpaceMethod = env->GetStaticMethodID(dasClass, "getTotalDiskSpace", "()" JSTR);
  jmethodID batteryLevelMethod = env->GetStaticMethodID(dasClass, "getBatteryLevel", "(" JCONTEXT ")" JSTR);
  jmethodID batteryStateMethod = env->GetStaticMethodID(dasClass, "getBatteryState", "(" JCONTEXT ")" JSTR);
  jmethodID appVersionMethod = env->GetStaticMethodID(dasClass, "getDasVersion", "(" JCONTEXT ")" JSTR);
  VALIDATE(deviceIdMethod);
  VALIDATE(combinedSystemVersionMethod);
  VALIDATE(deviceModelMethod);
  VALIDATE(osVersionMethod);
  VALIDATE(platformMethod);
  VALIDATE(freeDiskSpaceMethod);
  VALIDATE(totalDiskSpaceMethod);
  VALIDATE(batteryLevelMethod);
  VALIDATE(batteryStateMethod);
  VALIDATE(appVersionMethod);

  jstring jString = env->NewStringUTF(_deviceUUIDPath.c_str());
  _deviceId = StringFromJString(env, env->CallStaticObjectMethod(dasClass, deviceIdMethod, jString));
  env->DeleteLocalRef(jString);

  _deviceModel = StringFromJString(env, env->CallStaticObjectMethod(dasClass, deviceModelMethod));
  _osVersion = StringFromJString(env, env->CallStaticObjectMethod(dasClass, osVersionMethod));
  _combinedSystemVersion = StringFromJString(env, env->CallStaticObjectMethod(dasClass, combinedSystemVersionMethod));
  _platform = StringFromJString(env, env->CallStaticObjectMethod(dasClass, platformMethod));
  _freeDiskSpace = StringFromJString(env, env->CallStaticObjectMethod(dasClass, freeDiskSpaceMethod));
  _totalDiskSpace = StringFromJString(env, env->CallStaticObjectMethod(dasClass, totalDiskSpaceMethod));
  _batteryLevel = StringFromJString(env, env->CallStaticObjectMethod(dasClass, batteryLevelMethod, context));
  _batteryState = StringFromJString(env, env->CallStaticObjectMethod(dasClass, batteryStateMethod, context));
  _appVersion = StringFromJString(env, env->CallStaticObjectMethod(dasClass, appVersionMethod, context));

  jmethodID miscInfoMethod = env->GetStaticMethodID(dasClass, "getMiscInfo", "()[" JSTR);
  VALIDATE(miscInfoMethod);

  jobjectArray miscArray = (jobjectArray)env->CallStaticObjectMethod(dasClass, miscInfoMethod);
  if (miscArray != nullptr) {
    jsize arraySize = env->GetArrayLength(miscArray);
    if (arraySize % 2 != 0) {
      assert(false);
      DASError("DASPlatform_Android.Init", "unexpected misc array size: %d", arraySize);
    }
    for (jsize i = 0; i < arraySize; i += 2) {
      std::string eventName = StringFromJString(env, env->GetObjectArrayElement(miscArray, i));
      std::string eventValue = StringFromJString(env, env->GetObjectArrayElement(miscArray, i+1));
      _miscInfo.emplace(std::move(eventName), std::move(eventValue));
    }
  }
  env->DeleteLocalRef(miscArray);

  env->DeleteLocalRef(dasClass);
}

const std::map<std::string, std::string>& DASPlatform_Android::GetMiscGlobals() const
{
  static const std::map<std::string, std::string> empty;
  return empty;
}

#undef JCONTEXT
#undef JACTIVITY
#undef JSTR
#undef VALIDATE

}
