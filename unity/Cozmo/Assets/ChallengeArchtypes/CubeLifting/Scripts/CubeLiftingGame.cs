using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace CubeLifting {

  public class CubeLiftingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _LastSelectedId = -1;
    private CubeLiftingConfig _Config;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as CubeLiftingConfig ?? new CubeLiftingConfig();

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("CubeLiftingStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeUpDownState(_Config.Settings, 0), 1, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CurrentRobot.SetLiftHeight(0);
      CurrentRobot.SetHeadAngle(0);

      MaxAttempts = _Config.MaxAttempts;
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
