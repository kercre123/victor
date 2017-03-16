/**
 * File: blockConfigurationContainer.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Abstract class for caches of configurations that can be maintained
 * and updated by the BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/blockConfigurationContainer.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/vision/basestation/observableObject.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlockConfigurationContainer::BlockConfigurationContainer()
{
  
} // BlockConfigurationContainer() Constructor

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationContainer::Update(const Robot& robot)
{
  BlockWorldFilter blockFilter;
  blockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  std::vector<const ObservableObject*> allBlocks;
  robot.GetBlockWorld().FindLocatedMatchingObjects(blockFilter, allBlocks);
  
  // save the cache as weak pointers and then clear it - any weak pointers
  // not added back in within the loop will automatically be deleted
  SetCurrentCacheAsBackup();
  ClearCache();
  
  std::set<ObjectID> blocksAlreadyChecked;
  for(const auto& block: allBlocks){
    // ensure we don't re-build configurations for blocks we've already tracked
    if(blocksAlreadyChecked.find(block->GetID()) == blocksAlreadyChecked.end()){
      auto allConfigsAdded = AddAllConfigsWithObjectToCache(robot, block);
      for(const auto& config: allConfigsAdded){
        for(const auto& blockInConfig: config->GetAllBlockIDsOrdered()){
          blocksAlreadyChecked.insert(blockInConfig);
        }
      }
    }
  }
  
  // delete any shared pointers not moved over
  DeleteBackup();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
T BlockConfigurationContainer::AllConfigsWithID(const ObjectID& objectID, const T& configVector)
{
  T outputVector;
  
  for(const auto& config: configVector){
    if(config->ContainsBlock(objectID)){
      outputVector.push_back(config);
    }
   }
  
  return outputVector;
}

} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
