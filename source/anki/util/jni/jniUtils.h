#ifndef __Anki_Util_Jni_JniUtils_H__
#define __Anki_Util_Jni_JniUtils_H__

#include "util/jni/includeJni.h"
#include <functional>
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
  virtual jclass FindClassInProject(const char* className) = 0;
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
  static jobject getCurrentActivity(JNIEnv* env);

  static std::unique_ptr<JNIEnvWrapper> getJNIEnvWrapper();

  static JavaVM* GetJvm() { return _sJvm; }
  static void SetJvm(JavaVM* jvm, const char* randomProjectClass = nullptr);
  static bool AcquireClassLoaderWithClass(const char* randomProjectClass);
  static void SetCurrentActivity(JNIEnv* env, const jobject activity);

private:
  static JavaVM* _sJvm;
  static jobject _sCurrentActivity;

  JNIUtils() = delete;
};


// Smart handle for jobject/jclass local ref that will delete itself
struct JObjDefaultInitializer;
template <typename jobj, typename deleter, typename initializer = JObjDefaultInitializer>
class JObjHandleBase {
public:
  JObjHandleBase(jobj obj, JNIEnv* env)
  : _handle(initializer::init(obj, env), deleter::get_deleter(env)) {}
  JObjHandleBase(JObjHandleBase&& other) = default;
  JObjHandleBase& operator=(JObjHandleBase&& other) = default;
  jobj operator->() const { return get(); }
  jobj get() const { return static_cast<jobj>(_handle.get()); }

  bool operator==(std::nullptr_t val) const { return _handle == val; }
  bool operator!=(std::nullptr_t val) const { return _handle != val; }

protected:
  JObjHandleBase() {} // only for use by subclasses

  using jobject_base = typename std::remove_pointer<jobject>::type;
  std::unique_ptr<jobject_base, std::function<void(jobject)>> _handle;

  // sanity check
  static_assert(std::is_pointer<jobject>::value, "jobject not a pointer?");

  // requirements for template types
  static_assert(std::is_pointer<jobj>::value, "object type must be a pointer");
  using jtemplate_base = typename std::remove_pointer<jobj>::type;
  static_assert(std::is_base_of<jobject_base, jtemplate_base>::value, "object type must be derived from jobject");
};

template <typename T, typename D, typename I>
inline bool operator==(std::nullptr_t val, const JObjHandleBase<T, D, I>& obj) { return obj == val; }
template <typename T, typename D, typename I>
inline bool operator!=(std::nullptr_t val, const JObjHandleBase<T, D, I>& obj) { return obj != val; }

// Deleter for local references
struct JObjLocalDeleter {
  static std::function<void(jobject)> get_deleter(JNIEnv* env) {
    return [env] (jobject toDelete) {
      env->DeleteLocalRef(toDelete);
    };
  }
};

// Deleter for global references
struct JObjGlobalDeleter {
  static std::function<void(jobject)> get_deleter(JNIEnv* env) {
    return [] (jobject toDelete) {
      auto wrapper = JNIUtils::getJNIEnvWrapper();
      JNIEnv* env = wrapper->GetEnv();
      if (env != nullptr) {
        env->DeleteGlobalRef(toDelete);
      }
    };
  }
};

// Default handle initializer is a no-op for local references...
struct JObjDefaultInitializer {
  static jobject init(jobject obj, JNIEnv* env) {
    return obj;
  }
};

// For global references, initializer creates a new global reference
struct JObjGlobalInitializer {
  static jobject init(jobject obj, JNIEnv* env) {
    return env->NewGlobalRef(obj);
  }
};


template <typename T>
using JObjectLocalHandle = JObjHandleBase<T, JObjLocalDeleter>;

using JObjectHandle = JObjectLocalHandle<jobject>;
using JClassHandle = JObjectLocalHandle<jclass>;
using JStringHandle = JObjectLocalHandle<jstring>;
using JArrayHandle = JObjectLocalHandle<jarray>;
using JObjectArrayHandle = JObjectLocalHandle<jobjectArray>;
using JByteArrayHandle = JObjectLocalHandle<jbyteArray>;

// For global handles, we want to add the ability to set them after construction, which
// means giving them a default constructor
#define HandleBase JObjHandleBase<jobj, JObjGlobalDeleter, JObjGlobalInitializer>
template <typename jobj>
class JObjGlobalBase : public HandleBase
{
public:
  JObjGlobalBase() {}
  JObjGlobalBase(jobj obj, JNIEnv* env) : HandleBase(obj, env) {}
};
#undef HandleBase

using JGlobalObject = JObjGlobalBase<jobject>;

}
}

#endif
