using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class FollowCubeRotateGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {

    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("ForwardBackwardStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new RotateWithTarget(), 1, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    public LightCube LightCubeTarget() {
      foreach (ObservedObject visibleObj in CurrentRobot.VisibleObjects) {
        if (visibleObj is LightCube) {
          return (LightCube)visibleObj;
        }
      }
      return null;
    }

    public override void CleanUp() {
      DestroyDefaultQuitButton();
    }
  }

}
