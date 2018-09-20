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

#include "clad/types/featureGateTypes.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

#define FEATURE_OVERRIDES_ENABLED ( REMOTE_CONSOLE_ENABLED )
  
#if FEATURE_OVERRIDES_ENABLED

namespace {

  // don't change the order of this, else your overrides will be wrong
  enum FeatureTypeOverride
  {
    Default = 0, // no override
    Enabled, // feature is enabled
    Disabled // feature is disabled
  };

  static FeatureTypeOverride sFeatureTypeOverrides[FeatureTypeNumEntries];
  static std::string sOverrideSaveFilename;

  void PrintFeatureOverrides()
  {
    for ( uint8_t i = 0; i < FeatureTypeNumEntries; ++i )
    {
      const char* status = nullptr;
      switch ( sFeatureTypeOverrides[i] )
      {
        case FeatureTypeOverride::Default:
          continue;

        case FeatureTypeOverride::Enabled:
          status = "Enabled";
          break;

        case FeatureTypeOverride::Disabled:
          status = "Disabled";
          break;

        default:
          break;
      }

      // printing as a warning so it stands out in the console to remind people they have things overridden
      PRINT_NAMED_WARNING( "FeatureGate.Override", "[%s] is %s",
                           EnumToString(static_cast<FeatureType>(i)), status );
    }
  }

  void LoadFeatureOverrides()
  {
    Json::Reader reader;
    Json::Value data;

    const std::string fileContents = Util::FileUtils::ReadFile( sOverrideSaveFilename );
    if ( !fileContents.empty() && reader.parse( fileContents, data ) )
    {
      for ( uint8_t i = 0; i < FeatureTypeNumEntries; ++i )
      {
        const FeatureType feature = static_cast<FeatureType>(i);
        const char* featureString = FeatureTypeToString( feature );

        const Json::Value& overrideTypeJson = data[featureString];
        if ( !overrideTypeJson.isNull() )
        {
          sFeatureTypeOverrides[i] = static_cast<FeatureTypeOverride>(overrideTypeJson.asUInt());
        }
      }

      // if we've loaded something, it means we have overrides, so print them so people are aware they have something set
      PrintFeatureOverrides();
    }
  }

  void SaveFeatureOverrides()
  {
    Json::Value data;

    bool isOverrideSet = false;
    for ( uint8_t i = 0; i < FeatureTypeNumEntries; ++i )
    {
      const FeatureType feature = static_cast<FeatureType>(i);
      const char* featureString = FeatureTypeToString( feature );

      data[featureString] = static_cast<Json::Value::UInt>(sFeatureTypeOverrides[i]);

      isOverrideSet |= ( FeatureTypeOverride::Default != sFeatureTypeOverrides[i] );
    }

    if ( isOverrideSet )
    {
      Util::FileUtils::WriteFile( sOverrideSaveFilename, data.toStyledString() );
    }
    else
    {
      // if we have nothing set, remove the file (since this will be the default state mainly)
      Util::FileUtils::DeleteFile( sOverrideSaveFilename );
    }
  }

  void InitFeatureOverrides( Util::Data::DataPlatform* platform )
  {
    // default everything to "Default", which means no override
    for ( uint8_t i = 0; i < FeatureTypeNumEntries; ++i )
    {
      sFeatureTypeOverrides[i] = FeatureTypeOverride::Default;
    }

    const std::string& fileName = "featureGateOverrides.ini";
    sOverrideSaveFilename = platform->pathToResource( Anki::Util::Data::Scope::Cache, fileName );

    LoadFeatureOverrides();
  }

}

static const char* kConsoleFeatureGroup = "FeatureGate";
static const char* kFeatureEnumString =
  "Invalid,"
  "Laser,"
  "Exploring,"
  "FindCube,"
  "Keepaway,"
  "ReactToHeldCube,"
  "RollCube,"
  "MoveCube,"
  "Messaging,"
  "ReactToIllumination,"
  "KnowledgeGraph,"
  "Dancing,"
  "CubeSpinner,"
  "GreetAfterLongTime,"
  "AttentionTransfer,"
  "UserDefinedBehaviorTree,"
  "PopAWheelie,"
  "PRDemo";

CONSOLE_VAR_ENUM( uint8_t,  kFeatureToEdit,   kConsoleFeatureGroup,  0, kFeatureEnumString );

void EnableFeature( ConsoleFunctionContextRef context )
{
  sFeatureTypeOverrides[kFeatureToEdit] = FeatureTypeOverride::Enabled;
  PRINT_NAMED_DEBUG( "FeatureGate.Override", "Enabling feature %s",
                     EnumToString(static_cast<FeatureType>(kFeatureToEdit)) );

  SaveFeatureOverrides();
}
CONSOLE_FUNC( EnableFeature, kConsoleFeatureGroup );

void DisableFeature( ConsoleFunctionContextRef context )
{
  sFeatureTypeOverrides[kFeatureToEdit] = FeatureTypeOverride::Disabled;
  PRINT_NAMED_DEBUG( "FeatureGate.Override", "Disabling feature %s",
                     EnumToString(static_cast<FeatureType>(kFeatureToEdit)) );

  SaveFeatureOverrides();
}
CONSOLE_FUNC( DisableFeature, kConsoleFeatureGroup );

void DefaultFeature( ConsoleFunctionContextRef context )
{
  sFeatureTypeOverrides[kFeatureToEdit] = FeatureTypeOverride::Default;
  PRINT_NAMED_DEBUG( "FeatureGate.Override", "Removing override for feature %s",
                     EnumToString(static_cast<FeatureType>(kFeatureToEdit)) );

  SaveFeatureOverrides();
}
CONSOLE_FUNC( DefaultFeature, kConsoleFeatureGroup );

void DefaultAllFeatures( ConsoleFunctionContextRef context )
{
  for ( uint8_t i = 0; i < FeatureTypeNumEntries; ++i )
  {
    if ( FeatureTypeOverride::Default != sFeatureTypeOverrides[i] )
    {
      sFeatureTypeOverrides[i] = FeatureTypeOverride::Default;
      PRINT_NAMED_DEBUG( "FeatureGate.Override", "Removing override for feature %s",
                        EnumToString(static_cast<FeatureType>(i)) );
    }
  }

  SaveFeatureOverrides();
}
CONSOLE_FUNC( DefaultAllFeatures, kConsoleFeatureGroup );

#endif // FEATURE_OVERRIDES_ENABLED


CozmoFeatureGate::CozmoFeatureGate( Util::Data::DataPlatform* platform )
{
  #if FEATURE_OVERRIDES_ENABLED
  {
    InitFeatureOverrides( platform );
  }
  #endif
}

bool CozmoFeatureGate::IsFeatureEnabled(FeatureType feature) const
{
  #if FEATURE_OVERRIDES_ENABLED
  {
    const uint8_t featureIndex = static_cast<uint8_t>(feature);
    if ( FeatureTypeOverride::Default != sFeatureTypeOverrides[featureIndex] )
    {
      return ( FeatureTypeOverride::Enabled == sFeatureTypeOverrides[featureIndex] );
    }
  }
  #endif

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
