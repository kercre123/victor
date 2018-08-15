/**
 * File: conditionIlluminationDetected.h
 * 
 * Author: Humphrey Hu
 * Created: June 01 2018
 *
 * Description: Condition that checks the observed scene illumination entering and/or
 *              leaving from specified states.
 * 
 *              The condition will be true for a specified number of seconds after pre and
 *              post-transition conditions have been met.
 * 
 *              Specify "preTriggerStates" and/or "postTriggerStates" in JSON, as well as 
 *              accompanying latch parameters (see conditionIlluminationDetect.cpp). If no 
 *              states are given for a set of triggers, any illumination state will satisfy 
 *              that requirement.
 * 
 *              To fire on entering a state, specify only "preTriggerStates"
 *              To fire on exiting a state, specify only "postTriggerStates"
 *              To fire on transitioning, specify both "preTriggerStates and "postTriggerStates"
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__

#include "coretech/common/engine/robotTimeStamp.h"
#include "clad/types/imageTypes.h"
#include "engine/aiComponent/beiConditions/iBEICondition.h"
#include "engine/aiComponent/beiConditions/iBEIConditionEventHandler.h"

namespace Anki {
namespace Vector {

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
    std::vector<IlluminationState> preStates;     // Transition starts by entering these states
    f32 preConfirmationTime_s;                    // Number of seconds pre state must match
    u32 preConfirmationMinNum;                    // Min number of pre-state match events
    std::vector<IlluminationState> postStates;    // Transition finishes by entering these states
    f32 postConfirmationTime_s;                   // Number of seconds post state must match
    u32 postConfirmationMinNum;                   // Min number of post-state match events
    f32 matchHoldTime_s;                          // Number of seconds to hold the condition true on transition
  };
  
  enum class MatchState
  {
    WaitingForPre,
    ConfirmingPre,
    WaitingForPost,
    ConfirmingPost,
    MatchConfirmed
  };
  
  struct DynamicVariables
  {
    MatchState        matchState;     // State of matching process
    RobotTimeStamp_t  matchStartTime; // Time at which first match in series received (if matched)
    u32               matchedEvents;  // Number of matches in current series
  };

  ConfigParams _params;
  DynamicVariables _variables;
  std::unique_ptr<BEIConditionMessageHelper> _messageHelper;

  void TickStateMachine( const RobotTimeStamp_t& currTime, const IlluminationState& obsState );
  bool IsTimePassed( const RobotTimeStamp_t& t, const f32& dur ) const;
  bool IsTriggerState( const std::vector<IlluminationState>& triggers,
                       const IlluminationState& state ) const;
  bool ParseTriggerStates( const Json::Value& config, const char* key,
                           std::vector<IlluminationState>& triggers );
};

} // end namespace Vector
} // end namespace Anki

#endif // __Engine_AiComponent_BeiConditions_Conditions_ConditionIlluminationDetected_H__
