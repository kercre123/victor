/**
 * File: featureGate
 *
 * Author: baustin
 * Created: 1/6/16
 *
 * Description: Interface to config options specifying if features should be enabled/disabled
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "util/featureGate/featureGate.h"
#include "util/logging/logging.h"
#include "json/json.h"

#include <assert.h>

namespace Anki {
namespace Util {

void FeatureGate::Init(const std::string& jsonContents)
{
  // use permissive reader for comments
  Json::Reader reader{Json::Features::all()};
  Json::Value root;

  bool success = reader.parse(jsonContents, root);
  if (!success) {
    PRINT_NAMED_ERROR("FeatureGate.Init", "Invalid json format in feature file");
    return;
  }

  for (const Json::Value& value : root) {
    std::string featureName = value["feature"].asString();
    std::transform(featureName.begin(), featureName.end(), featureName.begin(), ::tolower);
    bool enabled = value["enabled"].asBool();
    if (_features.find(featureName) != _features.end()) {
      PRINT_NAMED_ERROR("FeatureGate.Init", "Feature '%s' is defined more than once", featureName.c_str());
    }
    _features[featureName] = enabled;
  }
}

bool FeatureGate::IsFeatureEnabled(const std::string& featureName) const
{
  auto it = _features.find(featureName);
  if (it == _features.end()) {
    // default behavior is that features not enabled in the json are disabled
    return false;
  }
  else {
    return it->second;
  }
}

FeatureGate::FeatureList FeatureGate::GetFeatures() const
{
  FeatureGate::FeatureList returnList;
  for (const auto& kv : _features) {
    returnList.emplace_back(kv.first.c_str(), kv.second);
  }
  return returnList;
}

} // namespace Util
} // namespace Anki
