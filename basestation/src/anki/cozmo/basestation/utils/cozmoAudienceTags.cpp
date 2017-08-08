/**
 * File: cozmoAudienceTags
 *
 * Author: baustin
 * Created: 7/24/17
 *
 * Description: Light wrapper for AudienceTags to initialize it with Cozmo-specific configuration
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/utils/cozmoAudienceTags.h"

#include "anki/cozmo/basestation/cozmoContext.h"
#include "util/environment/locale.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Cozmo {

CozmoAudienceTags::CozmoAudienceTags(const CozmoContext* context)
{
  // Define audience tags that will be used in Cozmo and provide handlers to determine if they apply

  auto localeLanguageHandler = [context] {
    Util::Locale* locale = context->GetLocale();
    std::string locale_language_tag =
      "locale_language_" + Util::StringToLower(locale->GetLanguageString());
    return locale_language_tag;
  };
  RegisterDynamicTag(localeLanguageHandler);

  auto localeCountryHandler = [context] {
    Util::Locale* locale = context->GetLocale();
    std::string locale_country_tag =
      "locale_country_" + Util::StringToLower(locale->GetCountryString());
    return locale_country_tag;
  };
  RegisterDynamicTag(localeCountryHandler);
}

}
}
