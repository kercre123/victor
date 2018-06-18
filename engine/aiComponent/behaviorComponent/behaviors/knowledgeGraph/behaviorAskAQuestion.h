/***********************************************************************************************************************
 *
 *  BehaviorAskAQuestion
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 5/09/2018
 *
 *  Description
 *  + Parent behavior requesting a knowledge graph inquiry.
 *  + Streams a users voice (assumed to be a question for Victor) to the cloud which returns a response/answer string
 *
 **********************************************************************************************************************/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAskAQuestion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAskAQuestion_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/voiceMessageTypes.h"


namespace Anki {
namespace Cozmo {

class BehaviorTextToSpeechLoop;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorAskAQuestion : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorAskAQuestion( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State Transitions

  void TransitionToBeginResponse();
  void TransitionToNoResponse();
  void TransitionToNoConnection();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helpers

  void BeginStreamingQuestion();
  void BeginResponseTTS();

  bool IsResponsePending() const;
  void ConsumeResponse();

  void OnStreamingComplete( BehaviorSimpleCallback next );


private:

  enum class EState : uint8_t
  {
    Listening,
    Responding,
    NoResponse,
    NoConnection
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Instance Vars are members that last the lifetime of the behavior and generally do not change (config vars)

  struct InstanceConfig
  {
    InstanceConfig();

    float               streamingDuration;
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dynamic Vars are members that change over the lifetime of the behavior and are generally reset every activation

  struct DynamicVariables
  {
    DynamicVariables();

    EState              state;
    float               streamingBeginTime;
    std::string         responseString;

  } _dVars;

}; // class BehaviorAskAQuestion

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAskAQuestion_H__
