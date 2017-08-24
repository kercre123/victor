#ifdef ANDROID

#include "util/helpers/ankiDefines.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

#include <android/log.h>
#define JNI_INIT_WARNING( message, args... ) __android_log_print( ANDROID_LOG_WARN, "AndroidRuntime", message, ##args )

namespace Anki {
namespace Util {

JavaVM* JNIUtils::_sJvm = nullptr;
jobject JNIUtils::_sCurrentActivity = nullptr;
  
// Keeping these hidden in the definitions file in an anonymous namespace for simplicity
namespace {
  JGlobalObject     _sClassLoader;
  jmethodID         _sFindClassMethod;
}

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
  
void JNIUtils::SetJvm(JavaVM* jvm, const char* randomProjectClass)
{
  _sJvm = jvm;
  
  if (nullptr != _sJvm && nullptr != randomProjectClass)
  {
    AcquireClassLoaderWithClass(randomProjectClass);
  }
}

bool JNIUtils::AcquireClassLoaderWithClass(const char* randomProjectClass)
{
  // Need to have a jvm set before we can use it to acquire the class loader
  if (nullptr == _sJvm)
  {
    JNI_INIT_WARNING("JNIUtils.AcquireClassLoaderWithClass.JVMNotSet");
    return false;
  }
  
  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  if (nullptr == env)
  {
    JNI_INIT_WARNING("JNIUtils.AcquireClassLoaderWithClass.JNIEnvNotFound");
    return false;
  }
  
  JClassHandle randomClass{env->FindClass(randomProjectClass), env};
  if (nullptr == randomClass)
  {
    JNI_INIT_WARNING("JNIUtils.AcquireClassLoaderWithClass.RandomProjectClassNotFound %s", randomProjectClass);
    return false;
  }
  
  // Note that the "getClassLoader" method is on the "Class" class type, hence this step.
  JClassHandle classClass{env->GetObjectClass(randomClass.get()), env};
  if (nullptr == classClass)
  {
    JNI_INIT_WARNING("JNIUtils.AcquireClassLoaderWithClass.GetClassClassFailed");
    return false;
  }
  
  JClassHandle classLoaderClass{env->FindClass("java/lang/ClassLoader"), env};
  if (nullptr == classLoaderClass)
  {
    JNI_INIT_WARNING("JNIUtils.AcquireClassLoaderWithClass.ClassLoaderClassNotFound");
    return false;
  }
  
  jmethodID getClassLoaderMethod = env->GetMethodID(classClass.get(), "getClassLoader", "()Ljava/lang/ClassLoader;");
  
  // Now we have all the things we need to get and store off the ClassLoader reference and method to find classes with it
  _sClassLoader = JGlobalObject{env->CallObjectMethod(randomClass.get(), getClassLoaderMethod), env};
  _sFindClassMethod = env->GetMethodID(classLoaderClass.get(), "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");

  return true;
}

// JNIEnvWrapper implementation

class JNIEnvWrapperImpl : public JNIEnvWrapper {
public:
  JNIEnvWrapperImpl(JavaVM* jvm)
  : _detachOnDestroy(false)
  , _jvm(jvm)
  , _env(nullptr)
  {
    if (!ANKI_USE_JNI) {
      PRINT_NAMED_WARNING("JNIEnvWrapperImpl.Init", "JVM not enabled for platform");
      return;
    }

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
  
  virtual jclass FindClassInProject(const char* className) override
  {
    if (nullptr == _env)
    {
      PRINT_NAMED_ERROR("JNIEnvWrapperImpl.FindClassInProject", "Missing JNIEnv");
      return nullptr;
    }
    
    if (nullptr == _sClassLoader)
    {
      PRINT_NAMED_ERROR("JNIEnvWrapperImpl.FindClassInProject", "Missing ClassLoader ref");
      return nullptr;
    }
    
    JStringHandle stringArg{_env->NewStringUTF(className), _env};
    return static_cast<jclass>(_env->CallObjectMethod(_sClassLoader.get(), _sFindClassMethod, stringArg.get()));
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
