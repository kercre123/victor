/**
 * File: stackConfigurationContainer.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/2/2016
 *
 * Description: Caches stacks for BlockConfigurationManager
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/stackConfigurationContainer.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StackConfigurationContainer::StackConfigurationContainer()
{
  
} // StackConfigurationContainer() Constructor


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<StackWeakPtr> StackConfigurationContainer::GetWeakStacks() const
{
  std::vector<StackWeakPtr> weakVec;
  for(const auto& stack: _stackCache){
    weakVec.push_back(stack);
  }
  return weakVec;
}


ConfigPtrVec StackConfigurationContainer::AddAllConfigsWithObjectToCache(const Robot& robot, const ObservableObject* object)
{
  ConfigPtrVec newConfigurations;
  
  auto stackPtr = StackOfCubes::BuildTallestStackForObject(robot, object);
  if(stackPtr != nullptr) {
    auto bestPtr = CastNewEntryToBestSharedPointer(_backupCache, stackPtr);
    _stackCache.push_back(bestPtr);
    newConfigurations.push_back(CAST_TO_BLOCK_CONFIG_SHARED(bestPtr));
  }
  
  return newConfigurations;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StackConfigurationContainer::AnyConfigContainsObject(const ObjectID& objectID) const
{
  for(const auto& entry: _stackCache){
    if(entry->ContainsBlock(objectID)){
      return true;
    }
  }
  
  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackConfigurationContainer::ClearCache()
{
  _stackCache.clear();
}
  
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockConfigurations::StackWeakPtr StackConfigurationContainer::GetTallestStack() const
{
  std::vector<ObjectID> bottomBlocksToIgnore;
  return GetTallestStack(bottomBlocksToIgnore);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockConfigurations::StackWeakPtr StackConfigurationContainer::GetTallestStack(const std::vector<ObjectID>& baseBlocksToIgnore) const
{
  if(_stackCache.empty()){
    return std::make_shared<StackOfCubes>();
  }
  
  using StackIteratorConst = std::vector<StackPtr>::const_iterator;
  auto tallestStack = _stackCache.front();
  for(StackIteratorConst stackIt = _stackCache.begin(); stackIt != _stackCache.end(); ++stackIt){
    if((*stackIt)->GetStackHeight() > tallestStack->GetStackHeight()){
      tallestStack = (*stackIt);
    }
  }
  
  return tallestStack;
}

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
