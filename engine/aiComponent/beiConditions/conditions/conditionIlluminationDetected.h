/**
 * File: conditionIlluminationDetected.h
 * 
 * Author: Humphrey Hu
 * Created: June 01 2018
 *
 * Description: Condition that checks the observed scene illumination for a particular state
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__

#include "clad/types/illuminationTypes.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Cozmo {

class BEIConditionMessageHelper;

class ConditionIlluminationDetected : public IBEICondition, private IBEIConditionEventHandler
{
public:

  ConditionIlluminationDetected( const Json::Value& config );
  virtual ~ConditionIlluminationDetected();

protected:

  virtual void InitInternal( BehaviorExternalInterface& bei ) override;
  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& bei ) const override;
  virtual void HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei ) override;

private:

  struct ConfigParams
  {
    IlluminationState targetState; // State that causes trigger
    f32 confirmationTime_s;   // Number of seconds state must match to trigger condition
    u32 confirmationMinNum; // Min number of match events to trigger condition
  };
  
  ConfigParams _params;
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;
  
  enum class MatchState
  {
    WaitingForMatch,
    ConfirmingMatch,
    MatchConfirmed
  };

  MatchState   _matchState;     // State of matching process
  TimeStamp_t  _matchStartTime; // Time at which first match in series received (if matched)
  u32          _matchedEvents;  // Number of matches in current series

  void HandleIllumination( const EngineToGameEvent& event );
};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__