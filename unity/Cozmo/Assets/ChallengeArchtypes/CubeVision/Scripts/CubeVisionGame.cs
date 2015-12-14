using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace VisionTraining {

  public class CubeVisionGame : GameBase {

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private float _LastTimeTargetSeen;
    private LightCube _CurrentTarget = null;

    [SerializeField]
    private string _TutorialSequenceName = "CubeVisionIntro";
    private ScriptedSequences.ISimpleAsyncToken _TutorialSequenceDoneToken;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _LastTimeTargetSeen = Time.time;
      AttemptsLeft = 5;
      MaxAttempts = 5;

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
      _StateMachineManager.AddStateMachine("FollowCubeStateMachine", _StateMachine);
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new RecognizeCubeState(), 1, true, null);
      _StateMachine.SetNextState(initCubeState);
      ShowHowToPlaySlide("ShowCubeVision");
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      if (_CurrentTarget == null) {
        _CurrentTarget = FindNewTarget();
      }

      if (CurrentRobot.VisibleObjects.Contains(_CurrentTarget)) {
        _LastTimeTargetSeen = Time.time;
      }

      if (Time.time - _LastTimeTargetSeen > 2.0f) {
        _CurrentTarget = null;
      }
    }

    public LightCube CurrentTarget() {
      return _CurrentTarget;
    }

    private LightCube FindNewTarget() {
      for (int i = 0; i < CurrentRobot.VisibleObjects.Count; ++i) {
        if (CurrentRobot.VisibleObjects[i] is LightCube) {
          return CurrentRobot.VisibleObjects[i] as LightCube;
        }
      }
      return null;
    }

    public void LostCube() {
      AttemptsLeft--;
      if (AttemptsLeft == 0) {
        RaiseMiniGameLose();
      }
    }

    protected override void CleanUpOnDestroy() {
    }
  }

}
