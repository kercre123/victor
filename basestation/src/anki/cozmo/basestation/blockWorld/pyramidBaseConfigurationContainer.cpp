/**
 * File: pyramidBaseConfigurationContainer.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Caches pyramid bases for BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/pyramidBaseConfigurationContainer.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PyramidBaseConfigurationContainer::PyramidBaseConfigurationContainer()
{
  
} // PyramidBaseConfigurationContainer() Constructor
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<PyramidBaseWeakPtr> PyramidBaseConfigurationContainer::GetWeakBases() const
{
  std::vector<PyramidBaseWeakPtr> weakVec;
  for(const auto& base: _pyramidBaseCache){
    weakVec.push_back(base);
  }
  return weakVec;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConfigPtrVec PyramidBaseConfigurationContainer::AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object)
{
  ConfigPtrVec newConfigurations;
  
  std::vector<const PyramidBase *> pyramidBases = Pyramid::BuildAllPyramidBasesForBlock(robot, object);
  for(auto& pyramidBase: pyramidBases){
    auto bestPtr = CastNewEntryToBestSharedPointer(_backupCache, pyramidBase);
    _pyramidBaseCache.push_back(bestPtr);
    newConfigurations.push_back(CAST_TO_BLOCK_CONFIG_SHARED(bestPtr));
  }
  
  return newConfigurations;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PyramidBaseConfigurationContainer::AnyConfigContainsObject(const ObjectID& objectID) const
{
  for(const auto& entry: _pyramidBaseCache){
    if(entry->ContainsBlock(objectID)){
      return true;
    }
  }
  
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PyramidBaseConfigurationContainer::ClearCache()
{
  _pyramidBaseCache.clear();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PyramidBaseConfigurationContainer::PruneFullPyramids(const std::vector<PyramidPtr>& fullPyramids)
{
  if(fullPyramids.size() > 0){
    if(fullPyramids.size() > 0){
      
      // There are pyramids and bases - therefore we have to check all pairs to see if there is a pyramid base duplicating a pyramid
      for(auto pyramidBaseIter = _pyramidBaseCache.begin(); pyramidBaseIter != _pyramidBaseCache.end(); ){
        bool baseErased = false;
        for(const auto& pyramidPtr: fullPyramids){
         if((*(*pyramidBaseIter)) == pyramidPtr->GetPyramidBase()){
           pyramidBaseIter = _pyramidBaseCache.erase(pyramidBaseIter);
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

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
