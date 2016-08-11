/**
 * File: demoBehaviorChooser.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-25
 *
 * Description: This is a chooser for the PR / announce demo. It handles enabling / disabling behavior groups,
 *              as well as any other special logic that is needed for the demo
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_DemoBehaviorChooser_H__
#define __Cozmo_Basestation_DemoBehaviorChooser_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/simpleBehaviorChooser.h"
#include "json/json-forwards.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {

class BehaviorAdmireStack;
class BehaviorDemoFearEdge;
class BlockWorldFilter;
class ObservableObject;
class Robot;

class DemoBehaviorChooser : public SimpleBehaviorChooser
{
  using super = SimpleBehaviorChooser;
public:

  // This disabled all behaviors from running, regardless of the config.
  DemoBehaviorChooser(Robot& robot, const Json::Value& config);

  virtual void OnSelected() override;

  virtual Result Update() override;

  virtual IBehavior* ChooseNextBehavior(Robot& robot, bool didCurrentFinish) const override;

  virtual const char* GetName() const override { return _name.c_str(); }

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
  enum class State : uint8_t {
    None,
    WakeUp,
    DriveOffCharger,
    FearEdge,
    Pounce,
    Faces,
    Cubes,
    MiniGame,
    Sleep
  };

protected:

  virtual void ModifyScore(const IBehavior* behavior, float& score) const override;

private:

  void TransitionToNextState();

  void TransitionToWakeUp();
  void TransitionToDriveOffCharger();
  void TransitionToFearEdge();
  void TransitionToPounce();
  void TransitionToFaces();
  void TransitionToCubes();
  void TransitionToMiniGame();
  void TransitionToSleep();

  bool ShouldTransitionOutOfCubesState();

  bool FilterBlocks(const ObservableObject* obj) const;
  
  // This will be called from Update, and if it returns true, we will transition to the next state
  std::function<bool(void)> _checkTransition;

  bool DidBehaviorRunAndStop(const char* behaviorName) const;
  bool IsBehaviorRunning(const char* behaviorName) const;
  void ResetDemoRelatedState();

  State _state = State::None;

  // for the UI to know which state, we need to convert state to an int
  static unsigned int GetUISceneNumForState(State state);

  std::string _name;

  void SetState_internal(State state, const std::string& stateName);

  bool _hasEdge = true;
  bool _hasSeenBlock = false;

  IBehavior* _faceSearchBehavior = nullptr;
  IBehavior* _gameRequestBehavior = nullptr;
  BehaviorDemoFearEdge* _fearEdgeBehavior = nullptr;
  BehaviorAdmireStack* _admireStackBehavior = nullptr;

  // behaviors to "encourage" by boosting their score
  std::map< const IBehavior*, float > _modifiedBehaviorScores;
  
  // tracking for the cubes to determine when there are three that are upright (and have been for some time)
  float _cubesUprightTime_s = -1.0f;
  // true if the cube is upright and (nearly) level and on the ground. Set to false if it moves or is observed
  // not upright or level
  std::set< ObjectID > _uprightCubes;

  std::vector<Signal::SmartHandle> _signalHandles;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilter;
    
  Robot& _robot;
};

}
}


#endif
