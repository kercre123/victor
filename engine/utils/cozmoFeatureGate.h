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

#ifndef ANKI_COZMO_BASESTATION_COZMO_FEATURE_GATE_H
#define ANKI_COZMO_BASESTATION_COZMO_FEATURE_GATE_H

#include "util/featureGate/featureGate.h"

namespace Anki {

namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Vector {

enum class FeatureType : uint8_t;

class CozmoFeatureGate : public Util::FeatureGate
{
public:
  CozmoFeatureGate( Util::Data::DataPlatform* platform );

  bool IsFeatureEnabled(FeatureType feature) const;
  void SetFeatureEnabled(FeatureType feature, bool enabled);
};

}
}

#endif
