using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace AskCozmo {

  public class AskCozmoGame : GameBase {

    private bool _AnimationPlaying = false;

    [SerializeField]
    private AskCozmoPanel _GamePanelPrefab;

    private AskCozmoPanel _GamePanel;

    private AskCozmoConfig _Config;

    private string _CurrentSlideName = null;

    private int _AttemptsLeft;

    // From 0 to 1
    private float _Progress = 0f;

    protected void InitializeMinigameObjects() {
      _GamePanel = UIManager.OpenView(_GamePanelPrefab);
      _GamePanel.OnAskButtonPressed += OnAnswerRequested;
      // ScriptedSequences.ScriptedSequenceManager.Instance.ActivateSequence("AzkCozmoSequence");
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      
      var askCozmoConfig = minigameConfig as AskCozmoConfig;

      if (askCozmoConfig == null) {
        DAS.Error(this, "Failed to load config as AskCozmoConfig!");
        // TODO: Handle this error
        return;
      }
      _Config = askCozmoConfig;

      DAS.Debug(this, _Config.Param1);
      DAS.Debug(this, _Config.Param2);

      for (int i = 0; i < _Config.ParamList.Length; ++i) {
        DAS.Debug(this, _Config.ParamList[i].ToString());
      }
      InitializeMinigameObjects();

      _AttemptsLeft = 3;
      Cozmo.MinigameWidgets.CozmoStatusWidget statusWidget = SharedMinigameView.AttemptBar;
      statusWidget.MaxAttempts = _AttemptsLeft;
      statusWidget.AttemptsLeft = _AttemptsLeft;

      _Progress = 0.5f;
      SharedMinigameView.ProgressBar.NumSegments = 4;

      // By default says "Challenge Progress"
      // ProgressBarLabelText = Localization.Get(keyNameHere);

      // Play a slide
      _CurrentSlideName = GetNextSlide();
    }

    protected override void CleanUpOnDestroy() {
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
    }

    // user just asked the question and pressed the
    // give me answer button.
    public void OnAnswerRequested() {
      if (_AnimationPlaying) {
        return;
      }

      _AnimationPlaying = true;
      if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.5f) {
        CurrentRobot.SendAnimation(AnimationName.kMajorWin, HandleAnimationDone);
        _Progress += 0.25f;
        if (_Progress >= 1) {
          RaiseMiniGameWin("Everybody is a winner");
        }
      }
      else {
        CurrentRobot.SendAnimation(AnimationName.kShocked, HandleAnimationDone);
        _Progress -= 0.25f;
      }

      _AttemptsLeft--;
      SharedMinigameView.AttemptBar.AttemptsLeft = _AttemptsLeft;
      if (_AttemptsLeft <= 0) {
        RaiseMiniGameLose("Everybody is a loser");
      }

      _CurrentSlideName = GetNextSlide();
    }

    void HandleAnimationDone(bool success) {
      _AnimationPlaying = false;
    }

    private string GetNextSlide() {
      if (_CurrentSlideName == null || _CurrentSlideName == "Slide2") {
        return "Slide1";
      }
      return "Slide2";
    }
  }

}
