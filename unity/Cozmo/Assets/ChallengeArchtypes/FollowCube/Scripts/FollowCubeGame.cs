using UnityEngine;
using System.Collections;

namespace FollowCube {

  public class FollowCubeGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public float NotSeenForgivenessThreshold = 2f;

    [SerializeField]
    private string _TutorialSequenceName = "FollowCubeIntro";
    private ScriptedSequences.ISimpleAsyncToken _TutorialSequenceDoneToken;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      MaxAttempts = 7;
      AttemptsLeft = 7;

      Progress = 0.0f;
      NumSegments = 5;

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

    public enum FollowTask {
      Forwards,
      Backwards,
      TurnLeft,
      TurnRight,
      FollowDrive
    }

    public FollowTask CurrentFollowTask;

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new FollowCubeForwardState(), 1, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      ShowHowToPlaySlide("ShowCubeVision");

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);

    }

    protected override void CleanUpOnDestroy() {

    }

    public void FailedAttempt() {
      AttemptsLeft--;
      if (AttemptsLeft == 0) {
        (_StateMachine.GetGame() as FollowCubeGame).RaiseMiniGameLose();
      }
    }

    // Update is called once per frame
    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    void InitialCubesDone() {
      SetSpeed();
      CurrentFollowTask = FollowTask.Forwards;
    }

    void SetSpeed() {
      ForwardSpeed = 30.0f;
      DistanceMax = 130.0f;
      DistanceMin = 90.0f;
    }

  }

}
