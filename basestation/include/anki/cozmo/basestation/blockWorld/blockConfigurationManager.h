/**
 * File: blockConfigurationManager.h
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Tracks and caches block configurations for behaviors to query
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef Anki_Cozmo_BlockConfigurationManager_H
#define Anki_Cozmo_BlockConfigurationManager_H

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"


namespace Anki{
namespace Cozmo{
  
// Forward declarations
class Robot;
class ObservableObject;
  
namespace BlockConfigurations{

  
class BlockConfigurationManager
{
public:
  friend BlockConfiguration;
  BlockConfigurationManager(const Robot& robot);

  // Accessors
  std::vector<BlockConfigWeakPtr> GetConfigurationsForType(ConfigurationType type) const;
  std::vector<BlockConfigWeakPtr> GetConfigurationsContainingObjectForType(ConfigurationType type, const ObjectID& objectID) const;
  
  // Convenience methods
  const bool IsObjectPartOfConfigurationType(ConfigurationType type, const ObjectID& objectID) const { return !GetConfigurationsContainingObjectForType(type, objectID).empty();}
  
  // Return a pointer to the current tallest stack
  const StackWeakPtr GetTallestStack() const;
  // Pass in a list of bottom blocks to ignore if you are looking to locate a specific stack
  const StackWeakPtr GetTallestStack(const std::vector<ObjectID>& baseBlocksToIgnore) const;
  
  // Checks to see if there are any extant pyramid bases which the object is a top block for
  // returns true if such a base exists and loads it in to pyramidBase, otherwise returns false
  bool CheckForPyramidBaseBelowObject(const ObservableObject* object, PyramidBaseWeakPtr& pyramidBase) const;

  // Notify the BlockConfigurationManager that an object's pose has changed and that it should
  // check the object on its next update to see if configurations have been created/destroyed as a result
  void SetObjectPoseChanged(const ObjectID& objectID){ _objectsPoseChangedThisTick.insert(objectID);}
  
  // Update should be called after all block world changes have been made
  // if it has been notified that object poses have changed, it checks whether these
  // changes are large enough to justify re-building the configurations
  // and then updates the internal block configuration list and object
  // caches accordingly
  void Update();
  
private:
  const Robot& _robot;
  // track the last pose where we updated configurations for a block so that we don't re-build on tiny block moves
  std::map<ObjectID, Pose3d> _objectIDToLastPoseConfigurationUpdateMap;
  // track all of the blocks that have moved so that configurations can be updated
  // once all poses are updated
  std::set<ObjectID> _objectsPoseChangedThisTick;
  
  using BlockConfigPtr = std::shared_ptr<const BlockConfiguration>;
  
  // Caches specific configuration types to make access faster
  std::map<ConfigurationType, std::vector<BlockConfigPtr>> _configToCacheMap;
  
  //////
  //Update functions
  //////
  // Returns true if any objects moved past threshold
  bool DidAnyObjectsMovePastThreshold();
  
  // Builds all configurations for all blocks in block world and stores them in _allBlockConfigs
  // any weakPtrs to configurations that still exist are maintained
  void UpdateAllBlockConfigs();
  
  // Since pyramid bases are made first it's possible that they're part of a full pyramid
  // This function removes any pyramid bases that are part of a full pyramid
  void PrunePyramidBases();
  // updates the _objectIDToLastPose map's last poses checked
  void UpdateLastConfigCheckBlockPoses();

  
  
}; // class BlockConfigurationManager

} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfigurationManager_H
