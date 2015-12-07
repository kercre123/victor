using UnityEngine;
using System.Collections;
using System.Linq;

namespace StackTraining {
  public class StackTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private int _BottomCubeId = -1;
    private int _TopCubeId = -1;

    private int _AttemptsLeft;

    public LightCube BottomCube {
      get {
        LightCube cube;
        CurrentRobot.LightCubes.TryGetValue(_BottomCubeId, out cube);
        return cube;
      }
    }

    public LightCube TopCube {
      get {
        LightCube cube;
        CurrentRobot.LightCubes.TryGetValue(_TopCubeId, out cube);
        return cube;
      }
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {

      var config = minigameConfig as StackTrainingConfig ?? new StackTrainingConfig();

      _AttemptsLeft = config.MaxAttempts;

      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("StackTrainingGame", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForStackState(), 2, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CurrentRobot.SetLiftHeight(0f);
      CurrentRobot.SetHeadAngle(0f);
    }

    protected override void InitializeMinigameView(Cozmo.MinigameWidgets.SharedMinigameView minigameView) {
      base.InitializeMinigameView(minigameView);
      minigameView.CreateCozmoStatusWidget(_AttemptsLeft);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {
      foreach (var cube in CurrentRobot.LightCubes) {
        cube.Value.TurnLEDsOff();
      }
    }

    public void PickCubes() {

      if (CurrentRobot.LightCubes.Count < 2) {
        // TODO: Inform player why they lost
        RaiseMiniGameLose();
        return;
      }
        
      _BottomCubeId = CurrentRobot.LightCubes.Keys.First();
      _TopCubeId = CurrentRobot.LightCubes.Keys.Last();
    }

    public bool TryDecrementAttempts() {
      _AttemptsLeft--;
      _SharedMinigameViewInstance.UpdateCozmoAttempts(_AttemptsLeft);

      return (_AttemptsLeft > 0);
    }
  }

}
