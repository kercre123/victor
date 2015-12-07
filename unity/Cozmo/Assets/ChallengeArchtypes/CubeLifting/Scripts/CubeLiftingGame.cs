using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace CubeLifting {

  public class CubeLiftingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private CubeLiftingConfig _Config;
    private int _AttemptsLeft;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as CubeLiftingConfig ?? new CubeLiftingConfig();

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("CubeLiftingStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeUpDownState(_Config.Settings, 0), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CurrentRobot.SetLiftHeight(0);
      CurrentRobot.SetHeadAngle(0);

      _AttemptsLeft = _Config.MaxAttempts;
    }

    protected override void InitializeMinigameView(Cozmo.MinigameWidgets.SharedMinigameView minigameView) {
      base.InitializeMinigameView(minigameView);
      minigameView.CreateCozmoStatusWidget(_AttemptsLeft);
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

    public bool TryDecrementAttempts() {
      _AttemptsLeft--;
      _SharedMinigameViewInstance.UpdateCozmoAttempts(_AttemptsLeft);

      return (_AttemptsLeft > 0);
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}
