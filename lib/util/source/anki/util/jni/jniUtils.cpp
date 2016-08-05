#ifdef ANDROID

#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Util {

JavaVM* JNIUtils::_sJvm = nullptr;

jobjectArray JNIUtils::convertStringMapToJObjectArray(JNIEnv* env, const std::map<std::string,std::string>& stringMap, std::vector<jstring>& stringRefs)
{
  jclass clazz = env->FindClass("java/lang/String");
  jobjectArray arr = env->NewObjectArray((jsize)(stringMap.size()*2), clazz, 0);
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
  env->DeleteLocalRef(clazz);
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
  jstring javaString = (jstring)env->GetStaticObjectField(clazz, fieldId);

  std::string s = getStringFromJString(env, javaString);
  env->DeleteLocalRef(javaString);
  return s;
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

  jstring javaString = (jstring) env->CallStaticObjectMethod(clazz, method);
  std::string s = JNIUtils::getStringFromJString(env, javaString);
  env->DeleteLocalRef(javaString);
  return s;
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

  jstring javaString = (jstring) env->CallObjectMethod(jobj, method);
  std::string s = JNIUtils::getStringFromJString(env, javaString);
  env->DeleteLocalRef(javaString);
  return s;
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
