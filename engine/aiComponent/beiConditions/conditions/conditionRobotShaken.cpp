/**
* File: strategyRobotShaken.cpp
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy for responding to robot being shaken
*
* Copyright: Anki, Inc. 2017
*
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionRobotShaken.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "coretech/common/engine/jsonTools.h"


namespace Anki {
namespace Vector {
  
namespace {
  const char* kKeyAccelMagnitude = "minAccelMagnitudeThreshold";
  const float kDefaultAccelThreshold = 16000.f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionRobotShaken::ConditionRobotShaken( const Json::Value& config ) :
  IBEICondition( config ),
  _minTriggerMagnitude( kDefaultAccelThreshold )
{
  JsonTools::GetValueOptional( config, kKeyAccelMagnitude, _minTriggerMagnitude );
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionRobotShaken::AreConditionsMetInternal( BehaviorExternalInterface& behaviorExternalInterface ) const
{
  // trigger this behavior when the filtered total accelerometer magnitude data exceeds a threshold
  const auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
  const bool isShaken = ( robotInfo.GetHeadAccelMagnitudeFiltered() >= _minTriggerMagnitude );
  const bool isOffTreads = robotInfo.IsPickedUp();
  
  // add a check for offTreadsState?
  return ( isShaken && isOffTreads );
}

} // namespace Vector
} // namespace Anki
