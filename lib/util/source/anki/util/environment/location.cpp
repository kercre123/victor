//
//  location.cpp
//
//  Created by Stuart Eichert on 6/5/2017
//  Copyright (c) 2017 Anki, Inc. All rights reserved.
//

#include "location.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include "json/json.h"
#include <mutex>
#include <string>

namespace Anki {
namespace Util {

static const std::string kCachedLocationFileName = "lastCachedLocation.json";
static const std::string kCachedLocationTmpFileName = "lastCachedLocation.json.tmp";

static std::recursive_mutex sLocationMutex;

static Location::ProviderConfig sProviderConfig;
static std::string sCachedLocationFilePath;

static bool sHaveCachedLocation = false;
static Location sCachedLocation;

static Dispatch::QueueHandle sDispatchQueue;

static bool ParseLocationFromJson(const Json::Value& jsonLocation, Location& location)
{
  Json::Value countryObj = jsonLocation.get("country", Json::Value(Json::objectValue));
  std::string isoCodeStr = countryObj.get("iso_code", Json::Value(Json::stringValue)).asString();
  if (isoCodeStr.empty() || (isoCodeStr.size() != 2)) {
    return false;
  }
  Locale::CountryISO2 country = Locale::CountryISO2FromString(isoCodeStr);
  location = Location(country);
  return true;
}

static bool ParseLocationFromJsonString(const std::string& jsonString, Location& location)
{
  Json::Reader reader;
  Json::Value jsonLocation;

  if (jsonString.empty()) {
    return false;
  }

  bool parsingSuccessful = reader.parse(jsonString, jsonLocation, false);
  if (!parsingSuccessful) {
    return false;
  }
  return ParseLocationFromJson(jsonLocation, location);
}

static bool ParseLocationFromJsonFile(const std::string& filePath, Location& location)
{
  if (!FileUtils::FileExists(filePath)) {
    return false;
  }

  const std::string& contents = StringFromContentsOfFile(filePath);
  return ParseLocationFromJsonString(contents, location);
}

void Location::StartProvider(const Location::ProviderConfig& config)
{
  std::lock_guard<std::recursive_mutex> lock(sLocationMutex);
  sProviderConfig = config;
  FileUtils::CreateDirectory(sProviderConfig.workingDir);
  sCachedLocationFilePath =
    FileUtils::FullFilePath({sProviderConfig.workingDir, kCachedLocationFileName});
  if (config.expireCache) {
    sHaveCachedLocation = false;
    FileUtils::DeleteFile(sCachedLocationFilePath);
  } else {
    sHaveCachedLocation = ParseLocationFromJsonFile(sCachedLocationFilePath, sCachedLocation);
    if (!sHaveCachedLocation) {
      FileUtils::DeleteFile(sCachedLocationFilePath);
    }
  }

  HttpRequestCallback callback =
  [] (const HttpRequest& request,
      const int responseCode,
      const std::map<std::string, std::string>& responseHeaders,
      const std::vector<uint8_t>& responseBody) {

      if (!isHttpSuccessCode(responseCode)) {
        Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "util.location.fetch_failed.bad_http_response",
                              {{DDATA, std::to_string(responseCode).c_str()}},
                              "%s", request.uri.c_str());

        return;
      }

      const std::string& downloadPath = request.storageFilePath;
      Location location;
      bool parsingSuccess = ParseLocationFromJsonFile(downloadPath, location);
      if (!parsingSuccess) {
        PRINT_NAMED_INFO("util.location.failed_to_parse", "%s", request.uri.c_str());
        FileUtils::DeleteFile(downloadPath);
        return;
      }

      std::lock_guard<std::recursive_mutex> lock(sLocationMutex);
      (void) rename(downloadPath.c_str(), sCachedLocationFilePath.c_str());
      sCachedLocation = location;
      sHaveCachedLocation = true;
  };

  if (!sProviderConfig.url.empty() && !sProviderConfig.authHeaderValue.empty()) {
    HttpRequest httpRequest;
    httpRequest.method = HttpMethodGet;
    httpRequest.uri = sProviderConfig.url;
    httpRequest.storageFilePath =
      FileUtils::FullFilePath({sProviderConfig.workingDir, kCachedLocationTmpFileName});
    httpRequest.headers["Authorization"] = sProviderConfig.authHeaderValue;
    if (sDispatchQueue.get() == nullptr) {
      sDispatchQueue.create("LocationCallback");
    }
    sProviderConfig.httpAdapter->StartRequest(std::move(httpRequest),
                                              sDispatchQueue.get(),
                                              callback);
  }

  return;
}

bool Location::GetCurrentLocation(Location& location)
{
  std::lock_guard<std::recursive_mutex> lock(sLocationMutex);

  if (sHaveCachedLocation) {
    location = sCachedLocation;
  } else {
    PRINT_NAMED_INFO("util.get_current_location.do_not_have_cached_location", "");
  }

  return sHaveCachedLocation;
}

} // namespace Util
} // namespace Anki
