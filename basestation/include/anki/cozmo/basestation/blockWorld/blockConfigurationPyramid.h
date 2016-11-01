/**
 * File: blockConfigurationPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Defines the Pyramid and PyramidBase configurations
 *
 * A PyramidBase is any two blocks that are close enough to adjacent to each other
 * that a top block could be stacked on top of them by Cozmo
 * Currently this is defined as cubes that are within 1/2 the block size of each others centers
 * on at least one axis and are both on the ground
 *
 * A PyramidBase has what are termed a StaticBlock and a BaseBlock
 * these are interchangable, but the names are derived from how Cozmo builds a
 * base - bringing a base block over to a static block and placing the base adjacent to the static
 *
 * A Pyramid is a PyramidBase that already has a top block on top of it
 * A top block has to be offset from the static block in the same direction
 * as the base block
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_BlockConfigurationPyramid_H
#define Anki_Cozmo_BlockConfigurationPyramid_H

#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/point.h"

namespace Anki{
namespace Cozmo{
  
// Forward declerations
class ObservableObject;
class Robot;
  
namespace BlockConfigurations{
  
// For checking the validity of GetBaseBlockOffset's return value
// A base block and static block should never have an offset of 0 from each other
  
class PyramidBase: public BlockConfiguration{
  public:
    // Allows pyramid to use GetAllBlockIDsOrdered() function
    friend class Pyramid;
    friend class BlockConfigurationManager;
  
    bool operator==(const PyramidBase& other) const;
    bool operator!=(const PyramidBase& other) const{ return !(*this == other);}
  
    // Accessors
    const ObjectID& GetStaticBlockID() const { return _staticBlockID;}
    const ObjectID& GetBaseBlockID() const { return _baseBlockID;}

    // the xAxis and isPositive bools are set to indicate x/y axis of the base and positive/negative orientation of the baseBlock relative to the static block
    // returns true if the indicator bools were set, false otherwise
    const bool GetBaseBlockOffset(const Robot& robot, bool& alongXAxis, bool& isPositive) const;
  
    // returns the x/y offset of the base block - returns 0/0 if offset is not valid
    const Point2f GetBaseBlockOffsetValues(const Robot& robot) const;

  
  protected:
    // Pyramid bases should only be created by the block configuration manager
    PyramidBase(const ObjectID& staticBlockID, const ObjectID& baseBlockID);
    // Checks the relative world positions of the blocks to see if they form a pyramid base
    static bool BlocksFormPyramidBase(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock);
    // Static accessor for BaseBlockOffset - returns true if the blocks form a pyramidBase
    // the xAxis and isPositive bools are set to indicate x/y axis of the base and positive/negative orientation of the baseBlock relative to the static block
    static const bool GetBaseBlockOffset(const Robot& robot, const ObservableObject* const staticBlock, const ObservableObject* const baseBlock, bool& alongXAxis, bool& isPositive);
  
    // Checks the object's position to see if it is on top of the base
    const bool ObjectIsOnTopOfBase(const Robot& robot, const ObservableObject* const object) const;
  
    // clear the base IDs
    virtual void ClearBase();
  
    // Returns static and base block in ascending order so that equivalent pyramids are always returned the same way
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const override;
  
  private:
    ObjectID _staticBlockID;
    ObjectID _baseBlockID;
};
  
class Pyramid: public BlockConfiguration{
  public:
    friend BlockConfigurationManager;
    friend BlockConfiguration;
 
    bool operator==(const Pyramid& other) const;
    bool operator!=(const Pyramid& other) const{ return !(*this == other);}
  
    // Accessors
    const PyramidBase& GetPyramidBase() const { return _base;}
    const ObjectID& GetTopBlockID() const { return _topBlockID;}
  
  protected:
    Pyramid(const PyramidBase& base, const ObjectID& topBlockID);
    Pyramid(const ObjectID& staticBlockID, const ObjectID& baseBlockID, const ObjectID& topBlockID);
  
    // Checks the object's position in the world against all other blocks on the ground
    // to determine if they form a pyramid base
    static std::vector<const PyramidBase*> BuildAllPyramidBasesForBlock(const Robot& robot, const ObservableObject* object);
    // Assumes that the cache of PyramidBases has been updated, and checks for top blocks on top of them
    static std::vector<const Pyramid*> BuildAllPyramidsForBlock(const Robot& robot, const ObservableObject* object);

    // clear the pyramid IDs
    virtual void ClearPyramid();
  
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const override;

  private:
    PyramidBase _base;
    ObjectID _topBlockID;
};


  
} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfigurationPyramid_H
