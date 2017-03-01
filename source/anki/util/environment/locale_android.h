/**
 * File: locale_android.h
 *
 * Author: seichert
 * Created: 03/24/2016
 *
 * Description: Android specific locale routines
 *
 * Copyright: Anki, Inc.
 *
 **/

#ifndef __util_environment_locale_android_H__
#define __util_environment_locale_android_H__

#include "locale.h"

namespace Anki {
namespace Util {

Locale GetCurrentLocaleAndroid();

Locale GetCurrentLocaleAndroid(void *jniEnv);

} // namespace Util
} // namespace Anki

#endif // __util_environment_locale_android_H__
