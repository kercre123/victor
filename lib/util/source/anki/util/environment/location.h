//
//  location.h
//
//  Created by Stuart Eichert on 6/5/2017
//  Copyright (c) 2017 Anki, Inc. All rights reserved.
//

#ifndef __util_environment_location_H__
#define __util_environment_location_H__

#include "locale.h"
#include "util/http/abstractHttpAdapter.h"

#include <string>

namespace Anki {
namespace Util {

class Location {
public:
  struct ProviderConfig {
    std::string url;
    std::string authHeaderValue;
    std::string workingDir;
    IHttpAdapter* httpAdapter;
    bool expireCache;
  };

  static void StartProvider(const ProviderConfig& config);

  static bool GetCurrentLocation(Location& location);

  Location() : Location(Locale::kDefaultCountry) { }
  Location(Locale locale) : Location(locale.GetCountry()) { }
  Location(Locale::CountryISO2 country)
    : _country(country) { }

  Locale::CountryISO2 GetCountry() const { return _country; }

private:
  Locale::CountryISO2 _country;
};

} // namespace Util
} // namespace Anki

#endif // __util_environment_location_H__
