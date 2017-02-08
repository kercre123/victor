/**
 * File: BlockConfigurationStack.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Defines the StackBlock configuration
 *
 * A stack can consist of 2 or 3 blocks stacked on top of each other such that
 * their centroids are located on top of each other
 *
 * A stack allways has a top and bottom block - middle block is only set for stacks of 3
 *
 * Due to talerences a configuration may be marked as both a stack and a pyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationStack.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/observableObject.h"

#include "util/math/math.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {

namespace {
  
using StackIterator = std::vector<StackWeakPtr>::iterator;
using StackIteratorConst = std::vector<StackWeakPtr>::const_iterator;
const float kOnGroundTolerenceStackBlockOnly = 2*ON_GROUND_HEIGHT_TOL_MM;

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StackOfCubes::StackOfCubes()
: BlockConfiguration(ConfigurationType::StackOfCubes)
{
  ClearStack();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StackOfCubes::StackOfCubes(const ObjectID& bottomBlockID, const ObjectID& middleBlockID)
: BlockConfiguration(ConfigurationType::StackOfCubes)
{
  _bottomBlockID = bottomBlockID;
  _topBlockID = middleBlockID;
  _middleBlockID.SetToUnknown();
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StackOfCubes::StackOfCubes(const ObjectID& bottomBlockID, const ObjectID& middleBlockID, const ObjectID& topBlockID)
: BlockConfiguration(ConfigurationType::StackOfCubes)
{
  _bottomBlockID = bottomBlockID;
  _middleBlockID = middleBlockID;
  _topBlockID = topBlockID;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StackOfCubes::operator==(const StackOfCubes& other) const
{
  return GetTopBlockID() == other.GetTopBlockID() &&
         GetMiddleBlockID() == other.GetMiddleBlockID() &&
         GetBottomBlockID() == other.GetBottomBlockID();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint8_t StackOfCubes::GetStackHeight() const{
  if(_bottomBlockID.IsUnknown()){
    return 0;
  }else if(_topBlockID.IsUnknown()){
    return 1;
  }else if(_middleBlockID.IsUnknown()){
    return 2;
  }else{
    return 3;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StackOfCubes::IsASubstack(const StackOfCubes& potentialSuperStack) const
{
  return _middleBlockID.IsUnknown() &&
         _bottomBlockID == potentialSuperStack.GetBottomBlockID() &&
         _topBlockID == potentialSuperStack.GetTopBlockID();
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StackOfCubes::ClearStack()
{
  _bottomBlockID.SetToUnknown();
  _middleBlockID.SetToUnknown();
  _topBlockID.SetToUnknown();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<ObjectID> StackOfCubes::GetAllBlockIDsOrdered() const
{
  std::vector<ObjectID> allBlocks;
  allBlocks.push_back(_bottomBlockID);
  if(_middleBlockID.IsSet()){
    allBlocks.push_back(_middleBlockID);
  }
  
  allBlocks.push_back(_topBlockID);
  
  return allBlocks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StackOfCubes* StackOfCubes::BuildTallestStackForObject(const Robot& robot, const ObservableObject* object)
{
  // Get all blocks that are on the ground
  std::vector<const ObservableObject*> blocksOnGround;
  
  {
    BlockWorldFilter bottomBlockFilter;
    bottomBlockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    bottomBlockFilter.AddFilterFcn([](const ObservableObject* blockPtr)
                                   {
                                     if(!blockPtr->IsRestingAtHeight(0, kOnGroundTolerenceStackBlockOnly)){
                                       return false;
                                     }
                                     
                                     if(!blockPtr->IsRestingFlat()){
                                       return false;
                                     }
                                     
                                     return true;
                                   });
    
    robot.GetBlockWorld().FindLocatedMatchingObjects(bottomBlockFilter, blocksOnGround);
  }

  
  // Identify all blocks above and below the current block
  std::vector<const ObservableObject*> blocksOnTopOfObject;
  std::vector<const ObservableObject*> blocksBelowObject;
  {
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    
    // blocks above
    auto currentBlock = object;
    auto nextBlock = currentBlock;
    BOUNDED_WHILE(10,(nextBlock = robot.GetBlockWorld().FindLocatedObjectOnTopOf(*currentBlock,
                                                                                 BlockWorld::kOnCubeStackHeightTolerence,
                                                                                 blocksOnlyFilter))){
      blocksOnTopOfObject.push_back(nextBlock);
      currentBlock = nextBlock;
    }
    
    // blocks below
    currentBlock = object;
    nextBlock = currentBlock;
    BOUNDED_WHILE(10,(nextBlock = robot.GetBlockWorld().FindLocatedObjectUnderneath(*currentBlock,
                                                                                    BlockWorld::kOnCubeStackHeightTolerence,
                                                                                    blocksOnlyFilter))){
      blocksBelowObject.push_back(nextBlock);
      currentBlock = nextBlock;
    }
  }
  
  // build the vector of all stacks including the block
  const StackOfCubes* largestStackContainingBlock = nullptr;
  
  // check to see if the block is part of a stack
  if(blocksOnGround.empty() || (blocksOnTopOfObject.empty() && blocksBelowObject.empty())){
    return largestStackContainingBlock;
  }
  
  // check to make sure the stack starts on the ground
  const bool objectOnGround = std::find(blocksOnGround.begin(), blocksOnGround.end(), object) != blocksOnGround.end();
  const bool lastObjectBelowOnGround = !blocksBelowObject.empty() &&
                std::find(blocksOnGround.begin(), blocksOnGround.end(), blocksBelowObject.back()) != blocksOnGround.end();
  
  if(!objectOnGround && !lastObjectBelowOnGround){
    return largestStackContainingBlock;
  }
  
  // actually build the stack
  std::vector<const ObservableObject*> blocksBottomToTop;
  for(auto blockIter = blocksBelowObject.rbegin(); blockIter != blocksBelowObject.rend(); ++blockIter){
    blocksBottomToTop.push_back(*blockIter);
  }
  
  blocksBottomToTop.push_back(object);
  
  for(auto blockIter = blocksOnTopOfObject.begin(); blockIter != blocksOnTopOfObject.end(); ++blockIter){
    blocksBottomToTop.push_back(*blockIter);
  }
  
  
  if(blocksBottomToTop.size() > 2){
    largestStackContainingBlock = new StackOfCubes(blocksBottomToTop[0]->GetID(), blocksBottomToTop[1]->GetID(), blocksBottomToTop[2]->GetID());
  }else if(blocksBottomToTop.size() == 2){
    largestStackContainingBlock = new StackOfCubes(blocksBottomToTop[0]->GetID(), blocksBottomToTop[1]->GetID());
  }
  
  return largestStackContainingBlock;
}
  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki
