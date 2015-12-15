using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace CubeLifting {

  public class CubeLiftingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private CubeLiftingConfig _Config;

    [SerializeField]
    private string _TutorialSequenceName = "CubeLiftingIntro";
    private ScriptedSequences.ISimpleAsyncToken _TutorialSequenceDoneToken;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as CubeLiftingConfig ?? new CubeLiftingConfig();

      MaxAttempts = _Config.MaxAttempts;

      if (!string.IsNullOrEmpty(_TutorialSequenceName)) {
        _TutorialSequenceDoneToken = ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence(_TutorialSequenceName);
        _TutorialSequenceDoneToken.Ready(HandleTutorialSequenceDone);
      }
      else {
        HandleTutorialSequenceDone(null);
      }
    }

    private void HandleTutorialSequenceDone(ScriptedSequences.ISimpleAsyncToken token) {
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("CubeLiftingStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeUpDownState(_Config.Settings, 0), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

      CurrentRobot.SetLiftHeight(0);
      CurrentRobot.SetHeadAngle(0);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    private void InitialCubesDone() {
      
    }

    public void LoseCubeSight() {
      AttemptsLeft--;
      if (AttemptsLeft == 0) {
        RaiseMiniGameLose();
      }
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
