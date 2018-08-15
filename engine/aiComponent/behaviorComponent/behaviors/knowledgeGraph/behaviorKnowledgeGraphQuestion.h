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
#include "engine/components/backpackLights/backpackLightComponentTypes.h"
#include "components/textToSpeech/textToSpeechWrapper.h"
#include "clad/audio/audioEventTypes.h"


namespace Anki {
namespace Vector {

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

  // start streaming mic data to knowledge graph
  void BeginStreamingQuestion();
  // speak back the response we got from knowledge graph
  void BeginResponseTTS();

  // we have a response intent pending from knowledgeGraph
  bool IsResponsePending() const;
  // translate that response intent into and answer string and store it for later
  void ConsumeResponse();

  // we're done listening to the question
  void OnStreamingComplete();
  // we allow the response audio to be interrupted under certain conditions
  void OnResponseInterrupted();

  // streaming cues are backpack lights and audio cues to let the user know when to speak
  void PlayEarconEnd();


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  enum class EState : uint8_t
  {
    GettingIn,
    WaitingToStream,
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

    double              streamingDuration; // how long before we give up on streaming and bail
    std::string         readyText; // audio tts to let the user know they can begin speaking
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;

    AudioMetaData::GameEvent::GenericEvent earConEnd;

  } _iVars;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Dynamic Vars are members that change over the lifetime of the behavior and are generally reset every activation

  struct DynamicVariables
  {
    DynamicVariables();

    EState              state;
    double              streamingBeginTime; // the time we begun actually streaming the mic data
    std::string         responseString; // the text we got back from knowledge graph
    EGenerationStatus   ttsGenerationStatus; // track the status of the reponse tts
    BackpackLightDataLocator  lightsHandle; // lights, camera, action!

  } _dVars;

  TextToSpeechWrapper _readyTTSWrapper;

}; // class BehaviorKnowledgeGraphQuestion

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorKnowledgeGraphQuestion_H__
