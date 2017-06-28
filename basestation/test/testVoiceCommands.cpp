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
#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandTuning.h"

#include "anki/common/types.h"

#include "util/logging/logging.h"

#include "gtest/gtest.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

extern Anki::Cozmo::CozmoContext* cozmoContext;

// This is an example of how to use the param calculation functionalty in VoiceCommandTuning
// Committing as disabled since it's more for finding values than unit testing, but we don't want
// code to rot so it's still in the first test below
#define DO_PARAM_CALCULATION_TEST 0

TEST(VoiceCommands, SpeechRecognizer_param_calculation)
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
  
  if (DO_PARAM_CALCULATION_TEST)
  {
    // Use the sample data in the phrase directory to attempt to calculate good values for params A and B
    int paramA = 0;
    int paramB = 0;
    vcTuning.CalculateParamsForCommand(testCommandSet, commandType, paramA, paramB);
    
    bool doAudioPlayback = false;
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
  }
} // SpeechRecognizer_param_calculation


TEST(VoiceCommands, SpeechRecognizer_en)
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 139.404857, 1.0);
  }
} // SpeechRecognizer_en


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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 79.978000, 1.0);
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
    const auto& paramScoreData = vcTuning.ScoreParams(testCommandSet, commandType, paramA, paramB, doAudioPlayback);
    double paramScore = paramScoreData.GetScore();
    double maxParamScore = paramScoreData.GetMaxScore();
    LOG_INFO("TestVoiceCommand",
             "Locale:%s Command:%s paramA:%d paramB:%d score:%f maxScore:%f",
             currentLocale.GetLocaleString().c_str(), EnumToString(commandType), paramA, paramB, paramScore, maxParamScore);
    
    // Verify that our historical observed score threshold for this test is maintained
    ASSERT_NEAR(paramScore, 79.995500, 1.0);
  }
} // SpeechRecognizer_ja

