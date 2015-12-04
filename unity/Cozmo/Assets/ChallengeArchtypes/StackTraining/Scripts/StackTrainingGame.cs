using UnityEngine;
using System.Collections;

namespace StackTraining {
  public class StackTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();


    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {

    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("StackTrainingGame", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForStackState(), 1, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}
