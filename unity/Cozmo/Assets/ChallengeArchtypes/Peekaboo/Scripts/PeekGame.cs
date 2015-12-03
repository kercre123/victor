using UnityEngine;
using System.Collections;

namespace Peekaboo {
  public class PeekGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _PeekSuccessCount = 0;
    private int _PeekGoalTarget  = 3;

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfigData) {
      PeekGameConfig config = (minigameConfigData as PeekGameConfig);
      _PeekGoalTarget = config.goal;
    }

    public void PeekSuccess() {
      _PeekSuccessCount++;
      if (_PeekSuccessCount >= _PeekGoalTarget) {
        RaiseMiniGameWin();
      }
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      _StateMachine.SetNextState(new LookingForFaceState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, false);
      SetSpeed();
      CreateDefaultQuitButton();
    }

    void SetSpeed() {
      ForwardSpeed = 20.0f;
      DistanceMax = 130.0f;
      DistanceMin = 90.0f;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }


    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }
}
