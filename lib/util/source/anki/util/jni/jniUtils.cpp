#ifdef ANDROID

#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Util {

JavaVM* JNIUtils::_sJvm = nullptr;
jobject JNIUtils::_sCurrentActivity = nullptr;

jobjectArray JNIUtils::convertStringMapToJObjectArray(JNIEnv* env, const std::map<std::string,std::string>& stringMap, std::vector<jstring>& stringRefs)
{
  JClassHandle clazz{env->FindClass("java/lang/String"), env};
  jobjectArray arr = env->NewObjectArray((jsize)(stringMap.size()*2), clazz.get(), 0);
  int k = 0;
  for (std::map<std::string,std::string>::const_iterator it = stringMap.begin(); it != stringMap.end(); ++it, k+=2) {
    std::string key = it->first;
    std::string val = it->second;
    jstring keyStr = env->NewStringUTF(key.c_str());
    jstring valStr = env->NewStringUTF(val.c_str());
    stringRefs.push_back(keyStr);
    stringRefs.push_back(valStr);
    env->SetObjectArrayElement(arr, k, keyStr);
    env->SetObjectArrayElement(arr, k+1, valStr);
  }
  return arr;
}

std::string JNIUtils::getStringFromJString(JNIEnv* env, jstring jString)
{
  if (nullptr == env) {
    return std::string();
  }

  if (nullptr == jString) {
    return std::string();
  }
  
  const char *cPtr = env->GetStringUTFChars(jString, 0);
  std::string ret(cPtr);
  env->ReleaseStringUTFChars(jString, cPtr);
  return ret;
}

bool JNIUtils::getStaticBooleanFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName)
{
  bool value = false;

  if (nullptr == env) {
    return value;
  }

  jfieldID fieldId = env->GetStaticFieldID(clazz, fieldName, "Z");
  value = (bool) env->GetStaticBooleanField(clazz, fieldId);

  return value;
}

int JNIUtils::getStaticIntFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName)
{
  int value = -1;

  if (nullptr == env) {
    return value;
  }

  jfieldID fieldId = env->GetStaticFieldID(clazz, fieldName, "I");
  value = (int) env->GetStaticIntField(clazz, fieldId);

  return value;
}

std::string JNIUtils::getStaticStringFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName)
{
  if (nullptr == env) {
    return std::string();
  }

  jfieldID fieldId = env->GetStaticFieldID(clazz, fieldName, "Ljava/lang/String;");
  JStringHandle javaString{(jstring)env->GetStaticObjectField(clazz, fieldId), env};

  return getStringFromJString(env, javaString.get());
}

std::string JNIUtils::getStringFromStaticObjectMethod(JNIEnv* env, const jclass clazz,
                                                      const char* name, const char* sig)
{
  if (nullptr == env) {
    return std::string();
  }
  jmethodID method = env->GetStaticMethodID(clazz, name, sig);
  return getStringFromStaticObjectMethod(env, clazz, method);
}

std::string JNIUtils::getStringFromStaticObjectMethod(JNIEnv* env, const jclass clazz,
                                                      const jmethodID method)
{
  if (nullptr == env) {
    return std::string();
  }

  JStringHandle javaString{(jstring) env->CallStaticObjectMethod(clazz, method), env};
  return JNIUtils::getStringFromJString(env, javaString.get());
}

std::string JNIUtils::getStringFromObjectMethod(JNIEnv* env, const jclass clazz,
                                                const jobject jobj, const char* name,
                                                const char* sig)
{
  if (nullptr == env) {
    return std::string();
  }

  jmethodID method = env->GetMethodID(clazz, name, sig);
  return getStringFromObjectMethod(env, jobj, method);
}

std::string JNIUtils::getStringFromObjectMethod(JNIEnv* env, const jobject jobj,
                                                const jmethodID method)
{
  if (nullptr == env) {
    return std::string();
  }

  JStringHandle javaString{(jstring)env->CallObjectMethod(jobj, method), env};
  return JNIUtils::getStringFromJString(env, javaString.get());
}

jobject JNIUtils::getUnityActivity(JNIEnv* env)
{
  if (nullptr == env) {
    return nullptr;
  }

  JClassHandle unityPlayer{env->FindClass("com/unity3d/player/UnityPlayer"), env};
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return nullptr;
  }
  if (nullptr == unityPlayer) {
    return nullptr;
  }
  jfieldID activityField = env->GetStaticFieldID(unityPlayer.get(), "currentActivity", "Landroid/app/Activity;");
  if (nullptr == activityField) {
    return nullptr;
  }
  jobject activity = env->GetStaticObjectField(unityPlayer.get(), activityField);

  return activity;
}

jobject JNIUtils::getCurrentActivity(JNIEnv* env)
{
  if (nullptr == env) {
    return nullptr;
  }

  jobject activity = getUnityActivity(env);

  if (nullptr == activity) {
    activity = env->NewLocalRef(_sCurrentActivity);
  }

  return activity;
}

void JNIUtils::SetCurrentActivity(JNIEnv* env, const jobject activity)
{
  if (_sCurrentActivity) {
    env->DeleteGlobalRef(_sCurrentActivity);
    _sCurrentActivity = (jobject) 0;
  }
  if (activity) {
    _sCurrentActivity = env->NewGlobalRef(activity);
  }
}

// JNIEnvWrapper implementation

class JNIEnvWrapperImpl : public JNIEnvWrapper {
public:
  JNIEnvWrapperImpl(JavaVM* jvm)
  : _detachOnDestroy(false)
  , _jvm(jvm)
  , _env(nullptr)
  {
    if (_jvm == nullptr) {
      PRINT_NAMED_ERROR("JNIEnvWrapperImpl.Init", "No JVM available");
      return;
    }

    // get JNI environment
    _jvm->GetEnv((void**)&_env, JNI_VERSION_1_6);
    if (_env == nullptr) {
      _detachOnDestroy = true;
      _jvm->AttachCurrentThread(&_env, nullptr);
    }
    if (_env == nullptr) {
      PRINT_NAMED_ERROR("JNIEnvWrapperImpl.Init", "Could not attach to JVM");
      _detachOnDestroy = false;
    }
  }

  virtual ~JNIEnvWrapperImpl() {
    if (_detachOnDestroy) {
      _jvm->DetachCurrentThread();
    }
  }

  virtual JNIEnv* GetEnv() override { return _env; }

private:
  bool _detachOnDestroy;
  JavaVM* _jvm;
  JNIEnv* _env;
};

std::unique_ptr<JNIEnvWrapper> JNIUtils::getJNIEnvWrapper()
{
  return std::unique_ptr<JNIEnvWrapper>{new JNIEnvWrapperImpl(_sJvm)};
}

}
}

#endif // ANDROID
