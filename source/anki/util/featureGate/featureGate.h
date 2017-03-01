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

#ifndef __Util_FeatureGate_H_
#define __Util_FeatureGate_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Anki {
namespace Util {

class FeatureGate
{
public:
  void Init(const std::string& jsonContents);

  bool IsFeatureEnabled(const std::string& featureName) const;

  using FeaturePair = std::pair<const char*, bool>;
  using FeatureList = std::vector<FeaturePair>;
  FeatureList GetFeatures() const;

protected:
  std::map<std::string, bool> _features;
};

} // namespace Util
} // namespace Anki


#endif // __Util_FeatureGate_H_
