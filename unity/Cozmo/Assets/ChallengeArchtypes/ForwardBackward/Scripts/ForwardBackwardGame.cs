using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace ForwardBackward {

  public class ForwardBackwardGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private ForwardBackwardConfig _Config;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as ForwardBackwardConfig ?? new ForwardBackwardConfig();
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("ForwardBackwardStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeForwardBackwardState(_Config.Settings, 0), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    private void InitialCubesDone() {
      
    }

    public int PickCube() {

      foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
        lightCube.Value.SetLEDs(0);
      }

      return CurrentRobot.LightCubes.First(x => x.Value.MarkersVisible).Key;
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}
