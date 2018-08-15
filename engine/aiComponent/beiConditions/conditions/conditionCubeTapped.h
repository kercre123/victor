/***********************************************************************************************************************
 *
 *  ConditionCubeTapped
 *  Victor
 *
 *  Created by Jarrod Hatfield on 4/02/2018
 *
 *  Description
 *  + BEI Condition that listens for cube tap events within a specified response time
 *
 **********************************************************************************************************************/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCubeTapped_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCubeTapped_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"
#include "coretech/common/engine/objectIDs.h"


#include <set>

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

namespace ExternalInterface {
struct ObjectTapped;
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ConditionCubeTapped : public IBEICondition, private IBEIConditionEventHandler
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  ConditionCubeTapped( const Json::Value& config );
  virtual ~ConditionCubeTapped();


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual void InitInternal( BehaviorExternalInterface& behaviorExternalInterface ) override;
  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& behaviorExternalInterface ) const override;
  virtual void HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface ) override;
  

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helper structs

  struct CubeTapInfo
  {
    ObjectID      id;
    float         lastTappedTime;
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal functions

  void HandleObjectTapped( BehaviorExternalInterface& bei, const ExternalInterface::ObjectTapped& msg );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Members

  struct ConfigParams
  {
    float maxTapResponseTime; // if a cube has been tapped within the last maxTapResponseTime seconds
  };

  ConfigParams _params;
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
  CubeTapInfo _tapInfo;

};


} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionCubeTapped_H__
