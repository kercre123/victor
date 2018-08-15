/**
 * File: conditionEngineErrorCodeReceived.h
 * 
 * Author: Arjun Menon
 * Created: August 08 2018
 *
 * Description: 
 * Condition that checks the image quality based on the given type expected
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionEngineErrorCodeReceived_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionEngineErrorCodeReceived_H__

#include "clad/types/engineErrorCodes.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

class BEIConditionMessageHelper;

class ConditionEngineErrorCodeReceived : public IBEICondition, private IBEIConditionEventHandler
{
public:

  ConditionEngineErrorCodeReceived( const Json::Value& config );
  virtual ~ConditionEngineErrorCodeReceived();

protected:

  virtual void InitInternal( BehaviorExternalInterface& bei ) override;
  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& bei ) const override;
  virtual void HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei ) override;

private:

  struct InstanceConfig
  {
    InstanceConfig();

    EngineErrorCode engineErrorCodeToCheck;
  };
  
  struct DynamicVariables
  {
    DynamicVariables();

    bool engineErrorCodeMatches;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
};

} // end namespace Vector
} // end namespace Anki

#endif
