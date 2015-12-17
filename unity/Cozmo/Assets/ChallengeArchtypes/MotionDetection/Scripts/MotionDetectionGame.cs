using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace MotionDetection {

  public class MotionDetectionGame : GameBase {

    private MotionDetectionConfig _Config;

    [SerializeField]
    private string _TutorialSequenceName;

    private ScriptedSequences.ISimpleAsyncToken _TutorialSequenceDoneToken;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      _Config = minigameConfig as MotionDetectionConfig ?? new MotionDetectionConfig();
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
      ShowHowToPlaySlide("MotionDetectionHelp01");
      _StateMachine.SetNextState(new RecognizeMotionState(_Config.TimeAllowedBetweenWaves, _Config.TotalWaveTime));
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, true);
    }

    protected override void CleanUpOnDestroy() {
      ScriptedSequences.ScriptedSequence sequence = ScriptedSequences.ScriptedSequenceManager.Instance.Sequences.Find(s => s.Name == _TutorialSequenceName);
      sequence.ResetSequence();
    }
  }

}
