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

#include "anki/common/types.h"

#include "util/logging/logging.h"

#include "gtest/gtest.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

extern Anki::Cozmo::CozmoContext* cozmoContext;

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
  double targetScore = 500;
  
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
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
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::HeyCozmo };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
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


TEST(VoiceCommands, SpeechRecognizer_en_HeyCozmo)
{
  
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::HeyCozmo };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
  // Testing HeyCozmo
  {
    VoiceCommandType commandType = VoiceCommandType::HeyCozmo;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    double scoreRatio = paramScore / (maxParamScore != 0.0 ? maxParamScore : 1.0);
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f ratio:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType),
             paramA, paramB, paramScore, maxParamScore, scoreRatio);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(scoreRatio, 0.929280, 0.01);
  }
} // SpeechRecognizer_en_HeyCozmo


TEST(VoiceCommands, SpeechRecognizer_en_trigger_and_commands)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"en", "US"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::HeyCozmo };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
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
    ASSERT_NEAR(scoreRatio, 0.672162, 0.01);
  }
} // SpeechRecognizer_en_trigger_and_commands


TEST(VoiceCommands, SpeechRecognizer_de)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"de", "DE"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::YesPlease, VoiceCommandType::NoThankYou };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
  // Testing YesPlease
  {
    VoiceCommandType commandType = VoiceCommandType::YesPlease;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 79.903000, 1.0);
  }
  
  // Testing NoThankYou
  {
    VoiceCommandType commandType = VoiceCommandType::NoThankYou;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 79.900000, 1.0);
  }
} // SpeechRecognizer_de


TEST(VoiceCommands, SpeechRecognizer_fr)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"fr", "FR"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::YesPlease, VoiceCommandType::NoThankYou };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
  // Testing YesPlease
  {
    VoiceCommandType commandType = VoiceCommandType::YesPlease;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 80.000000, 1.0);
  }
  
  // Testing NoThankYou
  {
    VoiceCommandType commandType = VoiceCommandType::NoThankYou;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 79.999500, 1.0);
  }
} // SpeechRecognizer_fr


TEST(VoiceCommands, SpeechRecognizer_ja)
{
  using namespace Anki::Cozmo::VoiceCommand;
  
  // Make sure voice command config data is loaded
  cozmoContext->GetDataLoader()->LoadVoiceCommandConfigs();
  CommandPhraseData commandPhraseData{};
  ASSERT_TRUE(commandPhraseData.Init(cozmoContext->GetDataLoader()->GetVoiceCommandConfig()));
  
  auto currentLocale = Anki::Util::Locale{"ja", "JP"};
  std::set<VoiceCommandType> testCommandSet = { VoiceCommandType::YesPlease, VoiceCommandType::NoThankYou };
  
  // We use a very hand-wavy "target" score (compared to the score we get back from the recognizer) as the last
  // bit of score differentiation when examining the performance of a recognizer against a sample data set
  constexpr double targetScore = 500;
  
  // Init the tuning class
  VoiceCommandTuning vcTuning{*cozmoContext->GetDataPlatform(), commandPhraseData, targetScore, currentLocale};
  
  // Testing YesPlease
  {
    VoiceCommandType commandType = VoiceCommandType::YesPlease;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 69.924400, 1.0);
  }
  
  // Testing NoThankYou
  {
    VoiceCommandType commandType = VoiceCommandType::NoThankYou;
    const auto& phraseDataList = commandPhraseData.GetPhraseDataList(currentLocale.GetLanguage(), {commandType} );
    
    // We currently only support testing params for commands that use a single phrase
    ASSERT_EQ(phraseDataList.size(), 1);
    
    PhraseDataSharedPtr phraseData = phraseDataList.front();
    int paramA = phraseData->GetParamA();
    int paramB = phraseData->GetParamB();
    
    bool doAudioPlayback = false;
    auto paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.CalculateScore("", commandPhraseData.GetLanguagePhraseData(currentLocale.GetLanguage()));
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 69.924400, 1.0);
  }
} // SpeechRecognizer_ja

