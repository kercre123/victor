/**
* File: voiceCommandTuning.h
*
* Author: Lee Crippen
* Created: 06/26/2017
*
* Description: Contains functionality for scoring THF speech recognition tuning parameters.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Cozmo_Basestation_VoiceCommand_VoiceCommandTuning_H_
#define __Cozmo_Basestation_VoiceCommand_VoiceCommandTuning_H_

#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/recognitionSetupData.h"
#include "anki/cozmo/basestation/voiceCommands/sampleKeyphraseData.h"

#include "clad/types/voiceCommandTypes.h"
#include "audioUtil/audioRecognizerProcessor.h"
#include "util/environment/locale.h"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Anki {
  
namespace AudioUtil {
  class AudioFileReader;
}

namespace Util {
namespace Data {
  class DataPlatform;
}
}

namespace Cozmo {
  
class SpeechRecognizerTHF;

namespace VoiceCommand {
  
class CommandPhraseData;
class PhraseData;
using PhraseDataSharedPtr = std::shared_ptr<PhraseData>;

class LanguagePhraseData;
using LanguagePhraseDataSharedPtr = std::shared_ptr<LanguagePhraseData>;
using ConstLanguagePhraseDataSharedPtr = std::shared_ptr<const LanguagePhraseData>;

struct TestResultScoreData
{
  double _score = 0;
  double _maxScore = 0;
  
  static constexpr double _kTargetScoreConstant = 500;
  
  double _hitMult = 1.0f;
  double _missMult = -1.0f;
  double _falsePositiveMult = -1.0f;
  double _targetScoreDiffMult = -1.0f;
  
  struct TestResultInfo
  {
    CommandCounts                                                       _expectedResults;
    std::vector<Anki::AudioUtil::AudioRecognizerProcessor::ResultType>  _actualResults;
  };
  
  std::map<std::string, TestResultInfo> _filenameToResultsMap;
  
  // Uses hits, misses, and false positives (and their multipliers) along with the score of recognized phrases to give an overall score
  double CalculateScore(const std::string phrase, const ConstLanguagePhraseDataSharedPtr& phraseDataLookup);
  
  // Uses hits and misses and the hits multiplier to give the best possible score (omits calculating negative impacts)
  double GetScore() const { return _score; }
  double GetMaxScore() const { return _maxScore; }
};

class VoiceCommandTuning {
public:
  VoiceCommandTuning(const Util::Data::DataPlatform& dataPlatform,
                     const CommandPhraseData& commandPhraseData,
                     Anki::Util::Locale locale,
                     const std::string& sampleGroupDir = "");
  ~VoiceCommandTuning();
  
  // Allows for scoring a command set when configuring the params for one of the commands
  TestResultScoreData ScoreParams(const std::set<VoiceCommandType>& testCommandSet,
                                  VoiceCommandType commandType, int paramA, int paramB, bool playAudio = false) const;
  
  // Allows for scoring a command set using the currently configured values
  TestResultScoreData ScoreParams(const std::set<VoiceCommandType>& testCommandSet, bool playAudio) const;
  
  TestResultScoreData ScoreTriggerAndCommand(const ContextConfig& commandListConfig,
                                             bool playAudio, ContextConfigSharedPtr& out_CombinedConfig) const;
  
  bool CalculateParamsForCommand(const std::set<VoiceCommandType>& testCommandSet,
                                 VoiceCommandType commandType, int& out_paramA, int& out_paramB) const;
  
  using ContextConfigSetter = std::function<void(ContextConfig&, float)>;
  float CalculateContextConfigValue(ContextConfigSetter setterFunc,
                                    double valueBegin, double valueEnd, bool useGoldenSectionSearch) const;
  
  static double GSSMaximum(std::function<double(double)> function, double a, double b, int maxIterations);
  static double FindBestBruteForce(std::function<double(double)> function, double a, double b, int iterations);
  
private:
  
  std::unique_ptr<Anki::AudioUtil::AudioFileReader>             _audioInput;
  std::unique_ptr<Anki::Cozmo::SpeechRecognizerTHF>             _recognizer;
  std::unique_ptr<Anki::AudioUtil::AudioRecognizerProcessor>    _recogProcessor;
  std::string                                                   _generalNNPath;
  std::string                                                   _languageSampleDir;
  TestResultScoreData                                           _baseScoreData;
  const CommandPhraseData::ContextDataMap&                      _contextDataMap;
  ConstLanguagePhraseDataSharedPtr                              _languagePhraseData;
  RecognitionSetupData                                          _triggerPhraseSetup;
  
  using ResetTask = std::function<void()>;
  TestResultScoreData CollectResults(const std::set<VoiceCommandType>& testCommandSet,
                                     bool playAudio, ResetTask resetTask = ResetTask{}) const;
};
  
  
  
} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_VoiceCommand_VoiceCommandTuning_H_
