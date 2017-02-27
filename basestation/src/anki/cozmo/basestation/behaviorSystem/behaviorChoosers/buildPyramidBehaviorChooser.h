/**
 * File: BuildPyramidBehaviorChooser.h
 *
 * Author: Kevin M. Karol
 * Created: 01/19/17
 *
 * Description: Chooser that manages the lights, music, and process of preparing
 * to build and build a pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_BuildPyramidBehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorChoosers_BuildPyramidBehaviorChooser_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "util/signals/simpleSignal_fwd.h"

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

  
namespace PyramidBehaviorChooser{
enum class PyramidAssignment{
  None,
  BaseBlock,
  StaticBlock,
  TopBlock
};
}
  
// A behavior chooser which handles soft sparking, intro/outro and relies on simple behavior chooser otherwise
class BuildPyramidBehaviorChooser : public SimpleBehaviorChooser
{
using BaseClass = SimpleBehaviorChooser;
  
public:
  // constructor/destructor
  BuildPyramidBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~BuildPyramidBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  virtual Result Update(Robot& robot) override;

  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // name (for debug/identification)
  virtual const char* GetName() const override { return "Pyramid"; }
  
  virtual void OnSelected() override;
  virtual void OnDeselected() override;
  
private:
  // Match music Rounds to enum values - starts at 1 to match rounds set up
  // in the current audio sound banks
  enum class PyramidConstructionStage{
    None = 0, // used for canceling lights
    SearchingForCube = 1,
    InitialCubeCarry,
    BaseFormed,
    TopBlockCarry,
    PyramidCompleteFlourish
  };
  
  enum class ChooserPhase{
    None, // Only used to force update
    SetupBlocks,
    BuildingPyramid
  };
  
  
  ////////
  /// General Chooser structs/tracking
  ///////
  
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
  bool _lightsShouldMessageCubeOnSide;
  
  
  void UpdateActiveBehaviorGroup(Robot& robot, bool settingUpPyramid);
  
  ////////
  /// General Chooser functions
  ///////
  
  // initialize the chooser, return result of operation
  bool GetCubePropertiesTrackerByID(const ObjectID& id, PyramidCubePropertiesTracker*& lightState);
  bool GetCubePropertiesTrackerByAssignment(const PyramidBehaviorChooser::PyramidAssignment& id,
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
  
  void UpdateTrackerPropertiesBasedOnCurrentRunningBehavior(const IBehavior* currentRunningBehavior);
  
  
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

#endif // __Cozmo_Basestation_BuildPyramidBehaviorChooser_H__
