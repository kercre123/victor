/**
* File: textToSpeechCoordinator.h
*
* Author: Sam Russell
* Created: 5/22/18
*
* Description: Component for requesting and tracking TTS utterance generation and playing
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Components_TextToSpeechCoordinator_H__
#define __Engine_Components_TextToSpeechCoordinator_H__

#include "engine/robotComponents_fwd.h"
#include "coretech/common/shared/types.h"
#include "clad/audio/audioSwitchTypes.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/helpers/templateHelpers.h"
#include "util/random/randomGenerator.h"
#include "util/signals/simpleSignal_fwd.h"

#include <functional>
#include <unordered_map>

namespace Anki{
namespace Cozmo{


const static uint8_t kInvalidUtteranceID = 0;
// Utterances should not take longer than this to generate
const static float kGenerationTimeout_s = 10.0f; // making this high until we figure out accurate generation times

// Forward declarations
class Robot;
enum class TextToSpeechState : uint8_t;
struct TextToSpeechEvent;

enum class UtteranceState {
  Invalid,
  Generating,
  Ready,
  Playing,
  Finished
};

enum class UtteranceTriggerType {
  Immediate,
  Manual,
  KeyFrame
};

class TextToSpeechCoordinator : public IDependencyManagedComponent<RobotComponentID>, public Util::noncopyable
{
public:
  TextToSpeechCoordinator();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IDependencyManagedComponent
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::DataAccessor);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {}
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& dependencies) const override {}
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  // IDependencyManagedComponent
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using AudioTtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
  using UtteranceUpdatedCallback = std::function<void(const UtteranceState&)>;

  const uint8_t CreateUtterance(const std::string& utteranceString,
                                const UtteranceTriggerType& triggerType,
                                const AudioTtsProcessingStyle& style = AudioTtsProcessingStyle::Default_Processed,
                                const float durationScalar = 1.f,
                                UtteranceUpdatedCallback callback = nullptr);


  const UtteranceState GetUtteranceState( const uint8_t utteranceID) const;
  const bool           PlayUtterance(const uint8_t utteranceID);

  // we can cancel the tts at any point during it's lifecycle, however ...
  //  + if cancelling when tts is generating, thread will hang until generation is complete
  //  + cancelling will clear the wav data in AnkiPluginInterface, regardless of which utteranceID was last delivered
  const bool           CancelUtterance(const uint8_t utteranceID);

private:
  void UpdateUtteranceState(const uint8_t& ttsID,
                            const TextToSpeechState& ttsState,
                            const float& expectedDuration_ms = 0.0f);

  struct UtteranceRecord {
    UtteranceState            state                       = UtteranceState::Invalid;
    UtteranceTriggerType      triggerType                 = UtteranceTriggerType::Manual;
    float                     timeStartedGeneration_s     = 0.0f;
    float                     timeStartedPlaying_s        = 0.0f;
    float                     expectedDuration_s          = 0.0f;
    size_t                    tickReadyForCleanup         = 0;
    UtteranceUpdatedCallback  callback                    = nullptr;
  };
  
  std::unordered_map<uint8_t, UtteranceRecord> _utteranceMap;

  Signal::SmartHandle            _signalHandle;

  Robot* _robot = nullptr;
}; // class TextToSpeechCoordinator

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_Components_TextToSpeechCoordinator_H__ 
