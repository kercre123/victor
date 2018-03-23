/**
 * File: testUserIntentsParsing.cpp
 *
 * Author: ross
 * Created: 2018 Mar 20
 *
 * Description: tests that intents from various sources are parsed into valid user intents
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentMap.h"
#include "engine/cozmoContext.h"
#include "gtest/gtest.h"
#include "test/engine/behaviorComponent/testBehaviorFramework.h"
#include "test/engine/behaviorComponent/testIntentsFramework.h"
#include "util/fileUtils/fileUtils.h"

#include "json/json.h"

#include <sstream>


using namespace Anki;
using namespace Anki::Cozmo;

extern CozmoContext* cozmoContext;

namespace {
  
TEST(UserIntentsParsing, CloudSampleFileParses)
{
  Json::Value config;
  
  // eventually we want this set by the build script that runs the tests based on the script that
  // is run. For now, load the temp file that has been committed to the repo.
  if( 0 ) {
    
    std::string inFilename;
    const char* szFilename = getenv("ANKI_TEST_INTENT_SAMPLE_FILE");
    if( szFilename != nullptr ) {
      inFilename = szFilename;
    } else {
      return;
    }
    
    const std::string fileContents{ Util::FileUtils::ReadFile( inFilename ) };
    
    Json::Reader reader;
    Json::Value config;
    const bool parsedOK = reader.parse(fileContents, config, false);
    EXPECT_TRUE(parsedOK);
    
  } else {
    
    const std::string jsonFilename = "test/aiTests/tempCloudIntentSamples.json";
    const bool parsedOK = cozmoContext->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources,
                                                                      jsonFilename,
                                                                      config);
    EXPECT_TRUE( parsedOK );
    
  }
  
  EXPECT_FALSE( config.empty() );
  
  TestBehaviorFramework tbf;
  tbf.InitializeStandardBehaviorComponent();
  
  TestIntentsFramework tih;
  
  // all labels we consider "complete" should be hit once by the samples
  std::set<std::string> completedLabels = tih.GetCompletedLabels();
  std::set<std::string> sampleLabels;
  
  
  for( const auto& sample : config ) {
    std::stringstream ss;
    ss << sample;
    
    UserIntent intent;
    const bool parsed = tih.TryParseCloudIntent( tbf, ss.str(), &intent );
    EXPECT_TRUE( parsed ) << "could not parse sample cloud intent " << ss.str();
    
    if( intent.GetTag() != UserIntentTag::unmatched_intent ) {
      const auto& label = tih.GetLabelForIntent( intent );
      if( !label.empty() ) {
        sampleLabels.insert( label );
      }
    }
  }
  
  // check that all labels we consider completed have a sample cloud intent
  auto itSamples = sampleLabels.begin();
  auto itCompleted = completedLabels.begin();
  while( itSamples != sampleLabels.end() && itCompleted != completedLabels.end() ) {
    while( itSamples != sampleLabels.end() ) {
      if( *itSamples == *itCompleted ) {
        ++itSamples;
        break;
      } else {
        ++itSamples;
      }
    }
    ++itCompleted;
  }
  
  EXPECT_FALSE(itSamples == sampleLabels.end() && itCompleted != completedLabels.end())
    << "Could not find " << *itCompleted << " and maybe more ";
  
  
  // it should be ok if user_intent_map has a cloud intent that isnt completed, but it should
  // exist in dialogflow samples; otherwise it should be considered garbage.
  UserIntentMap intentMap( cozmoContext->GetDataLoader()->GetUserIntentConfig() );
  std::vector<std::string> cloudIntentsList = intentMap.DevGetCloudIntentsList();
  for( const auto& cloudName : cloudIntentsList ) {
    bool found = false;
    for( const auto& elem : config ) {
      if( elem["intent"] == cloudName ) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE( found ) << "Could not find user_intent_map cloud intent " << cloudName << " in sample file";
  }
}
  
TEST(UserIntentsParsing, CompletedInCloudList)
{
  // tests that every completed intent has a match for both app and cloud in user_intent_map.
  
  UserIntentMap intentMap( cozmoContext->GetDataLoader()->GetUserIntentConfig() );
  
  std::vector<std::string> cloudIntentsList = intentMap.DevGetCloudIntentsList();
  std::vector<std::string> appIntentsList = intentMap.DevGetAppIntentsList();
  
  TestIntentsFramework tih;
  std::set<std::string> completedLabels = tih.GetCompletedLabels();
  for( const auto& completedLabel : completedLabels ) {
    const auto intent = tih.GetCompletedIntent(completedLabel);
    const auto intentTag = intent.GetTag();
    
    // does this exist in cloudIntentsList AND appIntentsList?
    auto itCloud = std::find_if( cloudIntentsList.begin(), cloudIntentsList.end(), [&](const auto& x ) {
      return (intentMap.GetUserIntentFromCloudIntent( x ) == intentTag);
    });
    EXPECT_TRUE( itCloud != cloudIntentsList.end() ) << "Could not find " << UserIntentTagToString(intentTag) << " in cloud intent list";
    
    auto itApp = std::find_if( appIntentsList.begin(), appIntentsList.end(), [&](const auto& x ) {
      return (intentMap.GetUserIntentFromAppIntent( x ) == intentTag);
    });
    EXPECT_TRUE( itApp != appIntentsList.end() ) << "Could not find " << UserIntentTagToString(intentTag) << " in app intent list";
  }
}


} // namespace
