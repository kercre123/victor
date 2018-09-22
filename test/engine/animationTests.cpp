/**
 * File: animationTests.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/6/18
 *
 * Description: Tests related to animation loading/playback
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "gtest/gtest.h"

#include "engine/components/dataAccessorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"


using namespace Anki;
using namespace Vector;

extern Anki::Vector::CozmoContext* cozmoContext;


TEST(AnimationTest, AnimationLoading)
{
  const auto kPathToDevData = "assets/dev_animation_data";
  const auto fullPath = cozmoContext->GetDataPlatform()->GetResourcePath(kPathToDevData);
  const bool useFullPath = true;
  const bool shouldRecurse = false;

  Robot robot(0, cozmoContext);
  auto platform = cozmoContext->GetDataPlatform();
  auto spritePaths = robot.GetComponent<DataAccessorComponent>().GetSpritePaths();
  auto spriteSequenceContainer = robot.GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
  std::atomic<float> loadingCompleteRatio(0);
  std::atomic<bool>  abortLoad(false);

  CannedAnimationContainer jsonAnimContainer;
  // Load JSON animations
  {
    const std::vector<const char*> jsonExt = {"json"};
    auto jsonFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, jsonExt, shouldRecurse);

    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer, 
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : jsonFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &jsonAnimContainer);
    }
  }

  // Load Binary animations
  CannedAnimationContainer binaryAnimContainer;
  {
    const std::vector<const char*> binExt = {"bin"};
    auto binaryFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, binExt, shouldRecurse);

    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer, 
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : binaryFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &binaryAnimContainer);
    }
  }

  // Compare the maps
  const auto jsonAnimNames = jsonAnimContainer.GetAnimationNames();
  const auto binaryAnimNames = binaryAnimContainer.GetAnimationNames();
  
  const bool sameSize = (jsonAnimNames.size() == binaryAnimNames.size());
  if(!sameSize){
    PRINT_NAMED_WARNING("AnimationTest.AnimationLoading.SizesDoNotMatch",
                        "There are %zu json animations and %zu binary animations - \
                        may be an issue copying assets for test",
                        jsonAnimNames.size(), binaryAnimNames.size());
    for(const auto& name: jsonAnimNames){
      const auto it = std::find(binaryAnimNames.begin(), binaryAnimNames.end(), name);
      if(it == binaryAnimNames.end()){
        PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.MissingAnim", 
                          "Animation %s exists in JSON format, but not binary",
                          name.c_str());
      }
    }
    for(const auto& name: binaryAnimNames){
      const auto it = std::find(jsonAnimNames.begin(), jsonAnimNames.end(), name);
      if(it == jsonAnimNames.end()){
        PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.MissingAnim", 
                          "Animation %s exists in binary format, but not JSON",
                          name.c_str());
      }
    }
  }

  // Make sure animation contents are the same - use binary as source of truth
  // above checks will fail if there are animations missing from binary
  for(const auto& name: binaryAnimNames){
    const bool animExists = jsonAnimContainer.HasAnimation(name);
    if(!animExists){
      PRINT_NAMED_WARNING("AnimationTest.AnimationLoading.MissingJSONAnim - may be an issue copying assets for test",
                          "JSON container does not have animation %s",
                          name.c_str());
      continue;
    }
    Animation* binaryAnim = binaryAnimContainer.GetAnimation(name);
    Animation* jsonAnim   = jsonAnimContainer.GetAnimation(name);
    ASSERT_TRUE(binaryAnim != nullptr);
    ASSERT_TRUE(jsonAnim != nullptr);
    
    const bool areAnimationsEquivalent = (*binaryAnim == *jsonAnim);
    EXPECT_TRUE(areAnimationsEquivalent);
    if(!areAnimationsEquivalent){
      PRINT_NAMED_ERROR("AnimationTest.AnimationLoading.AnimationsDoNotMatch",
                        "Binary animation and JSON animation %s do not match",
                        name.c_str());
    }
  }

} // AnimationTest.AnimationLoading


// Ensure that copying animations/keyframes doesn't result in any lost information
TEST(AnimationTest, AnimationCopying)
{
  const auto kPathToDevData = "assets/animations";
  const auto fullPath = cozmoContext->GetDataPlatform()->GetResourcePath(kPathToDevData);
  const bool useFullPath = true;
  const bool shouldRecurse = false;
  
  Robot robot(0, cozmoContext);
  auto platform = cozmoContext->GetDataPlatform();
  auto spritePaths = robot.GetComponent<DataAccessorComponent>().GetSpritePaths();
  auto spriteSequenceContainer = robot.GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
  std::atomic<float> loadingCompleteRatio(0);
  std::atomic<bool>  abortLoad(false);
  
  
  // Load Binary animations
  CannedAnimationContainer binaryAnimContainer;
  {
    const std::vector<const char*> binExt = {"bin"};
    auto binaryFilePaths = Util::FileUtils::FilesInDirectory(fullPath, useFullPath, binExt, shouldRecurse);
    
    CannedAnimationLoader animLoader(platform,
                                     spritePaths, spriteSequenceContainer,
                                     loadingCompleteRatio, abortLoad);
    for(const auto& fullPath : binaryFilePaths){
      animLoader.LoadAnimationIntoContainer(fullPath, &binaryAnimContainer);
    }
  }
  
  
  // Copy each animation and make sure that the contents stay the same
  const auto binaryAnimNames = binaryAnimContainer.GetAnimationNames();

  for(const auto& name: binaryAnimNames){
    Animation* binaryAnim = binaryAnimContainer.GetAnimation(name);
    ASSERT_TRUE(binaryAnim != nullptr);
    
    Animation copy = *binaryAnim;
    
    const bool areAnimationsEquivalent = (*binaryAnim == copy);
    EXPECT_TRUE(areAnimationsEquivalent);
    if(!areAnimationsEquivalent){
      PRINT_NAMED_ERROR("AnimationTest.AnimationCopying.AnimationsDoNotMatch",
                        "Animation %s has data which does not copy properly",
                        name.c_str());
    }
  }
  
} // AnimationTest.AnimationCopying

