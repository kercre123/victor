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

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
