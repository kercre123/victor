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
#include "clad/types/sayTextStyles.h"
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
const static float kGenerationTimeout_s = 3.0f;

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
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::DataAccessor);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {}
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& dependencies) const override {}
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  // IDependencyManagedComponent
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using UtteranceUpdatedCallback = std::function<void(const UtteranceState&)>;

  const uint8_t CreateUtterance(const std::string& utteranceString,
                                const SayTextVoiceStyle& style,
                                const UtteranceTriggerType& triggerType,
                                const float durationScalar = 1.f,
                                const float pitchScalar = 0.f,
                                UtteranceUpdatedCallback callback = nullptr);

  const uint8_t CreateUtterance(const std::string& utteranceString,
                                const SayTextIntent& intent,
                                const UtteranceTriggerType& triggerType,
                                const UtteranceUpdatedCallback callback = nullptr);

  const UtteranceState GetUtteranceState( const uint8_t utteranceID) const;
  const bool           PlayUtterance(const uint8_t utteranceID);
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

  // SayTextVoiceStyle lookup up by name
  using SayTextVoiceStyleMap = std::unordered_map<std::string, SayTextVoiceStyle>;

  // Cozmo Says intent metadata config data struct
  struct SayTextIntentConfig
  {
    struct ConfigTrait
    {
      uint textLengthMin = std::numeric_limits<uint>::min();
      uint textLengthMax = std::numeric_limits<uint>::max();
      float rangeMin = std::numeric_limits<float>::min();;
      float rangeMax = std::numeric_limits<float>::max();;
      float rangeStepSize = 0.0f;

      ConfigTrait() = default;
      ConfigTrait(const Json::Value& json);

      float GetDuration(Util::RandomGenerator& randomGen) const;
    };

    std::string name = "";
    SayTextVoiceStyle style = SayTextVoiceStyle::CozmoProcessing_Sentence;
    std::vector<ConfigTrait> durationTraits;
    std::vector<ConfigTrait> pitchTraits;

    SayTextIntentConfig() = default;
    SayTextIntentConfig(const std::string& intentName, const Json::Value& json, const SayTextVoiceStyleMap& styleMap);

    const ConfigTrait& FindDurationTraitTextLength(uint textLength) const;
    const ConfigTrait& FindPitchTraitTextLength(uint textLength) const;
  };

  // Store loaded Cozmo Says intent configs
  using SayIntentConfigMap = std::unordered_map<SayTextIntent, SayTextIntentConfig, Util::EnumHasher>;
  static SayIntentConfigMap _intentConfigs;

  Robot* _robot = nullptr;
}; // class TextToSpeechCoordinator

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_Components_TextToSpeechCoordinator_H__ 
