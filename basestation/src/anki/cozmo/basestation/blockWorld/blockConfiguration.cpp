/**
 * File: blockConfiguration.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Defines the BlockConfiguration interface that BlockConfigurationManager
 *  relies on to maintain its list of current block configurations
 *  Any configurations which are managed by BlockConfigurationManager should inherit from this class
 *  and override the marked functions (though they are not abstract so that this class can be instantiated)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "util/helpers/templateHelpers.h"


namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {
  
  
// Determines configuration equality based on type and comparing ordered blocks from the
// two configurations
bool BlockConfiguration::operator==(const BlockConfiguration& other) const {
  if(GetType() != other.GetType()){
    return false;
  }
  auto myBlocks = GetAllBlockIDsOrdered();
  auto otherBlocks = other.GetAllBlockIDsOrdered();
  if(myBlocks.size() != otherBlocks.size()){
    return false;
  }
  
  bool allMatch = true;
  for(auto index = 0; index < myBlocks.size(); index++){
    if(myBlocks[index] != otherBlocks[index]){
      allMatch = false;
      break;
    }
  }
  
  return allMatch;
};

const bool BlockConfiguration::ContainsBlock(const ObjectID& objectID) const
{
  auto blockIDs = GetAllBlockIDsOrdered();
  for(const auto& blockID: blockIDs){
    if(objectID == blockID){
      return true;
    }
  }
  
  return false;
}


BlockConfiguration::BlockConfigPtr BlockConfiguration::GetBestSharedPointer(const std::vector<BlockConfigPtr>& currentList, const BlockConfiguration* newEntry)
{
  for(const auto& entry: currentList){
    if((*entry) == (*newEntry)){
      Util::SafeDelete(newEntry);
      return entry;
    }
  }
  
  BlockConfigPtr castPtr =  BlockConfigPtr(newEntry);
  return castPtr;
}
  
  
std::vector<BlockConfiguration::BlockConfigPtr> BlockConfiguration::BuildAllConfigsWithObjectForType(
                                                    const Robot& robot, const ObservableObject* object,
                                                    ConfigurationType type, const std::vector<BlockConfigPtr>& currentConfigurations)
{
  std::vector<BlockConfigPtr> allConfigurations;
  

  switch (type) {
    case ConfigurationType::StackOfCubes:
    {
      auto stackPtr = StackOfCubes::BuildTallestStackForObject(robot, object);
      if(stackPtr != nullptr) {
        allConfigurations.push_back(GetBestSharedPointer(currentConfigurations, stackPtr));
      }
      break;
    }
    case ConfigurationType::Pyramid:
    {
      std::vector<const Pyramid *> pyramids = Pyramid::BuildAllPyramidsForBlock(robot, object);
      for(const auto& pyramid: pyramids){
        allConfigurations.push_back(GetBestSharedPointer(currentConfigurations, pyramid));
      }
      break;
    }
    case ConfigurationType::PyramidBase:
    {
      std::vector<const PyramidBase *> pyramidBases = Pyramid::BuildAllPyramidBasesForBlock(robot, object);
      for(const auto& pyramidBase: pyramidBases){
        allConfigurations.push_back(GetBestSharedPointer(currentConfigurations, pyramidBase));
      }
      break;
    }
    case ConfigurationType::Count:
    {
      PRINT_NAMED_WARNING("BlockConfiguration.BuildAllConfigsWithObjectForType.Count",
                          "Requested to build configurations of invalid type");
      break;
    }
  }
  
  return allConfigurations;
}
 
  
//////////////
///// Pointer conversion
//////////////
  
StackWeakPtr BlockConfiguration::AsStackWeakPtr(const BlockConfigWeakPtr& configPtr)
{
  BlockConfigPtr sharedPtr = configPtr.lock();
  if(sharedPtr){
    ASSERT_NAMED_EVENT(sharedPtr->GetType() == ConfigurationType::StackOfCubes,
                       "BlockConfiguration.AsStackWeakPtr.IncorrectType",
                       "Requested a StackWeakPtr for a configuration that was not a stack");
  }
  
  return std::static_pointer_cast<const StackOfCubes>(sharedPtr);
}

  
PyramidWeakPtr  BlockConfiguration::AsPyramidWeakPtr(const BlockConfigWeakPtr& configPtr)
{
  BlockConfigPtr sharedPtr = configPtr.lock();
  if(sharedPtr){
    ASSERT_NAMED_EVENT(sharedPtr->GetType() == ConfigurationType::Pyramid,
                       "BlockConfiguration.AsPyramidWeakPtr.IncorrectType",
                       "Requested a PyramidWeakPtr for a configuration that was not a pyramid");
  }

  return std::static_pointer_cast<const Pyramid>(configPtr.lock());
}
  
  
PyramidBaseWeakPtr BlockConfiguration::AsPyramidBaseWeakPtr(const BlockConfigWeakPtr& configPtr)
{
  BlockConfigPtr sharedPtr = configPtr.lock();
  if(sharedPtr){
    ASSERT_NAMED_EVENT(sharedPtr->GetType() == ConfigurationType::PyramidBase,
                       "BlockConfiguration.AsPyramidBaseWeakPtr.IncorrectType",
                       "Requested a PyramidBaseWeakPtr for a configuration that was not a pyramidBase");
  }
  
  return std::static_pointer_cast<const PyramidBase>(configPtr.lock());
}
  
  
std::shared_ptr<const StackOfCubes> BlockConfiguration::AsStackPtr(const BlockConfigPtr& configPtr){
  if(configPtr){
    ASSERT_NAMED_EVENT(configPtr->GetType() == ConfigurationType::StackOfCubes,
                     "BlockConfiguration.AsStackPtr.IncorrectType",
                     "Requested a StackPtr for a configuration that was not a stack");
  }
  return std::static_pointer_cast<const StackOfCubes>(configPtr);
}
  
  
std::shared_ptr<const Pyramid> BlockConfiguration::AsPyramidPtr(const BlockConfigPtr& configPtr){
  if(configPtr){
    ASSERT_NAMED_EVENT(configPtr->GetType() == ConfigurationType::Pyramid,
                       "BlockConfiguration.AsPyramidPtr.IncorrectType",
                       "Requested a PyramidPtr for a configuration that was not a pyramid");
  }

  return std::static_pointer_cast<const Pyramid>(configPtr);
}
std::shared_ptr<const PyramidBase> BlockConfiguration::AsPyramidBasePtr(const BlockConfigPtr& configPtr){
  if(configPtr){
    ASSERT_NAMED_EVENT(configPtr->GetType() == ConfigurationType::PyramidBase,
                       "BlockConfiguration.AsPyramidBasePtr.IncorrectType",
                       "Requested a PyramidBasePtr for a configuration that was not a pyramidBase");
  }

  return std::static_pointer_cast<const PyramidBase>(configPtr);
}

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
