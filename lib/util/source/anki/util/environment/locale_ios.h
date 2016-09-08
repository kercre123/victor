/**
 * File: locale_ios.h
 *
 * Author: seichert
 * Created: 03/24/2016
 *
 * Description: iOS specific locale routines
 *
 * Copyright: Anki, Inc.
 *
 **/

#ifndef __util_environment_locale_ios_H__
#define __util_environment_locale_ios_H__

#include "locale.h"

namespace Anki {
namespace Util {

Locale GetCurrentLocaleIOS();

} // namespace Util
} // namespace Anki

#endif // __util_environment_locale_ios_H__
