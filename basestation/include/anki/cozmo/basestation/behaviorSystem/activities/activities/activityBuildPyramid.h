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
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "clad/types/behaviorTypes.h"
#include "clad/types/objectTypes.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Json {
class Value;
}


namespace Anki {
namespace Cozmo {

// Forward declaration
class BehaviorPlayArbitraryAnim;
class BehaviorBuildPyramidBase;
class BehaviorBuildPyramid;
class BehaviorPyramidThankYou;
struct PyramidCubePropertiesTracker;
class BehaviorRespondPossiblyRoll;
struct ObjectLights;
  
class ActivityBuildPyramid : public IActivity
{
public:
  ActivityBuildPyramid(Robot& robot, const Json::Value& config);
  ~ActivityBuildPyramid();
  
  
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  virtual Result Update(Robot& robot) override;
  
  enum class PyramidAssignment{
    None,
    BaseBlock,
    StaticBlock,
    TopBlock
  };
  
protected:
  virtual void OnSelectedInternal(Robot& robot) override;
  virtual void OnDeselectedInternal(Robot& robot) override;
  
private:
  enum class ChooserPhase{
    None, // Only used to force update
    SetupBlocks,
    BuildingPyramid
  };
  
  ////////
  /// General Chooser structs/tracking
  ///////
  Robot& _robot;
  
  // Scored behavior choosers that take over when strict priority isn't necessary
  IBehaviorChooser* _activeBehaviorChooser; // One of the two choosers from below
  IBehaviorChooser* _setupSimpleChooser;
  IBehaviorChooser* _buildSimpleChooser;
  
  std::vector<Signal::SmartHandle> _eventHalders;
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
  BehaviorPyramidThankYou* _behaviorPyramidThankYou = nullptr;
  BehaviorRespondPossiblyRoll* _behaviorRespondPossiblyRoll = nullptr;
  BehaviorBuildPyramidBase* _behaviorBuildPyramidBase = nullptr;
  BehaviorBuildPyramid* _behaviorBuildPyramid = nullptr;
  
  
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
  
  
  void UpdateActiveBehaviorGroup(Robot& robot, bool settingUpPyramid);
  bool IsPyramidHardSpark(Robot& robot);
  
  ////////
  /// General Chooser functions
  ///////
  
  // initialize the chooser, return result of operation
  bool GetCubePropertiesTrackerByID(const ObjectID& id, PyramidCubePropertiesTracker*& lightState);
  bool GetCubePropertiesTrackerByAssignment(const PyramidAssignment& id,
                                            PyramidCubePropertiesTracker*& lightState);
  void CheckBlockWorldCubeOrientations(Robot& robot);
  void UpdateStateTrackerForUnrecognizedID(const ObjectID& objID);
  void UpdatePyramidAssignments(const BehaviorBuildPyramidBase* behavior);
  
  ////////
  /// Functions relating to choose next behaivor
  ///////
  
  // sub-behaviors to keep choosing behavior code cleaner based on stage
  IBehavior* ChooseNextBehaviorSetup(Robot& robot, const IBehavior* currentRunningBehavior);
  IBehavior* ChooseNextBehaviorBuilding(Robot& robot, const IBehavior* currentRunningBehavior);
  
  // These behaviors return a behavior pointer that should be choosen if they want something run
  // and nullptr if the requester should select a behavior given its own criteria
  IBehavior* CheckForShouldThankUser(Robot& robot, const IBehavior* currentRunningBehavior);
  IBehavior* CheckForResponsePossiblyRoll(Robot& robot, const IBehavior* currentRunningBehavior);
  
  // Some behaviors run directly by the chooser set properties once they've reached a
  // certain state.  This function ticks those behaviors when appropriate to find out
  // whether the PropertiesTracker needs to be updated to reflect the behavior's success
  void UpdatePropertiesTrackerBasedOnRespondPossiblyRoll(const IBehavior* currentRunningBehavior);
  
  
  ////////
  /// Functions relating to updating music/light state
  ///////
  void UpdateChooserPhase(Robot& robot);
  PyramidConstructionStage CheckLightAndPyramidConstructionStage(Robot& robot) const;
  void UpdateMusic(Robot& robot, const PyramidConstructionStage& desiredState);
  void UpdateDesiredLights(Robot& robot, const PyramidConstructionStage& desiredState);
  void SetCubeLights(Robot& robot);
  
  bool IsAnOnSideCubeLight(CubeAnimationTrigger anim);
  CubeAnimationTrigger GetAppropriateOnSideAnimation(Robot& robot,
                                                     const ObjectID& staticID);
  
  
  // Contains logic for maintaining base lights across multiple states
  // Returns true if base lights have been updated by it, false if
  // the requesting function can update the lights itself
  bool SetPyramidBaseLights(Robot& robot);
  
  ObjectLights GetDenouementBottomLightsModifier() const;
  ObjectLights GetBaseFormedBaseLightsModifier(Robot& robot,
                                               const ObjectID& staticID,
                                               const ObjectID& baseID) const;
  ObjectLights GetBaseFormedStaticLightsModifier(Robot& robot,
                                                 const ObjectID& staticID,
                                                 const ObjectID& baseID) const;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_Activities_Activities_ActivityBuildPyramid_H__
