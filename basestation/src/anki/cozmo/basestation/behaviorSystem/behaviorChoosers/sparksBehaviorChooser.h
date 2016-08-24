/**
 * File: sparksBehaviorChooser.h
 *
 * Author: Kevin M. Karol
 * Created: 08/17/16
 *
 * Description: Class for handling playing sparks.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_SparksBehaviorChooser_H__
#define __Cozmo_Basestation_SparksBehaviorChooser_H__

#include "simpleBehaviorChooser.h"
#include "clad/types/behaviorObjectives.h"

namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {
  
//Forward declarations
class IBehavior;
class MoodManager;
class Robot;
class BehaviorPlayArbitraryAnim;
template <typename Type> class AnkiEvent;
  
// A behavior chooser which handles soft sparking, intro/outro and relies on simple behavior chooser otherwise
class SparksBehaviorChooser : public SimpleBehaviorChooser
{
using BaseClass = SimpleBehaviorChooser;
  
public:

  // constructor/destructor
  SparksBehaviorChooser(Robot& robot, const Json::Value& config);
  virtual ~SparksBehaviorChooser();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehaviorChooser API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehavior* ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior) override;
  
  // name (for debug/identification)
  virtual const char* GetName() const override { return "Sparks"; }
  
  virtual void OnSelected() override;
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  // initialize the chooser, return result of operation
  Result ReloadFromConfig(Robot& robot, const Json::Value& config);

private:  
  enum class ChooserState{
    ChooserSelected,
    PlayingSparksIntro,
    UsingSimpleBehaviorChooser,
    WaitingForCurrentBehaviorToStop,
    PlayingSparksOutro
  };
  
  ChooserState _state;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // Created with factory
  BehaviorPlayArbitraryAnim* _behaviorPlayAnimation = nullptr;
  
  // Re-set each time spark is activated
  TimeStamp_t _timeChooserStarted;
  int _currentObjectiveCompletedCount;

  // Loaded in from behavior_config
  float _minTimeSecs;
  float _maxTimeSecs;
  int _numberOfRepetitions;
  BehaviorObjective _objectiveToListenFor;
  AnimationTrigger _softSparkUpgradeTrigger;
  
  // Special re-start indicator
  TimeStamp_t _timePlayingOutroStarted;
  bool _switchingSoftToHardSpark;
  
};
   
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_SparksBehaviorChooser_H__
