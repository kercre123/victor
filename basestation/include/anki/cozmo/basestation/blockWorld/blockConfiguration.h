/**
 * File: blockConfiguration.h
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 *  Description: Defines the BlockConfiguration interface that BlockConfigurationManager
 *  relies on to maintain its list of current block configurations
 *  Any configurations which are managed by BlockConfigurationManager should inherit from this class.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_BlockConfiguration_H
#define Anki_Cozmo_BlockConfiguration_H

#include "anki/cozmo/basestation/blockWorld/blockConfigTypeHelpers.h"
#include "anki/common/basestation/objectIDs.h"
#include "util/enums/enumOperators.h"

#include <vector>

namespace Anki{
namespace Cozmo{
  
// Forward declerations
class ObservableObject;
class Robot;
class BlockConfigurationManager;
  
namespace BlockConfigurations{

// Forward declarations
class StackOfCubes;
class Pyramid;
class PyramidBase;
class BlockConfiguration;
  
// define block configuration types
// also update stringMapping in blockConfigTypeHelpers
enum class ConfigurationType{
  StackOfCubes = 0,
  PyramidBase,
  Pyramid,
  
  // add new configurations before this
  Count
};

// convenience names
using BlockConfigWeakPtr = std::weak_ptr<const BlockConfiguration>;
using StackWeakPtr = std::weak_ptr<const StackOfCubes>;
using PyramidWeakPtr = std::weak_ptr<const Pyramid>;
using PyramidBaseWeakPtr = std::weak_ptr<const PyramidBase>;

  
class BlockConfiguration{
  public:
    friend class BlockConfigurationManager;
    friend class Pyramid;
    BlockConfiguration(ConfigurationType type){ _type = type;}
    virtual ~BlockConfiguration() {};
    // Determines configuration equality based on type and comparing ordered blocks from the
    // two configurations
    bool operator==(const BlockConfiguration& other) const;

    virtual ConfigurationType GetType() const{ return _type;}
    const bool ContainsBlock(const ObjectID& objectID) const;
  
    static StackWeakPtr AsStackWeakPtr(const BlockConfigWeakPtr& configPtr);
    static PyramidWeakPtr  AsPyramidWeakPtr(const BlockConfigWeakPtr& configPtr);
    static PyramidBaseWeakPtr AsPyramidBaseWeakPtr(const BlockConfigWeakPtr& configPtr);
  
  protected:
    ConfigurationType _type;

    // Provides a generic way to access BuildConfiguration functions for the different block configuration types
    // After configurations have been built they are checked against currentConfigurations for equality - if they are equal
    // the existing shared pointer is added to the vector instead of a new instance
    // returns a new vector containing pointers to all current configs of the requested type
    using BlockConfigPtr = std::shared_ptr<const BlockConfiguration>;
    static std::vector<BlockConfigPtr> BuildAllConfigsWithObjectForType(const Robot& robot, const ObservableObject* object,
                                                                        ConfigurationType type, const std::vector<BlockConfigPtr>& currentConfigurations);
    
    // This function should return an ordered vector of the blocks in a configuration
    // this is used to compare BlockConfigurations to ensure that equivalent configurations always
    // return blocks in the same order
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const = 0;
  
  private:
    // ensures that existing shared pointers are returned if available
    // also handles object deletion
    static BlockConfigPtr GetBestSharedPointer(const std::vector<BlockConfigPtr>& currentList, const BlockConfiguration* newEntry);

    static std::shared_ptr<const StackOfCubes> AsStackPtr(const BlockConfigPtr& configPtr);
    static std::shared_ptr<const Pyramid>  AsPyramidPtr(const BlockConfigPtr& configPtr);
    static std::shared_ptr<const PyramidBase> AsPyramidBasePtr(const BlockConfigPtr& configPtr);
  
};

} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfiguration_H
