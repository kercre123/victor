/**
 * File: locale_android.cpp
 *
 * Author: seichert
 * Created: 03/24/2016
 *
 * Description: Android specific locale routines
 *
 * Copyright: Anki, Inc.
 *
 **/

#include "locale_android.h"

#include "util/helpers/ankiDefines.h"
#if !defined(ANKI_PLATFORM_ANDROID)
#error This file is meant to be included for Android only
#endif

#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Util {

Locale GetCurrentLocaleAndroid(void *jniEnv)
{
  JNIEnv* env = (JNIEnv*) jniEnv;
  if (!env) {
    return Locale::kDefaultLocale;
  }

  JClassHandle localeClassHandle{env->FindClass("java/util/Locale"), env};
  if (localeClassHandle == nullptr) {
    PRINT_NAMED_ERROR("GetCurrentLocaleAndroid.Init.ClassNotFound", 
      "Unable to find java.util.Locale");
    return Locale::kDefaultLocale;
  }

  jmethodID getDefaultMethodID =
    env->GetStaticMethodID(localeClassHandle.get(), "getDefault", "()Ljava/util/Locale;");
  jmethodID getLanguageMethodID =
    env->GetMethodID(localeClassHandle.get(), "getLanguage", "()Ljava/lang/String;");
  jmethodID getCountryMethodID =
    env->GetMethodID(localeClassHandle.get(), "getCountry", "()Ljava/lang/String;");
  if (!getDefaultMethodID || !getLanguageMethodID || !getCountryMethodID) {
    return Locale::kDefaultLocale;
  }

  JObjectHandle localeObjHandle{env->CallStaticObjectMethod(localeClassHandle.get(), getDefaultMethodID, NULL), env};
  if (localeObjHandle == nullptr) {
    PRINT_NAMED_ERROR("GetCurrentLocaleAndroid.Init.ObjectNotFound", 
      "Unable to find current locale");
    return Locale::kDefaultLocale;
  }

  std::string languageString =
    JNIUtils::getStringFromObjectMethod(env, localeObjHandle.get(), getLanguageMethodID);
  std::string countryString =
    JNIUtils::getStringFromObjectMethod(env, localeObjHandle.get(), getCountryMethodID);

  Locale locale(languageString, countryString);

  return locale;
}

Locale GetCurrentLocaleAndroid()
{
  if (ANKI_USE_JNI) {
    auto envWrapper = JNIUtils::getJNIEnvWrapper();
    JNIEnv* env = envWrapper->GetEnv();
    if (env != nullptr) {
      return GetCurrentLocaleAndroid((void *) env);
    } else {
      PRINT_NAMED_ERROR("GetCurrentLocaleAndroid_env_null", "");
      return Locale::kDefaultLocale;
    }
  } else {
    PRINT_NAMED_WARNING("GetCurrentLocaleAndroid.jni_disabled",
                        "%s", "JNI locale not available");
    return Locale::kDefaultLocale;
  }

}

} // namespace Util
} // namespace Anki
