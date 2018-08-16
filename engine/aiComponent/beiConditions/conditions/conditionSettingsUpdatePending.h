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

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionSettingsUpdatePending_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionSettingsUpdatePending_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "json/json-forwards.h"
#include "clad/types/robotSettingsTypes.h"


namespace Anki {
namespace Vector {

class ConditionSettingsUpdatePending : public IBEICondition
{
public:

  explicit ConditionSettingsUpdatePending( const Json::Value& config );


protected:

  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& ) const override;


private:

  bool          _isSettingSpecified;
  RobotSetting  _setting;

};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionSettingsUpdatePending_H__
