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

#include "engine/voiceCommands/voiceCommandTuning.h"

#include "engine/voiceCommands/contextConfig.h"
#include "engine/voiceCommands/languagePhraseData.h"
#include "engine/voiceCommands/phraseData.h"
#include "engine/voiceCommands/recognitionSetupData.h"
#include "engine/voiceCommands/speechRecognizerTHF.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "audioUtil/audioFileReader.h"
#include "audioUtil/audioPlayer.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "json/json.h"

#include <cmath>
#include <thread>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
// forward-declared unique-pointers demand this
VoiceCommandTuning::~VoiceCommandTuning() = default;

#if THF_FUNCTIONALITY
VoiceCommandTuning::VoiceCommandTuning(const Util::Data::DataPlatform& dataPlatform,
                                       const CommandPhraseData& commandPhraseData,
                                       Anki::Util::Locale locale,
                                       const std::string& sampleGroupDir)
: _contextDataMap(commandPhraseData.GetContextData())
, _languagePhraseData(commandPhraseData.GetLanguagePhraseData(locale.GetLanguage()))
{
  // Get the test data
  static const std::string& sampleDirPath = "test/voiceCommandTests/samples";
  static const std::string& testBaseDir = dataPlatform.pathToResource(Util::Data::Scope::Resources, sampleDirPath);
  
  _languageSampleDir = Util::FileUtils::FullFilePath({testBaseDir, locale.GetLocaleString(), sampleGroupDir});
  
  
  // Get the language data files
  const auto& languageFilenames = commandPhraseData.GetLanguageFilenames(locale.GetLanguage(), locale.GetCountry());
  const std::string& languageDirStr = languageFilenames._languageDataDir;
  
  static const std::string& voiceCommandAssetDirStr = "assets/voiceCommand/base";
  const std::string& pathBase = dataPlatform.pathToResource(Util::Data::Scope::Resources, voiceCommandAssetDirStr);
  _generalNNPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._netfileFilename} );
  const std::string& generalPronunPath = Util::FileUtils::FullFilePath( {pathBase, languageDirStr, languageFilenames._ltsFilename} );
  
  // Store off a copy of the trigger phrase recognition data for later use
  _triggerPhraseSetup = commandPhraseData.GetRecognitionSetupData(locale.GetLanguage(), VoiceCommandListenContext::TriggerPhrase);
  
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
  _baseScoreData._missMult = -5.0f;
  _baseScoreData._falsePositiveMult = -10.0f;
  _baseScoreData._targetScoreDiffMult = -0.001f;
}

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
      scoreAverage += ScoreParams(testCommandSet, commandType, paramA, paramB).CalculateScore("", _languagePhraseData);
      paramA += paramAInc;
    }
    
    scoreAverage /= Util::numeric_cast<double>(numSamplesPerParamBScore);
    
    PRINT_CH_INFO("VoiceCommands",
                  "VoiceCommandTuning.CalculateParamsForCommand",
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
    
    double scoreValue = ScoreParams(testCommandSet, commandType, paramA, gssParamBResult).CalculateScore("", _languagePhraseData);
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

float VoiceCommandTuning::CalculateContextConfigValue(ContextConfigSetter setterFunc,
                                                      double valueBegin,
                                                      double valueEnd,
                                                      bool useGoldenSectionSearch) const
{
  std::map<double, double> valuesLUT;
  auto gssScoring = [this, &valuesLUT, &setterFunc] (double testingValue) -> double
  {
    const auto& iter = valuesLUT.find(testingValue);
    if (iter != valuesLUT.end())
    {
      return iter->second;
    }
    
    ContextConfig testConfig;
    setterFunc(testConfig, Util::numeric_cast<float>(testingValue));
    bool doAudioPlayback = false;
    ContextConfigSharedPtr combinedConfig;
    auto paramScoreData = ScoreTriggerAndCommand(testConfig, doAudioPlayback, combinedConfig);
    double paramScore = paramScoreData.CalculateScore("", _languagePhraseData);
    
    valuesLUT[testingValue] = paramScore;
    return paramScore;
  };
  
  if (useGoldenSectionSearch)
  {
    constexpr double maxIterations = 20;
    return Util::numeric_cast<float>(GSSMaximum(gssScoring, valueBegin, valueEnd, maxIterations));
  }
  else
  {
    constexpr double maxIterations = 100;
    return Util::numeric_cast<float>(FindBestBruteForce(gssScoring, valueBegin, valueEnd, maxIterations));
  }
}

TestResultScoreData VoiceCommandTuning::ScoreParams(const std::set<VoiceCommandType>& testCommandSet,
                                                    VoiceCommandType commandType, int paramA, int paramB, bool playAudio) const
{
  // Set up the recognizer for this test:
  PhraseDataSharedPtr phraseData;
  
  // Make a copy of the phraselist so we can make modifications
  std::vector<Anki::Cozmo::VoiceCommand::PhraseDataSharedPtr> commandPhraseList;
  {
    const auto& commandPhraseListTmp = _languagePhraseData->GetPhraseDataList(testCommandSet);
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
  VoiceCommand::RecognitionSetupData setupData;
  setupData._isPhraseSpotted = true;
  setupData._allowsFollowup = false;
  setupData._phraseList = std::move(commandPhraseList);
  
  _recognizer->AddRecognitionDataAutoGen(recogIndex, _generalNNPath, setupData);
  _recognizer->SetRecognizerIndex(recogIndex);
  
  // Get the results for this configuration
  TestResultScoreData scoreData = CollectResults(testCommandSet, playAudio);
  
  // Remove the recognizer now that we're done with it
  _recognizer->RemoveRecognitionData(recogIndex);
  _recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
  
  return scoreData;
}

TestResultScoreData VoiceCommandTuning::ScoreParams(const std::set<VoiceCommandType>& testCommandSet, bool playAudio) const
{
  const AudioUtil::SpeechRecognizer::IndexType recogIndex = 0;
  VoiceCommand::RecognitionSetupData setupData;
  setupData._isPhraseSpotted = true;
  setupData._allowsFollowup = false;
  setupData._phraseList = _languagePhraseData->GetPhraseDataList(testCommandSet);
  
  _recognizer->AddRecognitionDataAutoGen(recogIndex, _generalNNPath, setupData);
  _recognizer->SetRecognizerIndex(recogIndex);
  
  // Get the results for this configuration
  TestResultScoreData scoreData = CollectResults(testCommandSet, playAudio);
  
  // Remove the recognizer now that we're done with it
  _recognizer->RemoveRecognitionData(recogIndex);
  _recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
  
  return scoreData;
}

TestResultScoreData VoiceCommandTuning::ScoreTriggerAndCommand(const ContextConfig& commandListConfig,
                                                               bool playAudio,
                                                               ContextConfigSharedPtr& out_CombinedConfig) const
{
  // Add the trigger phrase recognizer
  constexpr AudioUtil::SpeechRecognizer::IndexType triggerIndex = Util::EnumToUnderlying(VoiceCommandListenContext::TriggerPhrase);
  const auto& triggerDataIter = _contextDataMap.find(VoiceCommandListenContext::TriggerPhrase);
  if (triggerDataIter == _contextDataMap.end())
  {
    return TestResultScoreData{};
  }
  // Make a copy of the set so we can add to it
  auto totalCommandSet = triggerDataIter->second._commandsSet;
  
  _recognizer->AddRecognitionDataAutoGen(triggerIndex, _generalNNPath, _triggerPhraseSetup);
  
  // Set up the custom recognizer for the command list context
  const auto& contextConfigMap = _languagePhraseData->GetContextConfigs();
  const auto& configMapIter = contextConfigMap.find(VoiceCommandListenContext::CommandList);
  ContextConfigSharedPtr combinedConfig;
  if (configMapIter != contextConfigMap.end())
  {
    combinedConfig.reset(new ContextConfig(*configMapIter->second));
  }
  else
  {
    combinedConfig.reset(new ContextConfig);
  }
  combinedConfig->Combine(commandListConfig);
  out_CombinedConfig = combinedConfig;
  
  
  const auto& dataIter = _contextDataMap.find(VoiceCommandListenContext::CommandList);
  if (dataIter == _contextDataMap.end())
  {
    return TestResultScoreData{};
  }
  VoiceCommand::RecognitionSetupData setupData;
  setupData._contextConfig = combinedConfig;
  
  const auto& contextData = dataIter->second;
  const auto& commandListCommandSet = contextData._commandsSet;
  
  setupData._isPhraseSpotted = contextData._isPhraseSpotted;
  setupData._allowsFollowup = contextData._allowsFollowup;
  setupData._phraseList = _languagePhraseData->GetPhraseDataList(commandListCommandSet);
  
  // Add these commands to the total set to score
  totalCommandSet.insert(commandListCommandSet.begin(), commandListCommandSet.end());
  
  constexpr AudioUtil::SpeechRecognizer::IndexType commandListIndex = Util::EnumToUnderlying(VoiceCommandListenContext::CommandList);
  _recognizer->AddRecognitionDataAutoGen(commandListIndex, _generalNNPath, setupData);
  
  _recognizer->SetRecognizerIndex(triggerIndex);
  _recognizer->SetRecognizerFollowupIndex(commandListIndex);
  
  auto resetTask = [this] () {
    _recognizer->SetRecognizerIndex(triggerIndex);
    _recognizer->SetRecognizerFollowupIndex(commandListIndex);
  };
  
  // Score the results for this configuration
  TestResultScoreData scoreData = CollectResults(totalCommandSet, playAudio, resetTask);
  
  // Remove the recognizer now that we're done with it
  _recognizer->RemoveRecognitionData(triggerIndex);
  _recognizer->RemoveRecognitionData(commandListIndex);
  _recognizer->SetRecognizerIndex(AudioUtil::SpeechRecognizer::InvalidIndex);
  
  return scoreData;
}
  
#else

VoiceCommandTuning::VoiceCommandTuning(const Util::Data::DataPlatform& dataPlatform,
                                       const CommandPhraseData& commandPhraseData,
                                       Anki::Util::Locale locale,
                                       const std::string& sampleGroupDir)
: _contextDataMap(commandPhraseData.GetContextData())
, _languagePhraseData(commandPhraseData.GetLanguagePhraseData(locale.GetLanguage()))
{}

TestResultScoreData VoiceCommandTuning::ScoreParams(const std::set<VoiceCommandType>& testCommandSet, bool playAudio) const
{
  return TestResultScoreData();
}

TestResultScoreData VoiceCommandTuning::ScoreTriggerAndCommand(const ContextConfig& commandListConfig,
                                                               bool playAudio,
                                                               ContextConfigSharedPtr& out_CombinedConfig) const
{
  return TestResultScoreData();
}

#endif // THF_FUNCTIONALITY

TestResultScoreData VoiceCommandTuning::CollectResults(const std::set<VoiceCommandType>& testCommandSet, bool playAudio, ResetTask resetTask) const
{
  // Load up the test data
  Json::Value jsonObject{};
  const std::string& phraseDataFilename = "samplePhraseData.json";
  Anki::Util::Data::DataPlatform::readAsJson(Util::FileUtils::FullFilePath({_languageSampleDir, phraseDataFilename}), jsonObject);
  SampleKeyphraseData sampleData{};
  sampleData.Init(jsonObject);
  const auto& fileToCommandData = sampleData.GetFileToDataMap();
  
  // score the results for this configuration
  TestResultScoreData scoreData = _baseScoreData;

  for (const auto& iter : fileToCommandData)
  {
    const auto& filename = iter.first;
    const auto& commandDataMap = iter.second;
    
    // Set up audio input
    _audioInput->ReadFile(Util::FileUtils::FullFilePath( {_languageSampleDir, filename} ));
    PRINT_CH_INFO("VoiceCommands", "VoiceCommandTuning.ScoreParams","Processing %s", filename.c_str());
    
    AudioUtil::AudioPlayer audioPlayer;
    if (playAudio)
    {
      audioPlayer.StartPlaying(_audioInput->GetAudioSamples().begin(), _audioInput->GetAudioSamples().end());
      std::this_thread::sleep_for(std::chrono::milliseconds(AudioUtil::kTimePerAudioChunk_ms));
    }
    
    // Tell the audio input to go through the audio data and deliver it
    constexpr bool addBeginningSilence = true;
    constexpr bool addEndingSilence = true;
    _audioInput->DeliverAudio(playAudio, addBeginningSilence, addEndingSilence);
    
    // If we were playing audio we need to make sure it's stopped
    if (playAudio)
    {
      audioPlayer.StopPlaying();
    }
    
    auto& resultsDataForFile = scoreData._filenameToResultsMap[filename];
    auto& resultsVectorForFile = resultsDataForFile._actualResults;
    while (_recogProcessor->HasResults())
    {
      resultsVectorForFile.push_back(_recogProcessor->PopNextResult());
    }
    
    // fill out important information on the scoredata so the calculated score will be correct
    auto& expectedFileResultsForSetCommands = resultsDataForFile._expectedResults;
    for (const auto& commandDataIter : commandDataMap)
    {
      if (testCommandSet.find(commandDataIter.first) != testCommandSet.end())
      {
        // If the test command set includes this command, include the expected count for it for this file
        expectedFileResultsForSetCommands.insert(commandDataIter);
      }
    }
    
    // Do whatever task we're supposed to do at the end of a file
    if (resetTask)
    {
      resetTask();
    }
  }
  
  return scoreData;
}

double TestResultScoreData::CalculateScore(const std::string phrase,
                                           const ConstLanguagePhraseDataSharedPtr& phraseDataLookup)
{
  // Go through the results from each file and get an average score of the results we got.
  int numResults = 0;
  double averageScore = 0;
  
  int hits = 0;
  int misses = 0;
  int falsePositives = 0;
  
  bool isEmptyPhrase = phrase.empty();
  for (const auto& iter : _filenameToResultsMap)
  {
    const auto& resultsData = iter.second;
    const auto& resultsList = resultsData._actualResults;
    
    // Make a copy of the expected results so we can decrement them as we go through what we actually got
    CommandCounts expectedResults = resultsData._expectedResults;
    for (const auto& result : resultsList)
    {
      const auto& commandType = phraseDataLookup->GetDataForPhrase(result.first)->GetVoiceCommandType();
      
      auto remainingExpectedCountIter = expectedResults.find(commandType);
      if (remainingExpectedCountIter == expectedResults.end())
      {
        ++falsePositives;
      }
      else if (remainingExpectedCountIter->second == 0)
      {
        ++falsePositives;
        expectedResults.erase(remainingExpectedCountIter);
      }
      else if (remainingExpectedCountIter->second > 0)
      {
        ++hits;
        --remainingExpectedCountIter->second;
        if (remainingExpectedCountIter->second == 0)
        {
          expectedResults.erase(remainingExpectedCountIter);
        }
      }
      
      if (isEmptyPhrase || result.first == phrase)
      {
        averageScore += result.second;
        ++numResults;
      }
    } // for (const auto& result : resultsList)
    
    // count up any remaining expected results that are in our test set as misses
    for (const auto& iter : expectedResults)
    {
      misses += iter.second;
    }
  } // for (const auto& iter : _filenameToResultsMap)
  
  if (numResults > 0)
  {
    averageScore = averageScore / Anki::Util::numeric_cast<double>(numResults);
  }
  
  _maxScore = Anki::Util::numeric_cast<double>(hits + misses) * _hitMult;
  
  // We want the deviation from the target score as an absolute value
  const double& targetScoreDiff = Anki::Util::Abs(averageScore - _kTargetScoreConstant);
  
  const double hitScore = Util::numeric_cast<double>(hits) * _hitMult;
  const double missScore = Util::numeric_cast<double>(misses) * _missMult;
  const double falsePositiveScore = Util::numeric_cast<double>(falsePositives) * _falsePositiveMult;
  const double targetScoreDiffScore = targetScoreDiff * _targetScoreDiffMult;
  
  _score = hitScore + missScore + falsePositiveScore + targetScoreDiffScore;
  return _score;
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

double VoiceCommandTuning::FindBestBruteForce(std::function<double(double)> function, double a, double b, int iterations)
{
  if (iterations < 2)
  {
    return a;
  }
  
  double delta = (b - a) / Util::numeric_cast<double>(iterations - 1);
  
  double bestInput = a;
  double bestScore = function(a);
  --iterations;
  
  double currentInput = a + delta;
  while (iterations--)
  {
    double nextScore = function(currentInput);
    if (nextScore > bestScore)
    {
      bestInput = currentInput;
      bestScore = nextScore;
    }
    currentInput += delta;
  }
  
  return bestInput;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
