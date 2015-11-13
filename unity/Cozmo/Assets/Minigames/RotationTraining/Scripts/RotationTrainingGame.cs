using UnityEngine;
using System.Collections;

namespace RotationTraining {
  public class RotationTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();

      // TODO: Swap countdown state for rotate to prep player.

      initCubeState.InitialCubeRequirements(new RotateState(), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      CreateDefaultQuitButton();
    }
	
    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
    
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }
}