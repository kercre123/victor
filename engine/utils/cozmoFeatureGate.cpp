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
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

#if REMOTE_CONSOLE_ENABLED

static CozmoFeatureGate* sCozmoGatePtr = nullptr;

static const char* kConsoleFeatureGroup = "FeatureGate";
static const char* kFeatureEnumString =
  "Invalid,"
  "AndroidConnectionFlow,"
  "SpeedTap,"
  "CodeLabGame,"
  "PeekABoo,"
  "SparksGatherCubes,"
  "GuardDog,"
  "Bouncer,"
  "Laser,"
  "Singing,"
  "LocalNotifications,"
  "Exploring,"
  "Messaging";

CONSOLE_VAR_ENUM( uint8_t,  kFeatureToEdit,   kConsoleFeatureGroup,  0, kFeatureEnumString );

void EnableFeature( ConsoleFunctionContextRef context )
{
  if ( nullptr != sCozmoGatePtr )
  {
    const FeatureType feature = static_cast<FeatureType>(kFeatureToEdit);
    if ( FeatureType::Invalid != feature )
    {
      sCozmoGatePtr->SetFeatureEnabled( feature, true );
      PRINT_NAMED_DEBUG( "FeatureGate.Console", "Enabling feature %s", EnumToString(feature) );
    }
  }
}
CONSOLE_FUNC( EnableFeature, kConsoleFeatureGroup );

void DisableFeature( ConsoleFunctionContextRef context )
{
  if ( nullptr != sCozmoGatePtr )
  {
    const FeatureType feature = static_cast<FeatureType>(kFeatureToEdit);
    if ( FeatureType::Invalid != feature )
    {
      sCozmoGatePtr->SetFeatureEnabled( feature, false );
      PRINT_NAMED_DEBUG( "FeatureGate.Console", "Disabling feature %s", EnumToString(feature) );
    }
  }
}
CONSOLE_FUNC( DisableFeature, kConsoleFeatureGroup );

#endif


CozmoFeatureGate::CozmoFeatureGate()
{
#if REMOTE_CONSOLE_ENABLED
  sCozmoGatePtr = this;
#endif
}

bool CozmoFeatureGate::IsFeatureEnabled(FeatureType feature) const
{
  std::string featureEnumName{FeatureTypeToString(feature)};
  std::transform(featureEnumName.begin(), featureEnumName.end(), featureEnumName.begin(), ::tolower);
  return Util::FeatureGate::IsFeatureEnabled(featureEnumName);
}

void CozmoFeatureGate::SetFeatureEnabled(FeatureType feature, bool enabled)
{
  std::string featureEnumName{FeatureTypeToString(feature)};
  std::transform(featureEnumName.begin(), featureEnumName.end(), featureEnumName.begin(), ::tolower);
  _features[featureEnumName] = enabled;
}

}
}
