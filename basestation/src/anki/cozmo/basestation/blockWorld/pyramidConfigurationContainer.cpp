/**
 * File: pyramidConfigurationContainer.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Caches pyramids for BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/pyramidConfigurationContainer.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"


namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PyramidConfigurationContainer::PyramidConfigurationContainer()
{
  
} // PyramidConfigurationContainer() Constructor

 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<PyramidWeakPtr> PyramidConfigurationContainer::GetWeakPyramids() const
{
  std::vector<PyramidWeakPtr> weakVec;
  for(const auto& pyramid: _pyramidCache){
    weakVec.push_back(pyramid);
  }
  return weakVec;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConfigPtrVec PyramidConfigurationContainer::AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object)
{
  ConfigPtrVec newConfigurations;

  
  std::vector<const Pyramid *> pyramids = Pyramid::BuildAllPyramidsForBlock(robot, object);
  for(auto& pyramid: pyramids){
    auto bestPtr = CastNewEntryToBestSharedPointer(_backupCache, pyramid);
    _pyramidCache.push_back(bestPtr);
    newConfigurations.push_back(CAST_TO_BLOCK_CONFIG_SHARED(bestPtr));
  }
  
  return newConfigurations;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidConfigurationContainer::AnyConfigContainsObject(const ObjectID& objectID) const
{
  for(const auto& entry: _pyramidCache){
    if(entry->ContainsBlock(objectID)){
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PyramidConfigurationContainer::ClearCache()
{
  _pyramidCache.clear();
}

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
