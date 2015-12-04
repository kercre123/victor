using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace ForwardBackward {

  public class ForwardBackwardGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _LastSelectedId = -1;
    private ForwardBackwardConfig _Config;

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as ForwardBackwardConfig ?? new ForwardBackwardConfig();
    }

    void Start() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("ForwardBackwardStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeForwardBackwardState(_Config.Settings, 0), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CreateDefaultQuitButton();
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    private void InitialCubesDone() {
      
    }

    public int PickCube() {

      // for weird edge case of if batteries die and there's only one cube left.
      if (CurrentRobot.LightCubes.Count == 1) {
        foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
          return lightCube.Key;
        }
      }

      int id = -2;

      do {
        int index = Random.Range(0, CurrentRobot.LightCubes.Count);
        int i = 0;

        foreach (KeyValuePair<int, LightCube> lightCube in CurrentRobot.LightCubes) {
          if (index == i) {
            id = lightCube.Key;
            lightCube.Value.SetLEDs(CozmoPalette.ColorToUInt(Color.blue));
          }
          else {
            lightCube.Value.SetLEDs(0);
          }
          i++;
        }
      } while (id == _LastSelectedId);

      _LastSelectedId = id;

      return id;
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}
