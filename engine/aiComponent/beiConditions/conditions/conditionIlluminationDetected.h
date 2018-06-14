/**
 * File: conditionIlluminationDetected.h
 * 
 * Author: Humphrey Hu
 * Created: June 01 2018
 *
 * Description: Condition that checks the observed scene illumination entering and/or
 * leaving from specified states
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__

#include "clad/types/imageTypes.h"
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

  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& request) const override;

  // Resets the condition to inactive
  void Reset();

protected:

  virtual void InitInternal( BehaviorExternalInterface& bei ) override;
  virtual bool AreConditionsMetInternal( BehaviorExternalInterface& bei ) const override;
  virtual void HandleEvent( const EngineToGameEvent& event, BehaviorExternalInterface& bei ) override;

private:

  struct ConfigParams
  {
    std::vector<IlluminationState> triggerStates; // Triggered by entering these states
    f32 confirmationTime_s;                       // Number of seconds state must match to trigger condition
    u32 confirmationMinNum;                       // Min number of match events to trigger condition
    bool ignoreUnknown;                           // Whether to ignore Unknown illuminations
  };
  
  enum class MatchState
  {
    WaitingForStart,
    ConfirmingMatch,
    MatchConfirmed
  };
  
  struct DynamicVariables
  {
    MatchState   matchState;     // State of matching process
    TimeStamp_t  matchStartTime; // Time at which first match in series received (if matched)
    u32          matchedEvents;  // Number of matches in current series
  };

  ConfigParams _params;
  DynamicVariables _variables;
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;

  void TickStateMachine( const TimeStamp_t& currTime, const IlluminationState& obsState );
  bool IsTimePassed( const TimeStamp_t& t, const f32& dur ) const;
  bool IsTriggerState( const IlluminationState& state ) const;
};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__