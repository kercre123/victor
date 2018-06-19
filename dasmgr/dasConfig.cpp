/**
* File: victor/dasmgr/dasConfig.cpp
*
* Description: DASConfig class implementation
*
* Copyright: Anki, inc. 2018
*
*/

#include "dasConfig.h"
#include "json/json.h"
#include "util/logging/logging.h"
#include <fstream>

namespace Anki {
namespace Victor {

DASConfig::DASConfig(const std::string & url, size_t queue_threshold_size, size_t max_deferrals_size) :
  _url(url),
  _queue_threshold_size(queue_threshold_size),
  _max_deferrals_size(max_deferrals_size)
{
}



std::unique_ptr<DASConfig> DASConfig::GetDASConfig(const Json::Value & json)
{
  if (!json.isObject()) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidJSON", "Invalid json object");
    return nullptr;
  }

  const auto & dasConfig = json["dasConfig"];
  if (!dasConfig.isObject()) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidDASConfig", "Invalid dasConfig");
    return nullptr;
  }

  const auto & url = dasConfig["url"];
  if (!url.isString()) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidURL", "Invalid url attribute");
    return nullptr;
  }

  const auto & queue_threshold_size = dasConfig["queue_threshold_size"];
  if (!queue_threshold_size.isUInt()) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidQueueThresholdSize", "Invalid queue_threshold_size attribute");
    return nullptr;
  }

  const auto & max_deferrals_size = dasConfig["max_deferrals_size"];
  if (!max_deferrals_size.isUInt()) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidMaxDeferralsSize", "Invalid max_deferrals_size attribute");
    return nullptr;
  }

  return std::make_unique<DASConfig>(url.asString(), queue_threshold_size.asUInt(), max_deferrals_size.asUInt());
}

std::unique_ptr<DASConfig> DASConfig::GetDASConfig(const std::string & path)
{
  std::ifstream ifstream(path);
  Json::Reader reader;
  Json::Value json;

  if (!reader.parse(ifstream, json)) {
    LOG_ERROR("DASConfig.GetDASConfig.InvalidJsonFile", "Unable to parse json from %s", path.c_str());
    return nullptr;
  }

  return GetDASConfig(json);
}

} // end namespace Victor
} // end namespace Anki
