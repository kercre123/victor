/**
* File: conditionSettingsUpdatePending.h
*
* Author: Jarrod Hatfield
* Created: 2018-08-15
*
* Description: Strategy that checks to see if a settings udpate change has been requested
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionSettingsUpdatePending.h"

#include "engine/components/settingsManager.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "coretech/common/engine/jsonTools.h"


namespace Anki {
namespace Vector {

namespace {
  const char* kKeyRobotSetting = "setting";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionSettingsUpdatePending::ConditionSettingsUpdatePending( const Json::Value& config ) :
  IBEICondition( config ),
  _isSettingSpecified( false )
{
  std::string settingString;
  if ( JsonTools::GetValueOptional( config, kKeyRobotSetting, settingString ) )
  {
    _isSettingSpecified = external_interface::RobotSetting_Parse( settingString, &_setting );
    ANKI_VERIFY( _isSettingSpecified, "ConditionSettingsUpdatePending.BadConfig",
                "Invalid setting specified '%s'",
                settingString.c_str() );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionSettingsUpdatePending::AreConditionsMetInternal( BehaviorExternalInterface& bei ) const
{
  if ( bei.HasSettingsManager() )
  {
    const SettingsManager& settings = bei.GetSettingsManager();
    return _isSettingSpecified ? settings.IsSettingsUpdateRequestPending() : settings.IsSettingsUpdateRequestPending( _setting );
  }

  return false;
}

}
}
