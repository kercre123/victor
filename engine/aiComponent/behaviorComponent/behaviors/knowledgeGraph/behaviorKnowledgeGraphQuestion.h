/***********************************************************************************************************************
 *
 *  BehaviorKnowledgeGraphQuestion
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 5/09/2018
 *
 *  Description
 *  + Parent behavior requesting a knowledge graph inquiry.
 *  + Streams a users voice (assumed to be a question for Victor) to the cloud which returns a response/answer string
 *
 **********************************************************************************************************************/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorKnowledgeGraphQuestion_H__
#define __Cozmo_Basestation_Behaviors_BehaviorKnowledgeGraphQuestion_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/mics/voiceMessageTypes.h"


namespace Anki {
namespace Cozmo {

class BehaviorTextToSpeechLoop;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorKnowledgeGraphQuestion : public ICozmoBehavior
{
  friend class BehaviorFactory;
  BehaviorKnowledgeGraphQuestion( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys( std::set<const char*>& expectedKeys ) const override;

  // wake word will cancel our TTS
  // note: we're broadly cancelling streaming the entire time we're active; technically we only need to do it
  //       while we're in the "Responding" state, but I expect this wont play very nice with our wakewordless streaming
  virtual bool ShouldSuppressTriggerWordResponse() const override { return true; }


protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Overrides from ICozmoBehavior

  virtual void InitBehavior() override;
  virtual void GetAllDelegates( std::set<IBehavior*>& delegates ) const override;

  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void HandleWhileActivated( const EngineToGameEvent& event ) override;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // State Transitions

  void TransitionToSearchingLoop();
  void TransitionToBeginResponse();
  void TransitionToNoResponse();
  void TransitionToNoConnection();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Helpers

  void BeginStreamingQuestion();
  void BeginResponseTTS();

  bool IsResponsePending() const;
  void ConsumeResponse();

  void OnStreamingComplete();
  void OnResponseInterrupted();


private:

  enum class EState : uint8_t
  {
    TransitionToListening,
    Listening,
    Searching,
    Responding,
    NoResponse,
    NoConnection,
    Interrupted
  };

  enum class EGenerationStatus
  {
    None,
    Success,
    Fail
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
    std::string         responseString;
    EGenerationStatus   ttsGenerationStatus;

  } _dVars;

}; // class BehaviorKnowledgeGraphQuestion

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorKnowledgeGraphQuestion_H__
