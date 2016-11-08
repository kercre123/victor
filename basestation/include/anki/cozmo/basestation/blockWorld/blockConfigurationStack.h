/**
 * File: BlockConfigurationStack.h
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

#ifndef Anki_Cozmo_BlockConfigurationStack_H
#define Anki_Cozmo_BlockConfigurationStack_H

#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki{
namespace Cozmo{
namespace BlockConfigurations{
class StackConfigurationContainer;
  
class StackOfCubes: public BlockConfiguration{
  public:
    friend StackConfigurationContainer;
  
    StackOfCubes();

    bool operator==(const StackOfCubes& other) const;
    bool operator!=(const StackOfCubes& other) const{ return !(*this == other);}

    // Accessors
    const ObjectID& GetBottomBlockID() const { return _bottomBlockID;}
    const ObjectID& GetMiddleBlockID() const { return _middleBlockID;}
    const ObjectID& GetTopBlockID() const { return _topBlockID;}
  
    uint8_t GetStackHeight() const;
  
    // A stack of 2 that becomes a stack of 3 is not "equal" but may
    // be identified as such using this function
    bool IsASubstack(const StackOfCubes& potentialSuperStack) const;

  protected:
    // Only the BlockConfigurationManager should create stacks
    StackOfCubes(const ObjectID& bottomBlockID, const ObjectID& middleBlockID);
    StackOfCubes(const ObjectID& bottomBlockID, const ObjectID& middleBlockID, const ObjectID& topBlockID);
  
    // Returns the largest stack the object is a part of
    // this can only be one stack since two blocks centroids should not be able to be beneath or on top
    // of a block at the same time
    static const StackOfCubes* BuildTallestStackForObject(const Robot& robot, const ObservableObject* object);
  
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const override;
  
    // Utility functions
    virtual void ClearStack();
  
  private:
    ObjectID _bottomBlockID;
    ObjectID _middleBlockID;
    ObjectID _topBlockID;
};
  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfigurationStack_H
