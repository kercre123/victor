using UnityEngine;
using System.Collections.Generic;

namespace CodeBreaker {

  public class CodeBreakerGame : GameBase {

    private const string kHowToPlaySlideName = "HowToPlay";
    private const string kGamePanelSlideName = "ScoreCard";

    private CodeBreakerReadySlide _ReadySlide;
    private CodeBreakerPanel _GamePanelSlide;
    private Color[] _ValidCodeColors;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      DAS.Info(this, "Game Created");

      CodeBreakerGameConfig config = minigameConfig as CodeBreakerGameConfig;
      _ValidCodeColors = new Color[config.ValidCodeColors.Length];
      config.ValidCodeColors.CopyTo(_ValidCodeColors, 0);

      // INGO Potentially we'll need to clamp the #cubes?
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new HowToPlayState(), config.NumCubesInCode, true, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(-1.0f);
    }

    private void InitialCubesDone() {

    }

    protected override void CleanUpOnDestroy() {
      // Don't need to destroy slides because the SharedMinigameView will handle it
    }

    public void ShowHowToPlaySlide(ReadyButtonClickedHandler readyButtonCallback) {
      GameObject howToPlaySlide = ShowHowToPlaySlide(kHowToPlaySlideName);
      _ReadySlide = howToPlaySlide.GetComponent<CodeBreakerReadySlide>();
      if (_ReadySlide != null) {
        if (readyButtonCallback != null) {
          _ReadySlide.OnReadyButtonClicked += readyButtonCallback;
        }
        else {
          DAS.Warn(this, "Tried to attach a null callback to the 'HowToPlay' slide!");
        }
      }
      else {
        DAS.Error(this, "The " + kHowToPlaySlideName + " slide in " + typeof(CodeBreakerGame).ToString()
        + " needs a " + typeof(CodeBreakerReadySlide).ToString() + " component attached to it!");
      }
    }

    public void ShowGamePanel() {
      GameObject gamePanelObject = ShowHowToPlaySlide(kGamePanelSlideName);
      _GamePanelSlide = gamePanelObject.GetComponent<CodeBreakerPanel>();
      if (_GamePanelSlide != null) {
        
      }
      else {
        DAS.Error(this, "The " + kGamePanelSlideName + " slide in " + typeof(CodeBreakerGame).ToString()
        + " needs a " + typeof(CodeBreakerPanel).ToString() + " component attached to it!");
      }
    }

    public void DisableButton() {
      if (_ReadySlide != null) {
        _ReadySlide.EnableButton(false);
      }
      if (_GamePanelSlide != null) {
        // TODO disable submit button
      }
    }

    public void SetButtonText(string text) {
      if (_ReadySlide != null) {
        _ReadySlide.SetButtonText(text);
      }
      if (_GamePanelSlide != null) {
        // TODO disable submit button
      }
    }

    public Color GetRandomColor() {
      return Color.white;
    }
  }

  // Represents a code picked by Cozmo
  public class CubeCode {
    /// <summary>
    /// Colors the cubes should be from left to right according to 
    /// Cozmo. (Index 0 indicates the leftmost cube from Cozmo)
    /// </summary>
    public Color[] cubeColors;
  }
}
