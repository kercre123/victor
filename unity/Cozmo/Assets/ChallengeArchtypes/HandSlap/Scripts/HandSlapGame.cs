using UnityEngine;
using System.Collections;

namespace HandSlap {
  
  public class HandSlapGame : GameBase {
    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _SuccessGoal;

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      HandSlapConfig config = minigameConfig as HandSlapConfig;
      MaxAttempts = config.MaxAttempts;
      _SuccessGoal = config.SuccessGoal;
      _MinSlapDelay = config.MinSlapDelay;
      _MaxSlapDelay = config.MaxSlapDelay;
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("HandSlapStateMachine", _StateMachine);

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);

      _StateMachine.SetNextState(new SeekState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {
      
    }
  }
}
