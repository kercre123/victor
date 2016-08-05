#ifndef __Anki_Util_Jni_JniUtils_H__
#define VALUEOBJECTS_JGLOBALOBJECTS_JNIUTILS_H

#include "util/jni/includeJni.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Anki {
namespace Util {

class JNIEnvWrapper {
public:
  virtual JNIEnv* GetEnv() = 0;
  virtual ~JNIEnvWrapper() {}
};

class JNIUtils {
public:
  static std::string getStringFromJString(JNIEnv* env, jstring jString);
  static jobjectArray convertStringMapToJObjectArray(JNIEnv* env, const std::map<std::string,std::string>& stringMap, std::vector<jstring>& stringRefs);
  static bool getStaticBooleanFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName);
  static int getStaticIntFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName);
  static std::string getStaticStringFieldFromClass(JNIEnv* env, const jclass clazz, const char* fieldName);
  static std::string getStringFromStaticObjectMethod(JNIEnv* env, const jclass clazz,
                                                     const char* name, const char* sig);
  static std::string getStringFromStaticObjectMethod(JNIEnv* env, const jclass clazz,
                                                     const jmethodID method);
  static std::string getStringFromObjectMethod(JNIEnv* env, const jclass clazz, const jobject jobj,
                                               const char* name, const char* sig);
  static std::string getStringFromObjectMethod(JNIEnv* env, const jobject jobj,
                                               const jmethodID method);

  static std::unique_ptr<JNIEnvWrapper> getJNIEnvWrapper();

  static JavaVM* GetJvm() { return _sJvm; }
  static void SetJvm(JavaVM* jvm) { _sJvm = jvm; }

private:
  static JavaVM* _sJvm;

  JNIUtils() = delete;
};

}
}

#endif
