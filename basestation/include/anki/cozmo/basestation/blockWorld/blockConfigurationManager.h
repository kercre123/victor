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

#include "anki/cozmo/basestation/blockWorld/blockConfiguration.h"
#include "anki/cozmo/basestation/blockWorld/stackConfigurationContainer.h"
#include "anki/cozmo/basestation/blockWorld/pyramidBaseConfigurationContainer.h"
#include "anki/cozmo/basestation/blockWorld/pyramidConfigurationContainer.h"

#include "util/signals/simpleSignal_fwd.h"
#include <set>
#include <map>

namespace Anki{
// Forward declarations
class ObjectID;
class Pose3d;
namespace Cozmo{
class Robot;
class ObservableObject;
  
namespace BlockConfigurations{
  
class BlockConfigurationManager
{
public:
  friend BlockConfiguration;
  BlockConfigurationManager(Robot& robot);

  // Cache accessors
  const StackConfigurationContainer& GetStackCache() const { return _stackCache;}
  const PyramidBaseConfigurationContainer& GetPyramidBaseCache() const { return _pyramidBaseCache;}
  const PyramidConfigurationContainer& GetPyramidCache() const { return _pyramidCache;}
  
  // Convenience methods
  bool IsObjectPartOfConfigurationType(ConfigurationType type, const ObjectID& objectID) const;
  
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
  
  const BlockConfigurationContainer& GetCacheByType(ConfigurationType type) const;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  void FlagForRebuild(){ _forceUpdate = true;}

private:
  const Robot& _robot;
  std::vector<::Signal::SmartHandle> _signalHandles;
  bool _forceUpdate;
  
  // track the last pose where we updated configurations for a block so that we don't re-build on tiny block moves
  std::map<ObjectID, Pose3d> _objectIDToLastPoseConfigurationUpdateMap;
  // track all of the blocks that have moved so that configurations can be updated
  // once all poses are updated
  std::set<ObjectID> _objectsPoseChangedThisTick;

  // configuration caches
  StackConfigurationContainer _stackCache;
  PyramidBaseConfigurationContainer _pyramidBaseCache;
  PyramidConfigurationContainer _pyramidCache;
  
  BlockConfigurationContainer& GetCacheByType(ConfigurationType type);
  
  //////
  //Update functions
  //////
  // Returns true if any objects moved past threshold
  bool DidAnyObjectsMovePastThreshold();
  
  // Builds all configurations for all blocks in block world and stores them in _allBlockConfigs
  // any weakPtrs to configurations that still exist are maintained
  void UpdateAllBlockConfigs();
  
  // updates the _objectIDToLastPose map's last poses checked
  void UpdateLastConfigCheckBlockPoses();

}; // class BlockConfigurationManager

} // namespace BlockConfigurations
} // namespace Cozmo
} // namespace Anki


#endif // Anki_Cozmo_BlockConfigurationManager_H
