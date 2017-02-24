/**
 * File: BuildPyramidBehaviorChooser.cpp
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Class for handling playing sparks.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/buildPyramidBehaviorChooser.h"

#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviors/behaviorRespondPossiblyRoll.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramid.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPyramidThankYou.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRespondPossiblyRoll.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationPyramid.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGameTag.h"


namespace Anki {
namespace Cozmo {

namespace{
using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
  
static const char* kLightsShouldMessageCubeOnSideKey = "shouldCubeMessageOnSide";

static const int kMinUprightBlocksForPyramid      = 3;
static const float kDelayAccountForPlacing_s      = 3.0;
static const float kDelayAccountForBaseCreation_s = 5.0;

// Interval at which disconnected cube orientations are pulled from block world
static const float kIntervalCheckCubeOrientation = 1.0;
  
// Pyramid Light constants
static const constexpr uint kBaseFormedTimeOn                  = 500;
static const constexpr uint kPyramidDenouementBaseOff_ms       = 650;
static const constexpr uint kPyramidDenouementAdditionalOff_ms = 75;

  
static const std::map<AxisName,UpAxis> kAxisNameMap = {
  {AxisName::Z_POS, UpAxis::ZPositive},
  {AxisName::Z_NEG, UpAxis::ZNegative},
  {AxisName::Y_POS, UpAxis::YPositive},
  {AxisName::Y_NEG, UpAxis::YNegative},
  {AxisName::X_POS, UpAxis::XPositive},
  {AxisName::X_NEG, UpAxis::XNegative}
};
  


static ObjectLights kEmptyObjectLights = {};

}

struct PyramidCubePropertiesTracker{
private:
  ObjectID _objectID = 0;
  UpAxis _currentUpAxis = UpAxis::Unknown;
  CubeAnimationTrigger _currentLightTrigger = CubeAnimationTrigger::Count;
  CubeAnimationTrigger _desiredLightTrigger = CubeAnimationTrigger::Count;
  ObjectLights _desiredLightModifier = ObjectLights();
  PyramidBehaviorChooser::PyramidAssignment _assignment = PyramidBehaviorChooser::PyramidAssignment::None;
  bool _hasAcknowledgedPositively = false;
  bool _hasEverBeenUpright = false;
  
public:
  ObjectID GetObjectID() const{ return _objectID;}
  UpAxis GetCurrentUpAxis() const{ return _currentUpAxis;}
  CubeAnimationTrigger GetCurrentLightTrigger() const{ return _currentLightTrigger;}
  CubeAnimationTrigger GetDesiredLightTrigger() const{ return _desiredLightTrigger;}
  const ObjectLights& GetDesiredLightModifier() const{ return _desiredLightModifier;}
  PyramidBehaviorChooser::PyramidAssignment GetPyramidAssignment() const{ return _assignment;}
  bool GetHasAcknowledgedPositively() const { return _hasAcknowledgedPositively;}
  bool GetHasEverBeenUpright(){ return _hasEverBeenUpright;}
  
  void SetObjectID(ObjectID objectID){ _objectID = objectID;}
  void SetUpAxis(UpAxis upAxis){
    _currentUpAxis = upAxis;
  }
  void SetCurrentLightTrigger(CubeAnimationTrigger trigger)
         { _currentLightTrigger = trigger;}
  void SetDesiredLightModifier(const ObjectLights& modifier)
         { _desiredLightModifier = modifier;}
  void SetDesiredLightTrigger(CubeAnimationTrigger trigger)
         { _desiredLightTrigger = trigger;}
  void SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment assignment)
         { _assignment = assignment;}
  void SetHasAcknowledgedPositively(bool hasAcknowledged) { _hasAcknowledgedPositively = hasAcknowledged;}
  void SetHasEverBeenUpright(bool upright){ _hasEverBeenUpright = upright;}
};
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BuildPyramidBehaviorChooser::BuildPyramidBehaviorChooser(Robot& robot, const Json::Value& config)
: SimpleBehaviorChooser(robot, config)
, _chooserPhase(ChooserPhase::None)
, _lastUprightBlockCount(-1)
, _pyramidObjectiveAchieved(false)
, _nextTimeCheckBlockOrientations_s(-1)
, _currentPyramidConstructionStage(PyramidConstructionStage::None)
, _highestAudioStageReached(PyramidConstructionStage::None)
, _lastTimeConstructionStageChanged_s(0)
, _lastCountBasesSeen(0)
, _uprightAnimIndex(0)
, _onSideAnimIndex(0)
, _forceLightMusicUpdate(false)
, _lightsShouldMessageCubeOnSide(false)
{
  UpdateActiveBehaviorGroup(_robot, true);
  
  JsonTools::GetValueOptional(config,
                              kLightsShouldMessageCubeOnSideKey,
                              _lightsShouldMessageCubeOnSide);
  
  /////////
  // Get pointers to all behaviors that must be manually called
  ///////
  
  // Get the build pyramid base behavior
  IBehavior* baseRaw = robot.GetBehaviorFactory().FindBehaviorByName("sparksBuildPyramidBase");
  DEV_ASSERT(baseRaw != nullptr &&
             baseRaw->GetClass() == BehaviorClass::BuildPyramidBase,
             "BuildPyramidBehaviorChooser.BuildPyramidBase.ImproperClassRetrievedForName");

  _behaviorBuildPyramidBase = dynamic_cast<BehaviorBuildPyramidBase*>(baseRaw);
  DEV_ASSERT(_behaviorBuildPyramidBase,
             "BuildPyramidBehaviorChooser.BehaviorBuildBase.PointerNotSet");
  
  // Get the build pyramid behavior
  IBehavior* pyramidRaw = robot.GetBehaviorFactory().FindBehaviorByName("sparksBuildPyramid");
  DEV_ASSERT(pyramidRaw != nullptr &&
             pyramidRaw->GetClass() == BehaviorClass::BuildPyramid,
             "BuildPyramidBehaviorChooser.BuildPyramid.ImproperClassRetrievedForName");
  
  _behaviorBuildPyramid = dynamic_cast<BehaviorBuildPyramid*>(pyramidRaw);
  DEV_ASSERT(_behaviorBuildPyramid,
             "BuildPyramidBehaviorChooser.BehaviorBuildPyramid.PointerNotSet");
  
  // Get the put down cube behavior
  IBehavior* putDownRaw = robot.GetBehaviorFactory().FindBehaviorByName("sparksPyramidPutDownBlock");
  DEV_ASSERT(putDownRaw != nullptr &&
             putDownRaw->GetClass() == BehaviorClass::PutDownBlock,
             "BuildPyramidBehaviorChooser.PutDownBlock.ImproperClassRetrievedForName");
  
  // Get the respond possibly roll behavior
  IBehavior* respondRoll = robot.GetBehaviorFactory().FindBehaviorByName("sparksPyramidRespondPossiblyRoll");
  DEV_ASSERT(respondRoll != nullptr &&
             respondRoll->GetClass() == BehaviorClass::RespondPossiblyRoll,
             "BuildPyramidBehaviorChooser.RespondRoll.ImproperClassRetrievedForName");
  
  _behaviorRespondPossiblyRoll = dynamic_cast<BehaviorRespondPossiblyRoll*>(respondRoll);
  DEV_ASSERT(_behaviorRespondPossiblyRoll,
             "BuildPyramidBehaviorChooser.RespondRoll.PointerNotSet");
  
  // Get the pyramid thank you behavior
  IBehavior* pyramidThankYou = robot.GetBehaviorFactory().FindBehaviorByName("pyramidThankYou");
  DEV_ASSERT(pyramidThankYou != nullptr &&
             pyramidThankYou->GetClass() == BehaviorClass::PyramidThankYou,
             "BuildPyramidBehaviorChooser.PyramidThankYou.ImproperClassRetrievedForName");
  
  _behaviorPyramidThankYou = dynamic_cast<BehaviorPyramidThankYou*>(pyramidThankYou);
  DEV_ASSERT(_behaviorRespondPossiblyRoll,
             "BuildPyramidBehaviorChooser.PyramidThankYou.PointerNotSet");
  
  
  /////////
  // Setup callbacks to update cube light patterns/phase
  ///////
  
  
  // Listen for up-axis changes to update response scenarios
  auto upAxisChangedCallback = [this](const EngineToGameEvent& event) {
    PyramidCubePropertiesTracker* propertyTracker = nullptr;
    ObjectID objID = event.GetData().Get_ObjectUpAxisChanged().objectID;
    if(!GetCubePropertiesTrackerByID(objID, propertyTracker))
    {
      UpdateStateTrackerForUnrecognizedID(objID);
      ANKI_VERIFY(GetCubePropertiesTrackerByID(objID, propertyTracker),
                 "BuildPyramidBehaviorChooser.ObjectNotAddedToTracker.TrackerIsStillNullptr", "");
    }
    if(propertyTracker != nullptr){
      propertyTracker->SetUpAxis(event.GetData().Get_ObjectUpAxisChanged().upAxis);
      _objectAxisChangeIDs.insert(event.GetData().Get_ObjectUpAxisChanged().objectID);
    }
  };
  
  auto objectiveAchievedCallback = [this](const EngineToGameEvent& event){
    if(event.GetData().Get_BehaviorObjectiveAchieved().behaviorObjective ==
       BehaviorObjective::BuiltPyramid){
      _pyramidObjectiveAchieved = true;
    }
  };
  
  if(robot.HasExternalInterface()){
    using namespace ExternalInterface;
    
    _eventHalders.push_back(robot.GetExternalInterface()->Subscribe(
                          MessageEngineToGameTag::ObjectUpAxisChanged,
                          upAxisChangedCallback));
    _eventHalders.push_back(robot.GetExternalInterface()->Subscribe(
                          MessageEngineToGameTag::BehaviorObjectiveAchieved,
                          objectiveAchievedCallback));
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BuildPyramidBehaviorChooser::~BuildPyramidBehaviorChooser()
{
  _behaviorPyramidThankYou = nullptr;
  _behaviorRespondPossiblyRoll = nullptr;
  _behaviorBuildPyramidBase = nullptr;
  _behaviorBuildPyramid = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::OnSelected()
{
  _uprightAnimIndex = 0;
  _onSideAnimIndex = 0;
  _lastUprightBlockCount = -1;
  _currentPyramidConstructionStage = PyramidConstructionStage::None;
  _highestAudioStageReached = PyramidConstructionStage::None;
  _chooserPhase = ChooserPhase::None;
  _nextTimeCheckBlockOrientations_s = -1;
  UpdateActiveBehaviorGroup(_robot, true);

  _pyramidObjectiveAchieved = false;
  
  for(auto& entry: _pyramidCubePropertiesTrackers){
    entry.second.SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment::None);
    entry.second.SetHasAcknowledgedPositively(false);
    entry.second.SetDesiredLightTrigger(CubeAnimationTrigger::Count);
    entry.second.SetHasEverBeenUpright(UpAxis::ZPositive == entry.second.GetCurrentUpAxis());
  }
  
  _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(),
                                                           ReactionTrigger::FistBump,
                                                           false);
  
  _forceLightMusicUpdate = true;
  
  UpdateChooserPhase(_robot);
  Update(_robot);

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::OnDeselected()
{
  // Make sure that all custom patterns are cleared off of the cubes
  for(auto& entry: _pyramidCubePropertiesTrackers){
    entry.second.SetDesiredLightTrigger(CubeAnimationTrigger::Count);
  }
  SetCubeLights(_robot);
  
  // Make sure no behaviors are deactivated on leaving pyramid in case they're
  // also mapped to another behavior group
  SetBehaviorGroupEnabled(BehaviorGroup::SetupBuildPyramid, true);
  SetBehaviorGroupEnabled(BehaviorGroup::BuildPyramid, true);
  _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(),
                                                          ReactionTrigger::ObjectPositionUpdated,
                                                          true);
  _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(),
                                                           ReactionTrigger::FistBump,
                                                           true);

}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateActiveBehaviorGroup(Robot& robot, bool settingUpPyramid)
{
  // Order matters
  if(settingUpPyramid){
    SetBehaviorGroupEnabled(BehaviorGroup::BuildPyramid, false);
    SetBehaviorGroupEnabled(BehaviorGroup::SetupBuildPyramid, true);
    // The setup phase has its own acknowledgments
    robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(),
                                                            ReactionTrigger::ObjectPositionUpdated,
                                                            false);
    
  }else{
    SetBehaviorGroupEnabled(BehaviorGroup::SetupBuildPyramid, false);
    SetBehaviorGroupEnabled(BehaviorGroup::BuildPyramid, true);
    robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(),
                                                            ReactionTrigger::ObjectPositionUpdated,
                                                            true);
  }
}



//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
// General Chooser Functions
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BuildPyramidBehaviorChooser::GetCubePropertiesTrackerByID(const ObjectID& id, PyramidCubePropertiesTracker*& pyramidCubeProperties)
{
  for(auto& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetObjectID() == id){
      pyramidCubeProperties = &entry.second;
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BuildPyramidBehaviorChooser::GetCubePropertiesTrackerByAssignment(
                        const PyramidBehaviorChooser::PyramidAssignment& id,
                        PyramidCubePropertiesTracker*& pyramidCubeProperties)
{
  for(auto& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetPyramidAssignment() == id){
      pyramidCubeProperties = &entry.second;
      return true;
    }
  }
  
  return false;
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::CheckBlockWorldCubeOrientations(Robot& robot)
{
  typedef std::vector<const ObservableObject*> BlockList;
  
  BlockList knownDisconnectedBlocks;
  BlockWorldFilter knownDisconnectedBlockFilter;
  knownDisconnectedBlockFilter.SetAllowedFamilies(
                         {{ObjectFamily::LightCube, ObjectFamily::Block}});
  // Only rely on this block world update if the block is both known
  // and disconnected - otherwise, the up axis message is a more reliable update
  knownDisconnectedBlockFilter.SetFilterFcn([](const ObservableObject* ptr){
    if(ptr->IsPoseStateKnown() && ptr->GetActiveID() == -1){
      return true;
    }
    return false;
  });
  
  robot.GetBlockWorld().FindMatchingObjects(knownDisconnectedBlockFilter,
                                            knownDisconnectedBlocks);
  
  // we only want to update orientations from block world if the pose state is known
  // because the pose is only updated through observation, so if we've received
  // an axis changed message from the cube that is more accurate information which
  // the rotation matrix will contradict
  for(const ObservableObject* block: knownDisconnectedBlocks){
    if (nullptr != block)
    {
      PyramidCubePropertiesTracker* currentpyramidCubeProperties = nullptr;
      GetCubePropertiesTrackerByID(block->GetID(), currentpyramidCubeProperties);
      if(currentpyramidCubeProperties == nullptr)
      {
        // If the block with that ID doesn't exist, create a new one
        UpdateStateTrackerForUnrecognizedID(block->GetID());
        ANKI_VERIFY(GetCubePropertiesTrackerByID(block->GetID(), currentpyramidCubeProperties),
                    "BuildPyramidBehaviorChooser.BlockWorldObjectNotAddedToTracker.TrackerIsStillNullptr", "");
      }
      
      AxisName name = block->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
      const UpAxis& currentUpAxis = kAxisNameMap.find(name)->second;
      
      if(currentUpAxis != currentpyramidCubeProperties->GetCurrentUpAxis()){
        currentpyramidCubeProperties->SetUpAxis(currentUpAxis);
        _objectAxisChangeIDs.insert(currentpyramidCubeProperties->GetObjectID());
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateStateTrackerForUnrecognizedID(const ObjectID& objID)
{

  BlockWorldFilter blockInAnyFrameFilter;
  blockInAnyFrameFilter.SetAllowedIDs({objID});
  blockInAnyFrameFilter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  blockInAnyFrameFilter.SetFilterFcn(nullptr);
  const ObservableObject* block = _robot.GetBlockWorld().FindMatchingObject(blockInAnyFrameFilter);
  
  DEV_ASSERT(block != nullptr,
             "BuildPyramidBehaviorChooser.UpdateStateTracker.NoBlocksWithID");
  if(block != nullptr){
    // Remove previous entry for block type if it exists
    auto blockTypeIter = _pyramidCubePropertiesTrackers.find(block->GetType());
    if(blockTypeIter != _pyramidCubePropertiesTrackers.end()){
      _pyramidCubePropertiesTrackers.erase(blockTypeIter);
    }
    
    PyramidCubePropertiesTracker newTracker;
    newTracker.SetObjectID(objID);
    _pyramidCubePropertiesTrackers.insert(std::make_pair(block->GetType(), newTracker));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdatePyramidAssignments(const BehaviorBuildPyramidBase* behavior)
{
  for(auto& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetPyramidAssignment() != PyramidBehaviorChooser::PyramidAssignment::None){
      _forceLightMusicUpdate = true;
      entry.second.SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment::None);
    }
  }
  
  // Allows assignments to be cleared out by passing in nullptr
  if(behavior == nullptr){
    return;
  }
  
  
  ObjectID id;
  PyramidCubePropertiesTracker* tracker = nullptr;
  
  if(behavior->GetBaseBlockID(id)){
    if(GetCubePropertiesTrackerByID(id, tracker)){
      tracker->SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment::BaseBlock);
    }
  }
  
  if(behavior->GetStaticBlockID(id)){
    if(GetCubePropertiesTrackerByID(id, tracker)){
      tracker->SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment::StaticBlock);
    }
  }
  
  if(behavior->GetTopBlockID(id)){
    if(GetCubePropertiesTrackerByID(id, tracker)){
      tracker->SetPyramidAssignment(PyramidBehaviorChooser::PyramidAssignment::TopBlock);
    }
  }
  
}


//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
// Functions relating to choose next behaivor
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BuildPyramidBehaviorChooser::ChooseNextBehavior(Robot& robot,
                                                           const IBehavior* currentRunningBehavior)
{
  UpdateTrackerPropertiesBasedOnCurrentRunningBehavior(currentRunningBehavior);

  // Thank the user if possible
  IBehavior* bestBehavior = CheckForShouldThankUser(robot, currentRunningBehavior);
  
  // Otherwise, see if we have to roll or respond to a block
  if(bestBehavior == nullptr){
    bestBehavior = CheckForResponsePossiblyRoll(robot, currentRunningBehavior);
  }
  
  // Otherwise proceed to specific chooser phase
  if(bestBehavior == nullptr){
    switch(_chooserPhase){
      case ChooserPhase::SetupBlocks:
      {
        bestBehavior = ChooseNextBehaviorSetup(robot, currentRunningBehavior);
        break;
      }
      case ChooserPhase::BuildingPyramid:
      {
        bestBehavior = ChooseNextBehaviorBuilding(robot, currentRunningBehavior);
        break;
      }
      case ChooserPhase::None:
      {
        DEV_ASSERT(false, "BuildPyramidBehaviorChooser.ChooseNextBehavior.InvalidPhase");
        break;
      }
    }
  }
  
  _objectAxisChangeIDs.clear();
  return bestBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior*  BuildPyramidBehaviorChooser::ChooseNextBehaviorSetup(Robot& robot,
                                                                 const IBehavior* currentRunningBehavior)
{
  return BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior*  BuildPyramidBehaviorChooser::ChooseNextBehaviorBuilding(Robot& robot,
                                                                    const IBehavior* currentRunningBehavior)
{
  //  Priority of functions:
  //    Build full pyramid -> Build pyramid base -> Search/fast forward behaviors
  
  IBehavior* bestBehavior = nullptr;
  BehaviorPreReqRobot kRobotPreReq(robot);
  
  if(_behaviorBuildPyramid->IsRunning() ||
     _behaviorBuildPyramid->IsRunnable(kRobotPreReq)){
    
    bestBehavior = _behaviorBuildPyramid;
    // If the behavior has not been running, update pyramid assignments
    // and then re-set base lights to reflect any changes of base assignment
    if(!_behaviorBuildPyramid->IsRunning()){
      UpdatePyramidAssignments(_behaviorBuildPyramid);
      SetPyramidBaseLights(robot);
    }
    
  }else if(_behaviorBuildPyramidBase->IsRunning() ||
           _behaviorBuildPyramidBase->IsRunnable(kRobotPreReq)){
    
    bestBehavior = _behaviorBuildPyramidBase;
    // If the behavior has not been running, update pyramid assignments
    // and then re-set base lights to reflect any changes of base assignment
    if(!_behaviorBuildPyramidBase->IsRunning()){
      UpdatePyramidAssignments(_behaviorBuildPyramidBase);
      SetPyramidBaseLights(robot);
    }
    
  }else{
    UpdatePyramidAssignments(nullptr);
    
    bestBehavior = BaseClass::ChooseNextBehavior(robot, currentRunningBehavior);
  }
  
  return bestBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BuildPyramidBehaviorChooser::CheckForShouldThankUser(Robot& robot,
                                                                const IBehavior* currentRunningBehavior)
{
  // Run through all of the axis changes to find if thank you can run
  // and to update the pyramidCubeProperties tracking information
  IBehavior* bestBehavior = nullptr;
  for(const auto& objectID: _objectAxisChangeIDs){
    PyramidCubePropertiesTracker* pyramidCubeProperties = nullptr;
    
    if(GetCubePropertiesTrackerByID(objectID, pyramidCubeProperties) &&
       pyramidCubeProperties != nullptr &&
       !pyramidCubeProperties->GetHasEverBeenUpright() &&
       pyramidCubeProperties->GetCurrentUpAxis() == UpAxis::ZPositive){
      
      const bool runningRollCube =
        currentRunningBehavior != nullptr &&
        currentRunningBehavior->GetClass() == BehaviorClass::RespondPossiblyRoll &&
        _behaviorRespondPossiblyRoll->GetResponseMetadata().GetObjectID() == objectID;
      
      bool rolledCubeHimself = false;
      if(runningRollCube){
        const auto& metadata = _behaviorRespondPossiblyRoll->GetResponseMetadata();
        rolledCubeHimself = metadata.GetReachedPreDocRoll();
      }
      
      if(!rolledCubeHimself){
        BehaviorPreReqAcknowledgeObject preReqObj(objectID);
        if(_behaviorPyramidThankYou->IsRunnable(preReqObj)){
          bestBehavior = _behaviorPyramidThankYou;
        }
      }
    }
    
    if(pyramidCubeProperties != nullptr &&
       pyramidCubeProperties->GetCurrentUpAxis() == UpAxis::ZPositive){
      pyramidCubeProperties->SetHasEverBeenUpright(true);
    }
  }
  
  // If a thank you is already running, return it so that it's not interrupted
  if(currentRunningBehavior != nullptr &&
     currentRunningBehavior->GetClass() == _behaviorPyramidThankYou->GetClass()){
    return _behaviorPyramidThankYou;
  }
  
  // Otherwise, return the best new thank you (if there is one)
  return bestBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BuildPyramidBehaviorChooser::CheckForResponsePossiblyRoll(Robot& robot,
                                                                     const IBehavior* currentRunningBehavior)
{
  // If any of the manually set behaviors are running, keep them running
  if(currentRunningBehavior != nullptr &&
     currentRunningBehavior->IsRunning()){
    if(currentRunningBehavior->GetClass() ==
       _behaviorRespondPossiblyRoll->GetClass()){
      return _behaviorRespondPossiblyRoll;
    }
  }
  
  IBehavior* bestBehavior = nullptr;
  BehaviorPreReqNone kNoPreReqs;
  int numberOfCubesOnSide = 0;
  
  for(auto& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetCurrentUpAxis() != UpAxis::ZPositive){
      numberOfCubesOnSide++;
    }
    
    ObservableObject* object = robot.GetBlockWorld().GetObjectByID(entry.second.GetObjectID());
    if(object != nullptr && !object->IsPoseStateUnknown()){
      if(entry.second.GetCurrentUpAxis() != UpAxis::ZPositive){
        BehaviorPreReqRespondPossiblyRoll preReqData(entry.second.GetObjectID(),
                                                     _uprightAnimIndex,
                                                     _onSideAnimIndex);
        if(_behaviorRespondPossiblyRoll->IsRunnable(preReqData)){
          bestBehavior = _behaviorRespondPossiblyRoll;
        }
        break;
      }
      
      if(bestBehavior == nullptr && !entry.second.GetHasAcknowledgedPositively()){
        const bool isSoftSpark = robot.GetBehaviorManager().IsActiveSparkSoft();

        const int onSideIdx = (_lightsShouldMessageCubeOnSide && !isSoftSpark) ?
                                                 _onSideAnimIndex : -1;
        
        BehaviorPreReqRespondPossiblyRoll preReqData(entry.second.GetObjectID(),
                                                     _uprightAnimIndex,
                                                     onSideIdx);
        if(_behaviorRespondPossiblyRoll->IsRunnable(preReqData)){
          bestBehavior = _behaviorRespondPossiblyRoll;
        }
      }
    }
  }
  
  // We don't want to acknowledge positively if all cubes are upright and we can start
  // building
  return  numberOfCubesOnSide != 0 ? bestBehavior : nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateTrackerPropertiesBasedOnCurrentRunningBehavior(const IBehavior* currentRunningBehavior)
{
  // Update respondPossiblyRoll tracker info
  if(currentRunningBehavior != nullptr &&
     currentRunningBehavior->GetClass() == BehaviorClass::RespondPossiblyRoll){
    
    const auto& metadata =  _behaviorRespondPossiblyRoll->GetResponseMetadata();
    
    if(metadata.GetPlayedUprightAnim()){
      _uprightAnimIndex = metadata.GetUprightAnimIndex() + 1;
    }
    
    if(metadata.GetPlayedOnSideAnim()){
      _onSideAnimIndex = metadata.GetOnSideAnimIndex() + 1;
    }
    
    // Set acknowledged positevely if response was a positive response
    const ObjectID& target = metadata.GetObjectID();
    PyramidCubePropertiesTracker* tracker = nullptr;
    ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(target);
    if(GetCubePropertiesTrackerByID(target, tracker) &&
       object != nullptr &&
       tracker->GetCurrentUpAxis() == UpAxis::ZPositive){
      tracker->SetHasAcknowledgedPositively(metadata.GetPlayedUprightAnim());
    }
  }
  
}

//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
// Functions relating to updating music/light state
//////////////////////////////////////////////
//////////////////////////////////////////////
//////////////////////////////////////////////
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BuildPyramidBehaviorChooser::Update(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(currentTime_s > _nextTimeCheckBlockOrientations_s){
    CheckBlockWorldCubeOrientations(robot);
    _nextTimeCheckBlockOrientations_s = currentTime_s + kIntervalCheckCubeOrientation;
  }
  
  if(_objectAxisChangeIDs.size() > 0 ||
     _chooserPhase == ChooserPhase::None){
    UpdateChooserPhase(robot);

  }
  
  PyramidConstructionStage desiredState = CheckLightAndPyramidConstructionStage(robot);
  
  // Reasons why music/lights might need to be updated
  const bool constructionStageChanged = desiredState != _currentPyramidConstructionStage;
  const bool pyramidSetupStageChanged = _chooserPhase == ChooserPhase::SetupBlocks &&
                                              _objectAxisChangeIDs.size() > 0;
  
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const bool numberOfPyramidBasesChanged = pyramidBases.size() != _lastCountBasesSeen;
  
  if(_forceLightMusicUpdate ||
     constructionStageChanged ||
     pyramidSetupStageChanged ||
     numberOfPyramidBasesChanged){
       UpdateMusic(robot, desiredState);
       UpdateDesiredLights(robot, desiredState);
       SetCubeLights(robot);
   }
  
  
  _lastCountBasesSeen = static_cast<int>(pyramidBases.size());
  _forceLightMusicUpdate = false;
  
  if(_currentPyramidConstructionStage != desiredState){
    _lastTimeConstructionStageChanged_s = currentTime_s;
    _currentPyramidConstructionStage = desiredState;
  }
  
  
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateChooserPhase(Robot& robot)
{
  // Check the up-axis of all cubes
  int countOfBlocksUpright = 0;
  for(auto const& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetCurrentUpAxis() == UpAxis::ZPositive){
      countOfBlocksUpright++;
    }
  }
  
  // If blocks have been removed since the last update,
  // they shouldn't be counted against the upright count
  if(_lastUprightBlockCount > _pyramidCubePropertiesTrackers.size()){
    _lastUprightBlockCount = static_cast<int>(_pyramidCubePropertiesTrackers.size());
  }
  
  // Notify game if the BuildPyramidPreReqs have change
  const bool uprightCountChanged = _lastUprightBlockCount != countOfBlocksUpright;
  if(uprightCountChanged && _robot.HasExternalInterface()){
    const bool minimumUprightCountReached =
      countOfBlocksUpright >= kMinUprightBlocksForPyramid;
    const bool fellBelowMinimumUprightCount =
      (_lastUprightBlockCount >= kMinUprightBlocksForPyramid) &&
      (countOfBlocksUpright < kMinUprightBlocksForPyramid);
    
    const bool isSoftSpark = robot.GetBehaviorManager().IsActiveSparkSoft();
    const bool didUserRequestSparkEnd = robot.GetBehaviorManager().DidGameRequestSparkEnd();

    
    if(minimumUprightCountReached && !isSoftSpark && !didUserRequestSparkEnd){
      _robot.GetExternalInterface()->BroadcastToGame<
      ExternalInterface::BuildPyramidPreReqsChanged>(true);
    }else if(fellBelowMinimumUprightCount && !isSoftSpark && !didUserRequestSparkEnd){
      _robot.GetExternalInterface()->BroadcastToGame<
      ExternalInterface::BuildPyramidPreReqsChanged>(false);
    }
  }
  
  // Check to see if the chooser phase has changed
  if(countOfBlocksUpright >= kMinUprightBlocksForPyramid ||
     countOfBlocksUpright == _pyramidCubePropertiesTrackers.size()){
    if(_chooserPhase != ChooserPhase::BuildingPyramid){
      _chooserPhase = ChooserPhase::BuildingPyramid;
      UpdateActiveBehaviorGroup(_robot, false);
    }
  }else{
    if(_chooserPhase != ChooserPhase::SetupBlocks){
      _chooserPhase = ChooserPhase::SetupBlocks;
      UpdateActiveBehaviorGroup(_robot, true);
    }
  }
  
  _lastUprightBlockCount = countOfBlocksUpright;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BuildPyramidBehaviorChooser::PyramidConstructionStage BuildPyramidBehaviorChooser::CheckLightAndPyramidConstructionStage(Robot& robot) const
{
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // Once we've started to play the success sequence, no going back
  if(_pyramidObjectiveAchieved){
    return PyramidConstructionStage::PyramidCompleteFlourish;
  }
  
  if(robot.GetOffTreadsState() != OffTreadsState::OnTreads ||
     _chooserPhase == ChooserPhase::SetupBlocks){
    return PyramidConstructionStage::None;
  }
  
  // Logic for updating lights/music while building pyramid
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  if((pyramidBases.size() == 0 && pyramids.size() == 0)){
    // there is a range in which we don't want to cancel lights while placing blocks
    const bool possiblyPlacingBase =
      _currentPyramidConstructionStage == PyramidConstructionStage::InitialCubeCarry &&
      (_behaviorBuildPyramid->IsRunning() || _behaviorBuildPyramidBase->IsRunning()) &&
      (_lastTimeConstructionStageChanged_s + kDelayAccountForPlacing_s > currentTime_s ||
       _lastTimeConstructionStageChanged_s + kDelayAccountForBaseCreation_s < currentTime_s);
    
    if(robot.IsCarryingObject() || possiblyPlacingBase){
      return PyramidConstructionStage::InitialCubeCarry;
    }else{
      return PyramidConstructionStage::SearchingForCube;
    }
  }else{
    // There's a gap between when the top block is "placed" and the final
    // pyramid is recognized - if the behavior is still running and we've
    // just been in a carrying state, don't cut the music/lights suddenly
    const bool behaviorStillPlacingBlock = _behaviorBuildPyramid->IsRunning() &&
                 (_currentPyramidConstructionStage == PyramidConstructionStage::TopBlockCarry);
    if(robot.IsCarryingObject() || behaviorStillPlacingBlock){
      return PyramidConstructionStage::TopBlockCarry;
    }else{
      return PyramidConstructionStage::BaseFormed;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateMusic(Robot& robot, const PyramidConstructionStage& desiredState)
{
  if(desiredState > _highestAudioStageReached){
    _highestAudioStageReached = desiredState;
    if(desiredState == PyramidConstructionStage::None){
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
              UnlockId::BuildPyramid, Util::EnumToUnderlying(PyramidConstructionStage::SearchingForCube));
    }else{
      robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(
              UnlockId::BuildPyramid, Util::EnumToUnderlying(desiredState));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::UpdateDesiredLights(Robot& robot, const PyramidConstructionStage& desiredState)
{
  CubeAnimationTrigger triggerForBase = CubeAnimationTrigger::Count;
  CubeAnimationTrigger triggerForStatic = CubeAnimationTrigger::Count;
  CubeAnimationTrigger triggerForTop = CubeAnimationTrigger::Count;
  
  ObjectLights baseModifier = kEmptyObjectLights;
  ObjectLights staticModifier = kEmptyObjectLights;
  ObjectLights topModifier = kEmptyObjectLights;
  
  bool baseLightsSet = false;
  bool staticLightsSet = false;
  bool topLightsSet = false;
  
  //////
  /// Determine the light triggers/modifiers to set
  //////
  
  switch(desiredState){
    case PyramidConstructionStage::SearchingForCube:
    {
      triggerForTop = CubeAnimationTrigger::Count;
      topLightsSet = true;
      if(!SetPyramidBaseLights(robot)){
        triggerForBase = CubeAnimationTrigger::Count;
        triggerForStatic = CubeAnimationTrigger::Count;
        baseLightsSet = true;
        staticLightsSet = true;
      }
      break;
    }
    case PyramidConstructionStage::InitialCubeCarry:
    {
      triggerForBase = CubeAnimationTrigger::PyramidSingle;
      triggerForStatic = CubeAnimationTrigger::PyramidPickup;
      baseLightsSet = true;
      staticLightsSet = true;
      break;
    }
    case PyramidConstructionStage::BaseFormed:
    case PyramidConstructionStage::TopBlockCarry:
    {
      if(desiredState == PyramidConstructionStage::BaseFormed){
        triggerForTop = CubeAnimationTrigger::PyramidSingle;
      }else{
        triggerForTop = CubeAnimationTrigger::PyramidPickup;
      }
      
      topLightsSet = true;
      SetPyramidBaseLights(robot);
      break;
    }
    case PyramidConstructionStage::PyramidCompleteFlourish:
    {
      triggerForBase = CubeAnimationTrigger::PyramidFlourish;
      triggerForStatic = CubeAnimationTrigger::PyramidFlourish;
      triggerForTop = CubeAnimationTrigger::PyramidFlourish;
      baseModifier = GetDenouementBottomLightsModifier();
      staticModifier = GetDenouementBottomLightsModifier();
      
      baseLightsSet = true;
      staticLightsSet = true;
      topLightsSet = true;
      break;
    }
    case PyramidConstructionStage::None:
    {
      // Update "onSide" lights based on block current state
      for(auto& entry: _pyramidCubePropertiesTrackers){
        if(entry.second.GetCurrentUpAxis() != UpAxis::ZPositive &&
           entry.second.GetCurrentLightTrigger() != CubeAnimationTrigger::PyramidOnSide){
          entry.second.SetDesiredLightTrigger(CubeAnimationTrigger::PyramidOnSide);
        }else if(entry.second.GetCurrentUpAxis() == UpAxis::ZPositive &&
                 entry.second.GetCurrentLightTrigger() != CubeAnimationTrigger::Count){
          entry.second.SetDesiredLightTrigger(CubeAnimationTrigger::Count);
        }
      }
      break;
    }
  }
  
  
  // Make sure that on side lights are cleared out if any cubes were on their side
  if(_currentPyramidConstructionStage == PyramidConstructionStage::None &&
     desiredState != PyramidConstructionStage::None){
    for(auto& entry:_pyramidCubePropertiesTrackers){
      if(entry.second.GetCurrentLightTrigger() == CubeAnimationTrigger::PyramidOnSide){
        entry.second.SetDesiredLightTrigger(CubeAnimationTrigger::Count);
      }
    }
  }
  
  
  //////
  /// Set the light triggers/modifiers on the appropriate tracker
  //////
  
  PyramidCubePropertiesTracker* baseBlockTracker = nullptr;
  if(GetCubePropertiesTrackerByAssignment(PyramidBehaviorChooser::PyramidAssignment::BaseBlock, baseBlockTracker) &&
     baseBlockTracker != nullptr &&
     baseLightsSet)
  {
    baseBlockTracker->SetDesiredLightTrigger(triggerForBase);
    baseBlockTracker->SetDesiredLightModifier(baseModifier);
  }
  
  PyramidCubePropertiesTracker* staticBlockTracker = nullptr;
  if(GetCubePropertiesTrackerByAssignment(PyramidBehaviorChooser::PyramidAssignment::StaticBlock, staticBlockTracker) &&
     staticBlockTracker != nullptr &&
     staticLightsSet)
  {
    staticBlockTracker->SetDesiredLightTrigger(triggerForStatic);
    staticBlockTracker->SetDesiredLightModifier(staticModifier);
  }
  
  PyramidCubePropertiesTracker* topBlockTracker = nullptr;
  if(GetCubePropertiesTrackerByAssignment(PyramidBehaviorChooser::PyramidAssignment::TopBlock, topBlockTracker) &&
     topBlockTracker != nullptr &&
     topLightsSet)
  {
    topBlockTracker->SetDesiredLightTrigger(triggerForTop);
    topBlockTracker->SetDesiredLightModifier(topModifier);
  }
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BuildPyramidBehaviorChooser::SetCubeLights(Robot& robot)
{
  // Remove any lights that are currently set
  for(auto& entry: _pyramidCubePropertiesTrackers){
    if(entry.second.GetCurrentLightTrigger() != entry.second.GetDesiredLightTrigger() ||
       entry.second.GetDesiredLightModifier() != kEmptyObjectLights)
    {
      const bool isSoftSpark = robot.GetBehaviorManager().IsActiveSparkSoft();
      const bool shouldSetForOnSide = (_lightsShouldMessageCubeOnSide && !isSoftSpark) ||
            entry.second.GetDesiredLightTrigger() != CubeAnimationTrigger::PyramidOnSide;
      
      if(entry.second.GetDesiredLightTrigger() != CubeAnimationTrigger::Count &&
         shouldSetForOnSide)
      {
        _robot.GetCubeLightComponent().StopAndPlayLightAnim(entry.second.GetObjectID(),
                                                            entry.second.GetCurrentLightTrigger(),
                                                            entry.second.GetDesiredLightTrigger(),
                                                            nullptr,
                                                            true,
                                                            entry.second.GetDesiredLightModifier());
      }
      else
      {
        _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(entry.second.GetCurrentLightTrigger(),
                                                                      entry.second.GetObjectID());
      }
      entry.second.SetCurrentLightTrigger(entry.second.GetDesiredLightTrigger());
      entry.second.SetDesiredLightModifier(kEmptyObjectLights);
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BuildPyramidBehaviorChooser::SetPyramidBaseLights(Robot& robot)
{
  
  // Clear out any existing base light triggers - they should be re-set below
  // if still valid and the lights won't update on SetCubeLights
  for(auto& cubeProperties: _pyramidCubePropertiesTrackers){
    if(cubeProperties.second.GetCurrentLightTrigger() == CubeAnimationTrigger::PyramidBaseBottom){
      cubeProperties.second.SetDesiredLightTrigger(CubeAnimationTrigger::PyramidBaseBottom);
    }
  }
  
  ObjectID baseBlockID;
  ObjectID staticBlockID;
  const auto& pyramidBases = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidBaseCache().GetBases();
  const auto& pyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  
  if(pyramidBases.size() == 1){
    const auto& base = pyramidBases[0];
    baseBlockID = base->GetBaseBlockID();
    staticBlockID = base->GetStaticBlockID();
  }else if(pyramidBases.size() > 1){
    // see if there's an assigment
    PyramidCubePropertiesTracker* baseBlockTracker = nullptr;
    PyramidCubePropertiesTracker* staticBlockTracker = nullptr;
    if(GetCubePropertiesTrackerByAssignment(PyramidBehaviorChooser::PyramidAssignment::BaseBlock, baseBlockTracker) &&
       GetCubePropertiesTrackerByAssignment(PyramidBehaviorChooser::PyramidAssignment::StaticBlock, staticBlockTracker) &&
       staticBlockTracker != nullptr &&
       baseBlockTracker != nullptr)
    {
      baseBlockID = baseBlockTracker->GetObjectID();
      staticBlockID = staticBlockTracker->GetObjectID();
    }else{
      const auto& base = pyramidBases[0];
      baseBlockID = base->GetBaseBlockID();
      staticBlockID = base->GetStaticBlockID();
    }
  }else if(pyramids.size() > 0){
    const auto& pyramid = pyramids[0];
    baseBlockID = pyramid->GetPyramidBase().GetBaseBlockID();
    staticBlockID = pyramid->GetPyramidBase().GetStaticBlockID();
  }
  
  if(baseBlockID.IsSet() && staticBlockID.IsSet()){
    PyramidCubePropertiesTracker* baseBlockTracker = nullptr;
    PyramidCubePropertiesTracker* staticBlockTracker = nullptr;
    if(GetCubePropertiesTrackerByID(baseBlockID, baseBlockTracker) &&
       GetCubePropertiesTrackerByID(staticBlockID, staticBlockTracker) &&
       staticBlockTracker != nullptr &&
       baseBlockTracker != nullptr)
    {
      if(baseBlockTracker->GetCurrentLightTrigger() != CubeAnimationTrigger::PyramidBaseBottom){
        baseBlockTracker->SetDesiredLightTrigger(CubeAnimationTrigger::PyramidBaseBottom);
        baseBlockTracker->SetDesiredLightModifier(
            GetBaseFormedBaseLightsModifier(robot,
                                            staticBlockTracker->GetObjectID(),
                                            baseBlockTracker->GetObjectID()));
      }
      
      if(staticBlockTracker->GetCurrentLightTrigger() != CubeAnimationTrigger::PyramidBaseBottom){
        staticBlockTracker->SetDesiredLightTrigger(CubeAnimationTrigger::PyramidBaseBottom);
        staticBlockTracker->SetDesiredLightModifier(
              GetBaseFormedStaticLightsModifier(robot,
                                                staticBlockTracker->GetObjectID(),
                                                baseBlockTracker->GetObjectID()));
      }
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLights BuildPyramidBehaviorChooser::GetBaseFormedBaseLightsModifier(Robot& robot,
                                                                          const ObjectID& staticID,
                                                                          const ObjectID& baseID) const
{
  ObjectLights baseBlockLights;
  baseBlockLights.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE;
  
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(staticID);
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(baseID);
  if(staticBlock == nullptr || baseBlock == nullptr){
    return baseBlockLights;
  }
  
  using namespace BlockConfigurations;
  Pose3d baseMidpoint;
  PyramidBase::GetBaseInteriorMidpoint(robot, baseBlock, staticBlock, baseMidpoint);
  
  baseBlockLights.relativePoint = {baseMidpoint.GetTranslation().x(),
    baseMidpoint.GetTranslation().y()};
  baseBlockLights.offset = {{kBaseFormedTimeOn*2,0,kBaseFormedTimeOn*4,kBaseFormedTimeOn*3}};
  
  return baseBlockLights;
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLights BuildPyramidBehaviorChooser::GetBaseFormedStaticLightsModifier(Robot& robot,
                                                                            const ObjectID& staticID,
                                                                            const ObjectID& baseID) const
{
  ObjectLights staticBlockLights;
  staticBlockLights.makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_BY_SIDE;
  
  const ObservableObject* staticBlock = robot.GetBlockWorld().GetObjectByID(staticID);
  const ObservableObject* baseBlock = robot.GetBlockWorld().GetObjectByID(baseID);
  if(staticBlock == nullptr || baseBlock == nullptr){
    return staticBlockLights;
  }
  
  using namespace BlockConfigurations;
  Pose3d staticMidpoint;
  PyramidBase::GetBaseInteriorMidpoint(robot, staticBlock, baseBlock, staticMidpoint);
  
  staticBlockLights.relativePoint = {staticMidpoint.GetTranslation().x(),
    staticMidpoint.GetTranslation().y()};
  
  return staticBlockLights;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectLights BuildPyramidBehaviorChooser::GetDenouementBottomLightsModifier() const
{
  ObjectLights kFlourishTopLights;
  kFlourishTopLights.offPeriod_ms ={{
    kPyramidDenouementBaseOff_ms - kPyramidDenouementAdditionalOff_ms,
    kPyramidDenouementBaseOff_ms,
    kPyramidDenouementBaseOff_ms + kPyramidDenouementAdditionalOff_ms,
    kPyramidDenouementBaseOff_ms + kPyramidDenouementAdditionalOff_ms*2
  }};
  
  return kFlourishTopLights;
  
}


} // namespace Cozmo
} // namespace Anki
