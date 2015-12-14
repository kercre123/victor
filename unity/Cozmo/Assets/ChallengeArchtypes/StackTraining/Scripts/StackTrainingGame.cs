using UnityEngine;
using System.Collections;
using System.Linq;

namespace StackTraining {
  public class StackTrainingGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    private int _BottomCubeId = -1;
    private int _TopCubeId = -1;

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

    [SerializeField]
    private string _TutorialSequenceName = "FollowCubeIntro";
    private ScriptedSequences.ISimpleAsyncToken _TutorialSequenceDoneToken;

    protected override void Initialize(MinigameConfigBase minigameConfig) {

      var config = minigameConfig as StackTrainingConfig ?? new StackTrainingConfig();

      MaxAttempts = config.MaxAttempts;

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
      _StateMachineManager.AddStateMachine("StackTrainingGame", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new WaitForStackState(), 2, true, null);
      _StateMachine.SetNextState(initCubeState);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);

      CurrentRobot.SetLiftHeight(0f);
      CurrentRobot.SetHeadAngle(0.15f);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    protected override void CleanUpOnDestroy() {

    }

    public void PickCubes() {

      if (CurrentRobot.LightCubes.Count < 2) {
        // TODO: Inform player why they lost
        RaiseMiniGameLose();
        return;
      }

      foreach (var cube in CurrentRobot.LightCubes) {
        cube.Value.TurnLEDsOff();
        if (cube.Value.MarkersVisible) {
          if (_BottomCubeId == -1) {
            _BottomCubeId = cube.Key;
          }
          else if (_TopCubeId == -1) {
            _TopCubeId = cube.Key;
          }
        }
      }
        
    }
  }

}
