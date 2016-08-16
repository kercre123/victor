#ifndef __Anki_Util_Jni_JniUtils_H__
#define __Anki_Util_Jni_JniUtils_H__

#include "util/jni/includeJni.h"
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace Anki {
namespace Util {

class JNIEnvWrapper {
public:
  virtual JNIEnv* GetEnv() = 0;
  virtual ~JNIEnvWrapper() {}
};

// Class of static helper functions
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
  static jobject getUnityActivity(JNIEnv* env);

  static std::unique_ptr<JNIEnvWrapper> getJNIEnvWrapper();

  static JavaVM* GetJvm() { return _sJvm; }
  static void SetJvm(JavaVM* jvm) { _sJvm = jvm; }

private:
  static JavaVM* _sJvm;

  JNIUtils() = delete;
};

// Smart handle for jobject/jclass local ref that will delete itself
template <typename jobj>
class JObjHandleBase {
public:
  JObjHandleBase(jobj obj, JNIEnv* env)
  : _handle(obj, [env] (jobj toDelete) {
    env->DeleteLocalRef(toDelete);
  }) {}
  JObjHandleBase(JObjHandleBase&& other) = default;
  JObjHandleBase& operator=(JObjHandleBase&& other) = default;
  jobj operator->() const { return _handle.get(); }
  jobj get() const { return _handle.get(); }

  bool operator==(std::nullptr_t val) const { return _handle == val; }
  bool operator!=(std::nullptr_t val) const { return _handle != val; }
  bool operator==(const JObjHandleBase& other) const { return _handle == other._handle; }
  bool operator!=(const JObjHandleBase& other) const { return _handle != other._handle; }

private:
  using jtemplate_base = typename std::remove_pointer<jobj>::type;
  std::unique_ptr<jtemplate_base, typename std::function<void(jobj)>> _handle;

  // sanity check
  static_assert(std::is_pointer<jobject>::value, "jobject not a pointer?");

  // requirements for template types
  static_assert(std::is_pointer<jobj>::value, "object type must be a pointer");
  using jobject_base = typename std::remove_pointer<jobject>::type;
  static_assert(std::is_base_of<jobject_base, jtemplate_base>::value, "object type must be derived from jobject");
};

template <typename T>
inline bool operator==(std::nullptr_t val, const JObjHandleBase<T>& obj) { return obj == val; }
template <typename T>
inline bool operator!=(std::nullptr_t val, const JObjHandleBase<T>& obj) { return obj != val; }

using JObjectHandle = JObjHandleBase<jobject>;
using JClassHandle = JObjHandleBase<jclass>;
using JStringHandle = JObjHandleBase<jstring>;
using JArrayHandle = JObjHandleBase<jarray>;
using JObjectArrayHandle = JObjHandleBase<jobjectArray>;
using JByteArrayHandle = JObjHandleBase<jbyteArray>;

}
}

#endif
