/**
 * File: cozmoFeatureGate
 *
 * Author: baustin
 * Created: 6/15/16
 *
 * Description: Light wrapper for FeatureGate to initialize it with Cozmo-specific configuration
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/utils/cozmoFeatureGate.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "clad/types/featureGateTypes.h"
#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

bool CozmoFeatureGate::IsFeatureEnabled(FeatureType feature) const
{
  std::string featureEnumName{FeatureTypeToString(feature)};
  std::transform(featureEnumName.begin(), featureEnumName.end(), featureEnumName.begin(), ::tolower);
  return Util::FeatureGate::IsFeatureEnabled(featureEnumName);
}

void CozmoFeatureGate::SetFeatureEnabled(Anki::Cozmo::FeatureType feature, bool enabled)
{
  std::string featureEnumName{FeatureTypeToString(feature)};
  std::transform(featureEnumName.begin(), featureEnumName.end(), featureEnumName.begin(), ::tolower);
  _features[featureEnumName] = enabled;
}

}
}
