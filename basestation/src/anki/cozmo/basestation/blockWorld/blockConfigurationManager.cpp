/**
 * File: blockConfigurationManager.cpp
 *
 * Author: Kevin M. Karol
 * Created: 10/5/2016
 *
 * Description: Tracks and caches block configurations for behaviors to query
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"

#include "anki/cozmo/basestation/blockWorld/blockConfigTypeHelpers.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/vision/basestation/observableObject.h"

namespace Anki {
namespace Cozmo {
namespace BlockConfigurations {
  
namespace {
// minumum distance that a block has to move for us to update configurations
const float constexpr kMinBlockMoveUpdateThreshold_mm = 5.f;
const float constexpr kMinBlockRotationUpdateThreshold_rad = DEG_TO_RAD(30);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlockConfigurationManager::BlockConfigurationManager(Robot& robot)
: _robot(robot)
, _forceUpdate(false)
, _stackCache(StackConfigurationContainer())
, _pyramidBaseCache(PyramidBaseConfigurationContainer())
, _pyramidCache(PyramidConfigurationContainer())
{
  
  // Listen for behavior objective achieved messages for spark repetitions counter
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotDelocalized>();
  }
  
} // BlockConfigurationManager() Constructor

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlockConfigurationManager::CheckForPyramidBaseBelowObject(const ObservableObject* object, PyramidBaseWeakPtr& pyramidBase) const
{
  for(const auto& pyramidBasePtr: GetPyramidBaseCache().GetBases()){
    if(pyramidBasePtr->ObjectIsOnTopOfBase(_robot, object)){
      pyramidBase = pyramidBasePtr;
      return true;
    }
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::SetObjectPoseChanged(const ObjectID& objectID, const PoseState newPoseState)
{
  _objectsPoseChangedThisTick.insert(objectID);
  
  // if the object becomes unknown, forget its last known pose
  if ( !ObservableObject::IsValidPoseState(newPoseState) ) {
    _objectIDToLastPoseConfigurationUpdateMap.erase(objectID);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::Update()
{
  if(_objectsPoseChangedThisTick.empty() && !_forceUpdate){
    // nothing to update
    return;
  }
  
  // build a list of objects that have moved past their last configuration check threshold
  if(_forceUpdate || DidAnyObjectsMovePastThreshold()){
    UpdateAllBlockConfigs();
    _pyramidBaseCache.PruneFullPyramids(GetPyramidCache().GetPyramids());
    UpdateLastConfigCheckBlockPoses();
  }
  
  _forceUpdate = false;
  _objectsPoseChangedThisTick.clear();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BlockConfigurationManager::DidAnyObjectsMovePastThreshold()
{
  bool anyObjectMovedPastThreshold = false;
  
  auto IsObjectPartOfAnyConfiguration = [this](const ObjectID& id){
    for(ConfigurationType type =(ConfigurationType)0; type != ConfigurationType::Count; ++type){
      if(IsObjectPartOfConfigurationType(type, id)){
        return true;
      }
    }
    return false;
  };
  
  // check to see if any block has moved past our update configuration threshold
  for(const auto& objectID: _objectsPoseChangedThisTick){
    const ObservableObject* blockMoved = _robot.GetBlockWorld().GetLocatedObjectByID(objectID);
    if(blockMoved == nullptr){
      // The block's pose was known at some point and has changed to unknown -
      // It may have been part of a configuration - if it was we need to rebuild the configurations
      if(IsObjectPartOfAnyConfiguration(objectID)){
        anyObjectMovedPastThreshold = true;
      }
      continue;
    }
    
    auto lastPoseMapIter = _objectIDToLastPoseConfigurationUpdateMap.find(objectID);
    if(lastPoseMapIter != _objectIDToLastPoseConfigurationUpdateMap.end()){
      // the blocks pose is known, so check if it moved past our threshold
      const auto& newBlockPose = blockMoved->GetPose();
      const bool isBlockWithinTolerance = lastPoseMapIter->second.IsSameAs(newBlockPose, kMinBlockMoveUpdateThreshold_mm, kMinBlockRotationUpdateThreshold_rad);
      if(!isBlockWithinTolerance){
        anyObjectMovedPastThreshold = true;
        break;
      }
    }else{
      // the object is not in our pose map, so check to see if it is part of any configurations
      anyObjectMovedPastThreshold = true;
    }
  } // end for _objectsPoseChangedThisTick
  
  return anyObjectMovedPastThreshold;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::UpdateAllBlockConfigs()
{
  // Iterate through all configuration types and blocks
  for(ConfigurationType type = (ConfigurationType)0; type != ConfigurationType::Count; ++type){
    auto& cache = GetCacheByType(type);
    cache.Update(_robot);
  }// end for ConfigurationType
}
  
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BlockConfigurationContainer& BlockConfigurationManager::GetCacheByType(ConfigurationType type)
{
  switch (type) {
    case ConfigurationType::StackOfCubes:
    {
      return _stackCache;
    }
    case ConfigurationType::PyramidBase:
    {
      return _pyramidBaseCache;
    }
    case ConfigurationType::Pyramid:
    {
      return _pyramidCache;
    }
    case ConfigurationType::Count:
    {
      DEV_ASSERT_MSG(false, "BlockConfigurationManager.GetCacheByType.InvalidType",
                     "Configuration requested for invalid type");
      //return value set for compilation;
      return _stackCache;
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const BlockConfigurationContainer& BlockConfigurationManager::GetCacheByType(ConfigurationType type) const
{
  switch (type) {
    case ConfigurationType::StackOfCubes:
    {
      return _stackCache;
    }
    case ConfigurationType::PyramidBase:
    {
      return _pyramidBaseCache;
    }
    case ConfigurationType::Pyramid:
    {
      return _pyramidCache;
    }
    case ConfigurationType::Count:
    {
      DEV_ASSERT_MSG(false, "BlockConfigurationManager.GetCacheByType.InvalidType",
                     "Configuration requested for invalid type");
      //return value set for compilation;
      return _stackCache;
    }
  }}
  
bool BlockConfigurationManager::IsObjectPartOfConfigurationType(ConfigurationType type, const ObjectID& objectID) const
{
  auto& cache = GetCacheByType(type);
  if(cache.AnyConfigContainsObject(objectID)){
    return true;
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BlockConfigurationManager::UpdateLastConfigCheckBlockPoses()
{
  BlockWorldFilter blockFilter;
  blockFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
  std::vector<const ObservableObject*> allBlocks;
  _robot.GetBlockWorld().FindLocatedMatchingObjects(blockFilter, allBlocks);
  
  for(const auto& block: allBlocks){
    DEV_ASSERT(block != nullptr, "BlockConfigurationManager.UpdateLastConfigCheckBlockPoses.NullBlock");
    const ObjectID& objectID = block->GetID();
    auto lastPoseMapIter = _objectIDToLastPoseConfigurationUpdateMap.find(objectID);
    if(lastPoseMapIter != _objectIDToLastPoseConfigurationUpdateMap.end()){
      lastPoseMapIter->second = block->GetPose().GetWithRespectToOrigin();
    }else{
      _objectIDToLastPoseConfigurationUpdateMap.insert(std::make_pair(objectID, block->GetPose().GetWithRespectToOrigin()));
    }
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BlockConfigurationManager::HandleMessage(const ExternalInterface::RobotDelocalized& msg)
{
  FlagForRebuild();
}
  

  
} // namesapce BlockConfiguration
} // namespace Cozmo
} // namespace Anki
