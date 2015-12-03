using UnityEngine;
using System.Collections;

namespace FollowCubeRotate {
  public class FollowCubeRotateGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private int _Lives = 3;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {

    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("ForwardBackwardStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForNewCube(), 1, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    public void LoseLife() {
      if (_Lives > 0) {
        _Lives--;
      }
      else {
        RaiseMiniGameLose();
      }
    }

    public LightCube FindNewCubeTarget() {
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
