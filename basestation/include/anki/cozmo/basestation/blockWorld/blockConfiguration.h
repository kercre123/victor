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

#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki{
// forward declaration
class ObjectID;
namespace Cozmo{
namespace BlockConfigurations{
  
// define block configuration types
// also update stringMapping in blockConfigTypeHelpers
enum class ConfigurationType{
  StackOfCubes = 0,
  PyramidBase,
  Pyramid,
  
  // add new configurations before this
  Count
};
  
class BlockConfiguration{
  public:
    BlockConfiguration(ConfigurationType type){ _type = type;}
    virtual ~BlockConfiguration() {};
    // Determines configuration equality based on type and comparing ordered blocks from the
    // two configurations
    bool operator==(const BlockConfiguration& other) const;

    virtual ConfigurationType GetType() const{ return _type;}
    const bool ContainsBlock(const ObjectID& objectID) const;
  
    // This function should return an ordered vector of the blocks in a configuration
    // this is used to compare BlockConfigurations to ensure that equivalent configurations always
    // return blocks in the same order
    virtual std::vector<ObjectID> GetAllBlockIDsOrdered() const = 0;
  
  protected:
    ConfigurationType _type;
};

} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfiguration_H
