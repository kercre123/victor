/**
 * File: ActivityBuildPyramid.h
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for building a pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBuildPyramid_H__
#define __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBuildPyramid_H__

#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Json {
class Value;
}


namespace Anki {
namespace Cozmo {

// Forward declaration
class BehaviorBuildPyramidBase;
class BehaviorBuildPyramid;
class BehaviorExternalInterface;
class BehaviorPlayArbitraryAnim;
class BehaviorPyramidThankYou;
class BehaviorRespondPossiblyRoll;
struct PyramidCubePropertiesTracker;
struct ObjectConnectionState;
struct ObjectLights;
  
class ActivityBuildPyramid : public IActivity
{
public:
  ActivityBuildPyramid(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config);
  ~ActivityBuildPyramid();
  
  virtual Result Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  enum class PyramidAssignment{
    None,
    BaseBlock,
    StaticBlock,
    TopBlock
  };
  
protected:
  virtual IBehaviorPtr GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) override;

  virtual void OnActivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  enum class ChooserPhase{
    None, // Only used to force update
    SetupBlocks,
    BuildingPyramid
  };
  
  ////////
  /// General Chooser structs/tracking
  ///////  
  // Scored behavior choosers that take over when strict priority isn't necessary
  IBSRunnableChooser*                  _activeBehaviorChooser; // One of the two choosers from below
  std::unique_ptr<IBSRunnableChooser>  _setupSimpleChooser;
  std::unique_ptr<IBSRunnableChooser>  _buildSimpleChooser;
  
  std::vector<Signal::SmartHandle> _eventHandlers;
  // Maps a light cube type (in case objectIDs are re-assigned for disconnected objects)
  // to knowledge about how we've altered the light/axis state
  std::map<ObjectType, PyramidCubePropertiesTracker> _pyramidCubePropertiesTrackers;
  
  ////////
  /// Properties relating to choose next behaivor
  ///////
  
  
  ChooserPhase _chooserPhase;
  int _lastUprightBlockCount;
  bool _pyramidObjectiveAchieved;
  float _nextTimeCheckBlockOrientations_s;
  float _nextTimeForceUpdateLightMusic_s;
  // For tracking which cubes rotated this tick for thanking user/updating phase
  std::set<ObjectID> _objectAxisChangeIDs;
  
  // Created with factory
  std::shared_ptr<BehaviorPyramidThankYou> _behaviorPyramidThankYou;
  std::shared_ptr<BehaviorRespondPossiblyRoll> _behaviorRespondPossiblyRoll;
  std::shared_ptr<BehaviorBuildPyramidBase> _behaviorBuildPyramidBase;
  std::shared_ptr<BehaviorBuildPyramid> _behaviorBuildPyramid;
  
  
  ////////
  /// Properties relating to updating music/light state
  ///////
  
  PyramidConstructionStage _currentPyramidConstructionStage;
  PyramidConstructionStage _highestAudioStageReached;
  float _lastTimeConstructionStageChanged_s;
  int _lastCountBasesSeen;
  // For tracking the number of times cozmo has had to roll blocks to get
  // ready for pyramid
  int _uprightAnimIndex;
  int _onSideAnimIndex;
  bool _forceLightMusicUpdate;
  float _timeRespondedRollStartedPreviously_s;
  
  void HandleObjectConnectionStateChange(BehaviorExternalInterface& behaviorExternalInterface, const ObjectConnectionState& connectionState);
  void UpdateActiveBehaviorGroup(BehaviorExternalInterface& behaviorExternalInterface, bool settingUpPyramid);
  bool IsPyramidHardSpark(BehaviorExternalInterface& behaviorExternalInterface);
  
  ////////
  /// General Chooser functions
  ///////
  
  // initialize the chooser, return result of operation
  bool GetCubePropertiesTrackerByID(const ObjectID& id, PyramidCubePropertiesTracker*& lightState);
  bool GetCubePropertiesTrackerByAssignment(const PyramidAssignment& id,
                                            PyramidCubePropertiesTracker*& lightState);
  void CheckBlockWorldCubeOrientations(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateStateTrackerForUnrecognizedID(BehaviorExternalInterface& behaviorExternalInterface,
                                           const ObjectID& objID);
  void UpdatePyramidAssignments(const std::shared_ptr<BehaviorBuildPyramidBase> behavior);
  
  ////////
  /// Functions relating to choose next behaivor
  ///////
  
  // sub-behaviors to keep choosing behavior code cleaner based on stage
  IBehaviorPtr ChooseNextBehaviorSetup(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);
  IBehaviorPtr ChooseNextBehaviorBuilding(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);
  
  // These behaviors return a behavior pointer that should be choosen if they want something run
  // and nullptr if the requester should select a behavior given its own criteria
  IBehaviorPtr CheckForShouldThankUser(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);
  IBehaviorPtr CheckForResponsePossiblyRoll(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);
  
  // Some behaviors run directly by the chooser set properties once they've reached a
  // certain state.  This function ticks those behaviors when appropriate to find out
  // whether the PropertiesTracker needs to be updated to reflect the behavior's success
  void UpdatePropertiesTrackerBasedOnRespondPossiblyRoll(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior);
  
  
  ////////
  /// Functions relating to updating music/light state
  ///////
  void UpdateChooserPhase(BehaviorExternalInterface& behaviorExternalInterface);
  int  GetNumberOfBlocksUpright();
  void NotifyGameOfPyramidPreReqs(BehaviorExternalInterface& behaviorExternalInterface);

  PyramidConstructionStage CheckLightAndPyramidConstructionStage(BehaviorExternalInterface& behaviorExternalInterface) const;
  void UpdateMusic(BehaviorExternalInterface& behaviorExternalInterface, const PyramidConstructionStage& desiredState);
  void UpdateDesiredLights(BehaviorExternalInterface& behaviorExternalInterface, const PyramidConstructionStage& desiredState);
  void SetCubeLights(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool IsAnOnSideCubeLight(CubeAnimationTrigger anim);
  CubeAnimationTrigger GetAppropriateOnSideAnimation(BehaviorExternalInterface& behaviorExternalInterface,
                                                     const ObjectID& staticID);
  
  
  // Contains logic for maintaining base lights across multiple states
  // Returns true if base lights have been updated by it, false if
  // the requesting function can update the lights itself
  bool SetPyramidBaseLights(BehaviorExternalInterface& behaviorExternalInterface);
  
  ObjectLights GetDenouementBottomLightsModifier() const;
  ObjectLights GetBaseFormedBaseLightsModifier(BehaviorExternalInterface& behaviorExternalInterface,
                                               const ObjectID& staticID,
                                               const ObjectID& baseID) const;
  ObjectLights GetBaseFormedStaticLightsModifier(BehaviorExternalInterface& behaviorExternalInterface,
                                                 const ObjectID& staticID,
                                                 const ObjectID& baseID) const;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBuildPyramid_H__
