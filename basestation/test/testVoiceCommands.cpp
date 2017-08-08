/**
 * File: testVoiceCommands.cpp
 *
 * Author:  Lee Crippen
 * Created: 06/14/2017
 *
 * Description: Unit/regression tests for Cozmo's Voice Commands
 *
 * Copyright: Anki, Inc. 2017
 *
 * --gtest_filter=VoiceCommands*
 **/

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/contextConfig.h"
#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandTuning.h"

#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/types.h"

#include "util/logging/logging.h"

#include "gtest/gtest.h"

#include <functional>

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::VoiceCommand;

//----------------------------------------------------------------------------------------------------------------------
// "calculating" examples, uses VoiceCommandTuning to determine good param values (for phrasespotting) or good config
// values (for the command list context)
//----------------------------------------------------------------------------------------------------------------------

// This enables using the calculation functionality in VoiceCommandTuning
// Committing as disabled since it's more for finding values than unit testing, but we don't want
// code to rot so it's still in the first test below
#define DO_CALCULATION_TEST 0

TEST(VoiceCommands, SpeechRecognizer_calculation_params)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  // Get the test data
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::HeyCozmo };
  VoiceCommandType commandType = VoiceCommandType::HeyCozmo;
  
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, currentLocale};
  
  if (DO_CALCULATION_TEST)
  {
    // Use the sample data in the phrase directory to attempt to calculate good values for params A and B
    int paramA = 0;
    int paramB = 0;
    vcTuning.CalculateParamsForCommand(testCommandSet, commandType, paramA, paramB);
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
  }
} // SpeechRecognizer_calculation_params


TEST(VoiceCommands, SpeechRecognizer_calculation_configs)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, currentLocale};
  
  if (DO_CALCULATION_TEST)
  {
    ContextConfig testConfig;
    
    constexpr double searchNOTABegin = 0.01;
    constexpr double searchNOTAEnd = 0.99;
    constexpr bool useGoldenSectionSearch = false;
    const float searchNOTA = vcTuning.CalculateContextConfigValue([](ContextConfig& c,float n){c.SetSearchNOTA(n);},
                                                                  searchNOTABegin, searchNOTAEnd, useGoldenSectionSearch);
    testConfig.SetSearchNOTA(searchNOTA);
    
    constexpr double searchBeamBegin = 50;
    constexpr double searchBeamEnd = 5000;
    const float searchBeam = vcTuning.CalculateContextConfigValue([](ContextConfig& c,float n){c.SetSearchBeam(n);},
                                                                  searchBeamBegin, searchBeamEnd, useGoldenSectionSearch);
    testConfig.SetSearchBeam(searchBeam);
    
    constexpr double recogMinDurationBegin = 40;
    constexpr double recogMinDurationEnd = 1000;
    const float minDuration = vcTuning.CalculateContextConfigValue([](ContextConfig& c,float n){c.SetRecogMinDuration(n);},
                                                                   recogMinDurationBegin, recogMinDurationEnd, useGoldenSectionSearch);
    testConfig.SetRecogMinDuration(minDuration);
    
    bool doAudioPlayback = false;
    ContextConfigSharedPtr combinedConfig;
    auto paramScoreData = vcTuning.ScoreTriggerAndCommand(testConfig, doAudioPlayback, combinedConfig);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s searchNOTA:%f searchBeam:%f recogMinDuration:%f score:%f maxScore:%f scoreRatio:%f",
             currentLocale.GetLocaleString().c_str(),
             combinedConfig->GetSearchNOTA(), combinedConfig->GetSearchBeam(), combinedConfig->GetRecogMinDuration(),
             paramScore, maxParamScore, paramScore/maxParamScore);
  }
} // SpeechRecognizer_calculation_configs


//----------------------------------------------------------------------------------------------------------------------
// "simple" tests, checking single commands against phrasespotted contexts
//----------------------------------------------------------------------------------------------------------------------

using VCSetType = std::set<VoiceCommandType>;

std::string GetCommandSetString(const VCSetType& commandSet)
{
  if (commandSet.size() < 1)
  {
    return "";
  }
  
  auto setIter = commandSet.begin();
  
  std::stringstream sstream;
  sstream << "{ " << EnumToString(*(setIter++));
  for (; setIter != commandSet.end(); ++setIter)
  {
    sstream << " " << EnumToString(*setIter);
  }
  
  sstream << " }";
  
  return sstream.str();
}


static double GetScoreRatio(const CommandPhraseData& commandPhraseData, const Util::Locale& locale,
                            const Util::Data::DataPlatform& dataPlatform, const VCSetType& commandSet)
{
  // Init the tuning class
  VoiceCommandTuning vcTuning{dataPlatform, commandPhraseData, locale};
  
  bool doAudioPlayback = false;
  auto paramScoreData = vcTuning.ScoreParams(commandSet, doAudioPlayback);
  double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(locale.GetLanguage()));
  double maxParamScore = paramScoreData.GetMaxScore();
  double scoreRatio = paramScore / (maxParamScore != 0.0 ? maxParamScore : 1.0);
  LOG_INFO("TestVoiceCommand",
           "Locale:%s CommandSet:%s score:%f maxScore:%f ratio:%f",
           locale.GetLocaleString().c_str(), GetCommandSetString(commandSet).c_str(),
           paramScore, maxParamScore, scoreRatio);
  
  return scoreRatio;
}

const static auto& _kYesNoCommandSet = VCSetType{VoiceCommandType::YesPlease, VoiceCommandType::NoThankYou};

TEST(VoiceCommands, SpeechRecognizer_simple_en)
{
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  
  auto unitTest = std::bind(GetScoreRatio, commandPhraseData, currentLocale, *cozmoContext->GetDataPlatform(),
                            std::placeholders::_1);
  
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::HeyCozmo}), 0.879210, 0.01);
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::Continue}), 0.684333, 0.01);
  ASSERT_NEAR(unitTest(_kYesNoCommandSet), 0.405925, 0.01);
}


TEST(VoiceCommands, SpeechRecognizer_simple_de)
{
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  auto currentLocale = Anki::Util::Locale{"de", "DE"};
  
  
  auto unitTest = std::bind(GetScoreRatio, commandPhraseData, currentLocale, *cozmoContext->GetDataPlatform(),
                            std::placeholders::_1);
  
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::Continue}), 0.994974, 0.01);
  ASSERT_NEAR(unitTest(_kYesNoCommandSet), 0.998900, 0.01);
}


TEST(VoiceCommands, SpeechRecognizer_simple_fr)
{
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  auto currentLocale = Anki::Util::Locale{"fr", "FR"};
  
  
  auto unitTest = std::bind(GetScoreRatio, commandPhraseData, currentLocale, *cozmoContext->GetDataPlatform(),
                            std::placeholders::_1);
  
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::Continue}), 0.998990, 0.01);
  ASSERT_NEAR(unitTest(_kYesNoCommandSet), 0.999194, 0.01);
}


TEST(VoiceCommands, SpeechRecognizer_simple_ja)
{
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  auto currentLocale = Anki::Util::Locale{"ja", "JP"};
  
  
  auto unitTest = std::bind(GetScoreRatio, commandPhraseData, currentLocale, *cozmoContext->GetDataPlatform(),
                            std::placeholders::_1);
  
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::HeyCozmo}), 0.996757, 0.01);
  ASSERT_NEAR(unitTest(VCSetType{VoiceCommandType::Continue}), 0.999028, 0.01);
  ASSERT_NEAR(unitTest(_kYesNoCommandSet), 0.999978, 0.01);
}


//----------------------------------------------------------------------------------------------------------------------
// "complex" tests, checking both the trigger phrase and command within the same clip
//----------------------------------------------------------------------------------------------------------------------

TEST(VoiceCommands, SpeechRecognizer_complex_en)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::HeyCozmo };
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, currentLocale};
  
  // Testing our saved config values
  {
    ContextConfig testConfig;
    
    bool doAudioPlayback = false;
    ContextConfigSharedPtr combinedConfig;
    auto paramScoreData = vcTuning.ScoreTriggerAndCommand(testConfig, doAudioPlayback, combinedConfig);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    double scoreRatio = paramScore / (maxParamScore != 0.0 ? maxParamScore : 1.0);
    LOG_INFO("TestVoiceCommand",
             "Locale:%s searchNOTA:%f searchBeam:%f recogMinDuration:%f score:%f maxScore:%f scoreRatio:%f",
             currentLocale.GetLocaleString().c_str(),
             combinedConfig->GetSearchNOTA(), combinedConfig->GetSearchBeam(), combinedConfig->GetRecogMinDuration(),
             paramScore, maxParamScore, scoreRatio);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(scoreRatio, 0.616636, 0.01);
  }
}
