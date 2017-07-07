/**
* File: voiceCommandTuning.cpp
*
* Author: Lee Crippen
* Created: 06/26/2017
*
* Description: Contains functionality for scoring THF speech recognition tuning parameters.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "anki/cozmo/basestation/voiceCommands/voiceCommandTuning.h"

#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/languagePhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "anki/cozmo/basestation/voiceCommands/sampleKeyphraseData.h"
#include "anki/cozmo/basestation/voiceCommands/speechRecognizerTHF.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "audioUtil/audioFileReader.h"
#include "audioUtil/audioPlayer.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "json/json.h"

#include <cmath>
#include <thread>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

VoiceCommandTuning::VoiceCommandTuning(const Util::Data::DataPlatform& dataPlatform,
                                       const CommandPhraseData& commandPhraseData,
                                       double targetScore,
                                       Anki::Util::Locale locale)
: _commandPhraseData(commandPhraseData)
, _currentLocale(locale)
{
  // Get the test data
  static const std::string& sampleDirPath = "test/voiceCommandTests/samples";
  static const std::string& testBaseDir = dataPlatform.pathToResource(Util::Data::Scope::Resources, sampleDirPath);
  
  _languageSampleDir = Util::FileUtils::FullFilePath({testBaseDir, locale.GetLocaleString()});
  
  
  // Get the language data files
  const auto& languageFilenames = commandPhraseData.GetLanguageFilenames(locale.GetLanguage(), locale.GetCountry());
  const std::string& languageDirStr = languageFilenames._languageDataDir;
  
  static const std::string& voiceCommandAssetDirStr = "assets/voiceCommand/base";
  const std::string& pathBase = dataPlatform.pathToResource(Util::Data::Scope::Resources, voiceCommandAssetDirStr);
  _generalNNPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._netfileFilename} );
  const std::string& generalPronunPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._ltsFilename} );
  
  // Set up recognizer
  _recognizer.reset(new SpeechRecognizerTHF{});
  _recognizer->Init(generalPronunPath);
  _recognizer->Start();
  
  // Set up audio file reader
  _audioInput.reset(new AudioUtil::AudioFileReader{});
  
  // Set up processor
  _recogProcessor.reset(new AudioUtil::AudioRecognizerProcessor{""});
  _recogProcessor->SetSpeechRecognizer(_recognizer.get());
  _recogProcessor->SetAudioInputSource(_audioInput.get());
  _recogProcessor->Start();
  
  // score the results for this configuration
  _baseScoreData._hitMult = 20.0f;
  _baseScoreData._missMult = -10.0f;
  _baseScoreData._falsePositiveMult = -5.0f;
  _baseScoreData._targetScoreDiffMult = -0.001f;
  _baseScoreData._targetScoreConstant = targetScore;
}

// forward-declared unique-pointers demand this
VoiceCommandTuning::~VoiceCommandTuning() = default;

bool VoiceCommandTuning::CalculateParamsForCommand(const std::set<VoiceCommandType>& testCommandSet,
                                                   VoiceCommandType commandType, int& out_paramA, int& out_paramB) const
{
  constexpr int numSamplesPerParamBScore = 5;
  constexpr double paramABegin = -500;
  constexpr double paramAEnd = -1500;
  
  std::map<int, double> paramBLUT;
  auto gssParamBScoring = [this, commandType, &paramBLUT, &testCommandSet] (double paramBValue) -> double
  {
    int paramB = Util::numeric_cast<int>(paramBValue);
    if (paramBLUT.find(paramB) != paramBLUT.end())
    {
      return paramBLUT[paramB];
    }
    
    double scoreAverage = 0;
    
    double paramA = paramABegin;
    double paramAInc = (paramAEnd - paramABegin) / Util::numeric_cast<double>(numSamplesPerParamBScore);
    
    for (int i = 0; i < numSamplesPerParamBScore; ++i)
    {
      scoreAverage += ScoreParams(testCommandSet, commandType, paramA, paramB).GetScore();
      paramA += paramAInc;
    }
    
    scoreAverage /= Util::numeric_cast<double>(numSamplesPerParamBScore);
    
    PRINT_NAMED_INFO("VoiceCommandTuning.CalculateParamsForCommand",
                     "Average for paramB:%d %f", paramB, scoreAverage);
    
    paramBLUT[paramB] = scoreAverage;
    return scoreAverage;
  };
  
  const double rangeBeginParamB = 100;
  const double rangeEndParamB = 1000;
  const double maxIterationsParamB = 20;
  int gssParamBResult = Util::numeric_cast<int>(std::llround(GSSMaximum(gssParamBScoring, rangeBeginParamB, rangeEndParamB, maxIterationsParamB)));
  
  std::map<int, double> paramALUT;
  auto gssParamAScoring = [this, commandType, gssParamBResult, &paramALUT, &testCommandSet] (double paramAValue) -> double
  {
    int paramA = Util::numeric_cast<int>(paramAValue);
    if (paramALUT.find(paramA) != paramALUT.end())
    {
      return paramALUT[paramA];
    }
    
    double scoreValue = ScoreParams(testCommandSet, commandType, paramA, gssParamBResult).GetScore();
    paramALUT[paramA] = scoreValue;
    return scoreValue;
  };
  
  const double rangeBeginParamA = 0;
  const double rangeEndParamA = -2000;
  const double maxIterationsParamA = 10;
  int gssParamAResult = Util::numeric_cast<int>(std::llround(GSSMaximum(gssParamAScoring, rangeBeginParamA, rangeEndParamA, maxIterationsParamA)));
  
  out_paramA = gssParamAResult;
  out_paramB = gssParamBResult;
  
  return true;
}

TestResultScoreData VoiceCommandTuning::ScoreParams(const std::set<VoiceCommandType>& testCommandSet,
                                                    VoiceCommandType commandType, int paramA, int paramB, bool playAudio) const
{
  PhraseDataSharedPtr phraseData;
  
  // Make a copy of the phraselist so we can make modifications
  std::vector<Anki::Cozmo::VoiceCommand::PhraseDataSharedPtr> commandPhraseList;
  {
    const auto& commandPhraseListTmp = _commandPhraseData.GetPhraseDataList(_currentLocale.GetLanguage(), testCommandSet);
    for (const auto& item : commandPhraseListTmp)
    {
      commandPhraseList.push_back(PhraseDataSharedPtr(new PhraseData(*item)));
      
      // If this is the phrase data we'll be focusing on, keep track of it
      if (commandPhraseList.back()->GetVoiceCommandType() == commandType)
      {
        phraseData = commandPhraseList.back();
      }
    }
  }
  
  phraseData->SetParamA(paramA);
  phraseData->SetParamB(paramB);
  //    phraseData->SetParamC(250);
  const AudioUtil::SpeechRecognizer::IndexType recogIndex = 0;
  const bool isPhraseSpotted = true;
  const bool allowsFollowupRecog = false;
  _recognizer->AddRecognitionDataAutoGen(recogIndex, _generalNNPath, commandPhraseList, isPhraseSpotted, allowsFollowupRecog);
  _recognizer->SetRecognizerIndex(recogIndex);
  
  // score the results for this configuration
  TestResultScoreData scoreData = _baseScoreData;
  scoreData._scoredPhrase = phraseData->GetPhrase();
  
  Json::Value jsonObject{};
  const std::string& phraseDataFilename = "samplePhraseData.json";
  Anki::Util::Data::DataPlatform::readAsJson(Util::FileUtils::FullFilePath({_languageSampleDir, phraseDataFilename}), jsonObject);
  SampleKeyphraseData sampleData{};
  sampleData.Init(jsonObject);
  const auto& fileToCommandData = sampleData.GetFileToDataMap();
  for (const auto& iter : fileToCommandData)
  {
    const auto& filename = iter.first;
    
    // Set up audio input
    _audioInput->ReadFile(Util::FileUtils::FullFilePath( {_languageSampleDir, filename} ));
    
    AudioUtil::AudioPlayer audioPlayer;
    if (playAudio)
    {
      PRINT_NAMED_INFO("VoiceCommandTuning.ScoreParams","Playing audio filename %s", filename.c_str());
      audioPlayer.StartPlaying(_audioInput->GetAudioSamples().begin(), _audioInput->GetAudioSamples().end());
      std::this_thread::sleep_for(std::chrono::milliseconds(AudioUtil::kTimePerAudioChunk_ms));
    }
    else
    {
      (void)audioPlayer;
    }
    
    // Tell the audio input to go through the audio data and deliver it
    _audioInput->DeliverAudio(playAudio);
    
    // If we were playing audio we need to make sure it's stopped
    if (playAudio)
    {
      audioPlayer.StopPlaying();
    }
    
    // Make a copy of the expected results so we can decrement them as we go through what we actually got
    CommandCounts expectedResults = iter.second;
    auto& resultsVectorForFile = scoreData._filenameToResultsMap[filename];
    while (_recogProcessor->HasResults())
    {
      auto result = _recogProcessor->PopNextResult();
      
      PhraseDataSharedPtr commandDataFound;
      for (const auto& dataIter : commandPhraseList)
      {
        if (dataIter->GetPhrase() == result.first)
        {
          commandDataFound = dataIter;
          break;
        }
      }
      const auto& commandType = commandDataFound->GetVoiceCommandType();
      
      auto remainingExpectedCountIter = expectedResults.find(commandType);
      if (remainingExpectedCountIter == expectedResults.end())
      {
        ++scoreData._falsePositives;
      }
      else if (remainingExpectedCountIter->second == 0)
      {
        ++scoreData._falsePositives;
        expectedResults.erase(remainingExpectedCountIter);
      }
      else if (remainingExpectedCountIter->second > 0)
      {
        ++scoreData._hits;
        --remainingExpectedCountIter->second;
        if (remainingExpectedCountIter->second == 0)
        {
          expectedResults.erase(remainingExpectedCountIter);
        }
      }
      
      resultsVectorForFile.push_back(std::move(result));
    }
    
    // count up any remaining expected results that are in our test set as misses
    for (const auto& iter : expectedResults)
    {
      if (testCommandSet.find(iter.first) != testCommandSet.end())
      {
        scoreData._misses += iter.second;
      }
    }
  }
  
  _recognizer->RemoveRecognitionData(recogIndex);
  _recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
  
  return scoreData;
}

double TestResultScoreData::GetScore() const
{
  // Go through the results from each file and get an average score of the results we got.
  int numResults = 0;
  double averageScore = 0;
  
  for (const auto& iter : _filenameToResultsMap)
  {
    const auto& resultsList = iter.second;
    for (const auto& result : resultsList)
    {
      if (result.first == _scoredPhrase)
      {
        averageScore += result.second;
        ++numResults;
      }
    }
  }
  
  if (numResults > 0)
  {
    averageScore = averageScore / Anki::Util::numeric_cast<double>(numResults);
  }
  
  // We want the deviation from the target score as an absolute value
  const double& targetScoreDiff = Anki::Util::Abs(averageScore - _targetScoreConstant);
  
  const double hitScore = Util::numeric_cast<double>(_hits) * _hitMult;
  const double missScore = Util::numeric_cast<double>(_misses) * _missMult;
  const double falsePositiveScore = Util::numeric_cast<double>(_falsePositives) * _falsePositiveMult;
  const double targetScoreDiffScore = targetScoreDiff * _targetScoreDiffMult;
  
  const double finalScore = hitScore + missScore + falsePositiveScore + targetScoreDiffScore;
  return finalScore;
}

double TestResultScoreData::GetMaxScore() const
{
  return  (Anki::Util::numeric_cast<double>(_hits + _misses) * _hitMult);
}


// Golden section search(for maximum in range)
// See https://en.wikipedia.org/wiki/Golden-section_search for details
double VoiceCommandTuning::GSSMaximum(std::function<double(double)> function, double a, double b, int maxIterations)
{
  static const double goldenRatio = (std::sqrt(5.0) + 1.0) / 2.0;
  
  double c = b - (b - a) / goldenRatio;
  double d = a + (b - a) / goldenRatio;
  
  while(maxIterations > 0)
  {
    if (function(c) > function(d))
    {
      b = d;
    }
    else
    {
      a = c;
    }
    
    c = b - (b - a) / goldenRatio;
    d = a + (b - a) / goldenRatio;
    
    --maxIterations;
  }
  
  return (b + a) / 2.0;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
