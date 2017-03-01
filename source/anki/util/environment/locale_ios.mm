/**
 * File: locale_ios.mm
 *
 * Author: seichert
 * Created: 03/24/2016
 *
 * Description: iOS specific locale routines
 *
 * Copyright: Anki, Inc.
 *
 **/

#include "locale_ios.h"

#include "util/helpers/ankiDefines.h"
#if !defined(ANKI_PLATFORM_IOS)
#error This file is meant to be included for iOS only
#endif

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>

namespace Anki {
namespace Util {

Locale GetCurrentLocaleIOS()
{
  NSString* localeIdentifier = [[NSLocale currentLocale] localeIdentifier];
  NSString* localeNormalized = [localeIdentifier stringByReplacingOccurrencesOfString:@"_"
                                                                           withString:@"-"];
  NSArray* localeInfo = [localeNormalized componentsSeparatedByString:@"-"];
  NSString* lang = nil;
  NSString* country = nil;
  if (localeInfo.count == 2) {
    lang = localeInfo.firstObject;
    country = localeInfo.lastObject;
  }

  NSArray* preferredLanguages = [NSLocale preferredLanguages];
  if (preferredLanguages.count > 0) {
    lang = preferredLanguages.firstObject;

    // preferredLanguages might return an array of locales (e.g. 'de-US', 'en-US')
    // we just want the language.
    NSArray* langInfo = [lang componentsSeparatedByString:@"-"];
    if (langInfo.count > 0) {
      lang = langInfo.firstObject;
    }
  }

  std::string language;
  std::string countryCode;
  if (lang) {
    language = [lang UTF8String];
  }

  if (country) {
    countryCode = [country UTF8String];
  }

  Locale locale(language, countryCode);

  return locale;
}

} // namespace Util
} // namespace Anki
