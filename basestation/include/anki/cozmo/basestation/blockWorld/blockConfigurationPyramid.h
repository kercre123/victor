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
#include "anki/common/basestation/math/pose.h"


namespace Anki{
namespace Cozmo{
  
// Forward declerations
class ObservableObject;
class Robot;
  
namespace BlockConfigurations{

class PyramidConfigurationContainer;
class PyramidBaseConfigurationContainer;
  
// For checking the validity of GetBaseBlockOffset's return value
// A base block and static block should never have an offset of 0 from each other
  
class PyramidBase: public BlockConfiguration{
  protected:
    // The BlockConfigurationManager builds all configurations
    // base vs static is decided by pyramid base so that:
    // baseBlockID < staticBlockID
    PyramidBase(const ObjectID& baseBlock1, const ObjectID& baseBlock2);
  
  public:
    // Allows pyramid to use GetAllBlockIDsOrdered() function
    friend class Pyramid;
    friend class BlockConfigurationManager;
  
    bool operator==(const PyramidBase& other) const;
    bool operator!=(const PyramidBase& other) const{ return !(*this == other);}
  
    // Accessors
    const ObjectID& GetStaticBlockID() const { return _staticBlockID;}
    const ObjectID& GetBaseBlockID() const { return _baseBlockID;}

    // the xAxis and isPositive bools are set to indicate x/y axis of the base
    // and positive/negative orientation of the baseBlock relative to the static block
    // returns true if the indicator bools were set, false otherwise
    static bool BlocksFormPyramidBase(const Robot& robot,
                                      const ObservableObject* const staticBlock,
                                      const ObservableObject* const baseBlock);
  
    // determine and set the two cube top corners closest to the other cube in the base
    // ruterns true if corners are set by function, false if not possible
    static bool GetBaseInteriorCorners(const Robot& robot,
                                       const ObservableObject* const targetCube,
                                       const ObservableObject* const otherCube,
                                       Pose3d& corner1,
                                       Pose3d& corner2);
  
    // determine the midpoint along the top most interior edge of the target cube
    static bool GetBaseInteriorMidpoint(const Robot& robot,
                                       const ObservableObject* const targetCube,
                                       const ObservableObject* const otherCube,
                                       Pose3d& midPoint);
  
  protected:
    // Checks the object's position to see if it is on top of the base
    const bool ObjectIsOnTopOfBase(const Robot& robot,
                                   const ObservableObject* const object) const;
  
    // clear the base IDs
    virtual void ClearBase();
  
    // Returns static and base block in ascending order so that equivalent pyramids
    // are always returned the same way
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const override;
  
  private:
    ObjectID _staticBlockID;
    ObjectID _baseBlockID;
};
  
class Pyramid: public BlockConfiguration{
  protected:
    friend PyramidConfigurationContainer;
    friend PyramidBaseConfigurationContainer;
    Pyramid(const PyramidBase& base, const ObjectID& topBlockID);
    Pyramid(const ObjectID& baseBlock1, const ObjectID& baseBlock2,
                                           const ObjectID& topBlockID);
 
  public:
    bool operator==(const Pyramid& other) const;
    bool operator!=(const Pyramid& other) const{ return !(*this == other);}
  
    // Accessors
    const PyramidBase& GetPyramidBase() const { return _base;}
    const ObjectID& GetTopBlockID() const { return _topBlockID;}
  
  protected:
    // Checks the object's position in the world against all other blocks
    // on the ground to determine if they form a pyramid base
    static std::vector<const PyramidBase*> BuildAllPyramidBasesForBlock(
                                                  const Robot& robot,
                                                  const ObservableObject* object);
  
    // Assumes that the cache of PyramidBases has been updated,
    // checks for top blocks on top of them
    static std::vector<const Pyramid*> BuildAllPyramidsForBlock(
                                                  const Robot& robot,
                                                  const ObservableObject* object);

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
