/**
 * File: blockConfigurationManager.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Tracks and caches block configurations for behaviors to query
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"

#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/observableObject.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {
  
namespace {
// minumum distance that a block has to move for us to update configurations
const float constexpr kMinBlockMoveUpdateThreshold_mm = 15.f;
const float constexpr kMinBlockRotationUpdateThreshold_rad = DEG_TO_RAD(30);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlockConfigurationManager::BlockConfigurationManager(const Robot& robot)
: _robot(robot)
{
  //Initialize configuration map
  for(ConfigurationType type =(ConfigurationType)0; type != ConfigurationType::Count; ++type){
    _configToCacheMap.insert(std::make_pair(type, std::vector<BlockConfigPtr>()));
  }
  
} // BlockConfigurationManager() Constructor

  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<BlockConfigWeakPtr> BlockConfigurationManager::GetConfigurationsForType(ConfigurationType type) const
{
  std::vector<BlockConfigWeakPtr> configs;
  auto cacheIter = _configToCacheMap.find(type);
  ASSERT_NAMED_EVENT(cacheIter != _configToCacheMap.end(), "BlockConfigurationManager.GetConfigurationForType.InvalidType",
                       "InvalidType %d passed in to get configurations", type);
  
  
  for(const auto& config: cacheIter->second){
    configs.push_back(config);
  }
  
  return configs;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<BlockConfigWeakPtr> BlockConfigurationManager::GetConfigurationsContainingObjectForType(ConfigurationType type, const ObjectID& objectID) const
{
  std::vector<BlockConfigWeakPtr> configs;
  auto cacheIter = _configToCacheMap.find(type);
  ASSERT_NAMED_EVENT(cacheIter != _configToCacheMap.end(), "BlockConfigurationManager.GetConfigurationsContainingObjectForType.InvalidType",
                     "InvalidType %d passed in to get configurations", type);
  
  for(const auto& config: cacheIter->second){
    if(config->ContainsBlock(objectID)){
      configs.push_back(config);
    }
  }
  
  return configs;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockConfigurations::StackWeakPtr BlockConfigurationManager::GetTallestStack() const
{
  std::vector<ObjectID> bottomBlocksToIgnore;
  return GetTallestStack(bottomBlocksToIgnore);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockConfigurations::StackWeakPtr BlockConfigurationManager::GetTallestStack(const std::vector<ObjectID>& baseBlocksToIgnore) const
{
  auto stackCache = _configToCacheMap.find(ConfigurationType::StackOfCubes)->second;
  if(stackCache.empty()){
    return std::make_shared<StackOfCubes>();
  }
  
  using StackIteratorConst = std::vector<BlockConfigPtr>::const_iterator;
  auto tallestStack = BlockConfiguration::AsStackPtr(stackCache.front());
  for(StackIteratorConst stackIt = stackCache.begin(); stackIt != stackCache.end(); ++stackIt){
    auto compareStackPtr = BlockConfiguration::AsStackPtr((*stackIt));
    if(compareStackPtr->GetStackHeight() > tallestStack->GetStackHeight()){
      tallestStack = compareStackPtr;
    }
  }
  
  return tallestStack;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlockConfigurationManager::CheckForPyramidBaseBelowObject(const ObservableObject* object, PyramidBaseWeakPtr& pyramidBase) const
{
  auto pyramidBaseCache = _configToCacheMap.find(ConfigurationType::PyramidBase)->second;
  for(const auto& storedPyramid: pyramidBaseCache){
    auto pyramidBasePtr = BlockConfiguration::AsPyramidBasePtr(storedPyramid);
    if(pyramidBasePtr->ObjectIsOnTopOfBase(_robot, object)){
      pyramidBase = pyramidBasePtr;
      return true;
    }
  }
  
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::Update()
{
  if(_objectsPoseChangedThisTick.empty()){
    // nothing to update
    return;
  }
  
  // build a list of objects that have moved past their last configuration check threshold
  if(DidAnyObjectsMovePastThreshold()){
    UpdateAllBlockConfigs();
    PrunePyramidBases();
    UpdateLastConfigCheckBlockPoses();
  }
  
  _objectsPoseChangedThisTick.clear();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlockConfigurationManager::DidAnyObjectsMovePastThreshold()
{
  bool anyObjectMovedPastThreshold = false;
  
  auto IsObjectPartOfAnyConfiguration = [this](const ObjectID& id){
    for(ConfigurationType type =(ConfigurationType)0; type != ConfigurationType::Count; ++type){
      if(IsObjectPartOfConfigurationType(type, id)){
        return true;
      }
    }
    return false;
  };
  
  // check to see if any block has moved past our update configuration threshold
  for(const auto& objectID: _objectsPoseChangedThisTick){
    const ObservableObject* blockMoved = _robot.GetBlockWorld().GetObjectByID(objectID);
    if(blockMoved == nullptr){
      continue;
    }
    
    auto lastPoseMapIter = _objectIDToLastPoseConfigurationUpdateMap.find(objectID);
    if(lastPoseMapIter != _objectIDToLastPoseConfigurationUpdateMap.end()){
      if(blockMoved->IsPoseStateUnknown()){
        // The block's pose was known at some point and has changed to unknown -
        // It may have been part of a configuration - if it was we need to rebuild the configurations
        if(IsObjectPartOfAnyConfiguration(blockMoved->GetID())){
          anyObjectMovedPastThreshold = true;
          break;
        }
      }else{
        // the blocks pose is known, so check if it moved past our threshold
        const auto newBlockPose = blockMoved->GetPose();
        const bool isBlockWithinTolerence = lastPoseMapIter->second.IsSameAs(newBlockPose, kMinBlockMoveUpdateThreshold_mm, kMinBlockRotationUpdateThreshold_rad);
        if(!isBlockWithinTolerence){
          anyObjectMovedPastThreshold = true;
          break;
        }
      }
    }else{
      // the object is not in our pose map, so check to see if it is part of any configurations
      anyObjectMovedPastThreshold = true;
    }
  } // end for _objectsPoseChangedThisTick
  
  return anyObjectMovedPastThreshold;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::UpdateAllBlockConfigs()
{
  std::vector<BlockConfigPtr> allBlockConfigs;
  
  BlockWorldFilter blockFilter;
  blockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  std::vector<const ObservableObject*> allBlocks;
  _robot.GetBlockWorld().FindMatchingObjects(blockFilter, allBlocks);
  
  // Iterate through all configuration types and blocks
  for(ConfigurationType type = (ConfigurationType)0; type != ConfigurationType::Count; ++type){
    // preserve the current list for reference and hold shared_pointer instances
    // anything not inserted into the cache map through newCache will be deleted when it falls out of scope
    const std::vector<BlockConfigPtr> currentCache = _configToCacheMap.find(type)->second;
    std::vector<BlockConfigPtr>& newCache = _configToCacheMap.find(type)->second;
    newCache.clear();
    
    std::set<ObjectID> blocksAlreadyChecked;
    for(const auto& block: allBlocks){
      // ensure we don't re-build configurations for blocks we've already tracked
      if(blocksAlreadyChecked.find(block->GetID()) == blocksAlreadyChecked.end()){
        std::vector<BlockConfigPtr> currentTypeConfigs = BlockConfiguration::BuildAllConfigsWithObjectForType(_robot, block, type, currentCache);
        for(const auto& entry: currentTypeConfigs){
          // update the blocks we've already tracked for this configuration type so we don't
          // build/add configurations twice
          for(const auto& blockInConfig: entry->GetAllBlockIDsOrdered()){
            blocksAlreadyChecked.insert(blockInConfig);
          }
             
          newCache.push_back(entry);
        }
      }
    }
  }// end for ConfigurationType
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::PrunePyramidBases()
{
  auto& pyramidBaseCache = _configToCacheMap.find(ConfigurationType::PyramidBase)->second;
  if(pyramidBaseCache.size() > 0){
    const auto& fullPyramidCache = _configToCacheMap.find(ConfigurationType::Pyramid)->second;
    if(fullPyramidCache.size() > 0){
      
      // There are pyramids and bases - therefore we have to check all pairs to see if there is a pyramid base duplicating a pyramid
      for(auto pyramidBaseIter = pyramidBaseCache.begin(); pyramidBaseIter != pyramidBaseCache.end(); ){
        bool baseErased = false;
        for(const auto& configPtr: fullPyramidCache){
          auto pyramidPtr = BlockConfiguration::AsPyramidPtr(configPtr);
          auto baseCachePtr = BlockConfiguration::AsPyramidBasePtr(*pyramidBaseIter);
          if(*baseCachePtr == pyramidPtr->GetPyramidBase()){
            pyramidBaseIter = pyramidBaseCache.erase(pyramidBaseIter);
            baseErased = true;
            break;
          }
        }
        // If the pyramidBaseIter was not erased, advance to next value
        if(!baseErased){
          ++pyramidBaseIter;
        }
      }// end for pyramidBaseIter
      
    }// end if(fullPyramidCache.size() > 0
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::UpdateLastConfigCheckBlockPoses()
{
  BlockWorldFilter blockFilter;
  blockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  std::vector<const ObservableObject*> allBlocks;
  _robot.GetBlockWorld().FindMatchingObjects(blockFilter, allBlocks);
  
  for(const auto& block: allBlocks){
    if(block != nullptr) {
      const ObjectID& objectID = block->GetID();
      auto lastPoseMapIter = _objectIDToLastPoseConfigurationUpdateMap.find(objectID);
      
      if(lastPoseMapIter != _objectIDToLastPoseConfigurationUpdateMap.end()){
        if(block->IsPoseStateUnknown()){
          _objectIDToLastPoseConfigurationUpdateMap.erase(objectID);
        }else{
          lastPoseMapIter->second = block->GetPose();
        }
      }else if(!block->IsPoseStateUnknown()){
        _objectIDToLastPoseConfigurationUpdateMap.insert(std::make_pair(objectID, block->GetPose()));
      }
      
    }
  }
}
  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
