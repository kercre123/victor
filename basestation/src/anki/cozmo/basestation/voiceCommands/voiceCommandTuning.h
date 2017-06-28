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

struct TestResultScoreData
{
  std::string _scoredPhrase;
  
  int _hits = 0;
  int _misses = 0;
  int _falsePositives = 0;
  
  double _targetScoreConstant = 600;
  
  double _hitMult = 1.0f;
  double _missMult = -1.0f;
  double _falsePositiveMult = -1.0f;
  double _targetScoreDiffMult = -1.0f;
  
  std::map<std::string, std::vector<Anki::AudioUtil::AudioRecognizerProcessor::ResultType>> _filenameToResultsMap;
  
  // Uses hits, misses, and false positives (and their multipliers) along with the score of recognized phrases to give an overall score
  double GetScore() const;
  
  // Uses hits and misses and the hits multiplier to give the best possible score (omits calculating negative impacts)
  double GetMaxScore() const;
};

class VoiceCommandTuning {
public:
  VoiceCommandTuning(const Util::Data::DataPlatform& dataPlatform,
                     const CommandPhraseData& commandPhraseData,
                     double targetScore,
                     Anki::Util::Locale locale);
  ~VoiceCommandTuning();
  
  TestResultScoreData ScoreParams(const std::set<VoiceCommandType>& testCommandSet,
                                  VoiceCommandType commandType, int paramA, int paramB, bool playAudio = false) const;
  
  bool CalculateParamsForCommand(const std::set<VoiceCommandType>& testCommandSet,
                                 VoiceCommandType commandType, int& out_paramA, int& out_paramB) const;
  
  static double GSSMaximum(std::function<double(double)> function, double a, double b, int maxIterations);
  
private:
  
  std::unique_ptr<Anki::AudioUtil::AudioFileReader>             _audioInput;
  std::unique_ptr<Anki::Cozmo::SpeechRecognizerTHF>             _recognizer;
  std::unique_ptr<Anki::AudioUtil::AudioRecognizerProcessor>    _recogProcessor;
  std::string                                                   _generalNNPath;
  std::string                                                   _languageSampleDir;
  TestResultScoreData                                           _baseScoreData;
  const CommandPhraseData&                                      _commandPhraseData;
  Anki::Util::Locale                                            _currentLocale;
};
  
  
  
} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_VoiceCommand_VoiceCommandTuning_H_
