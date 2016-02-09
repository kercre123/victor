using UnityEngine;
using System.Collections.Generic;

namespace CodeBreaker {

  public class CodeBreakerGame : GameBase {

    private const string kHowToPlaySlideName = "HowToPlay";
    private const string kGamePanelSlideName = "ScoreCard";

    private CodeBreakerReadySlide _ReadySlide;
    private CodeBreakerPanel _GamePanelSlide;

    [SerializeField]
    private Color[] _ValidCodeColors;

    private CodeBreakerGameConfig _Config;

    public int NumCubesInCode {
      get { return _Config.NumCubesInCode; }
    }

    public Color[] ValidColors {
      get { return _ValidCodeColors; }
    }

    public Color CorrectPosAndColorBackpackColor;
    public Color CorrectColorOnlyBackpackColor;
    public Color NotCorrectColor;

    private int _NumGuessesLeft;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      DAS.Info(this, "Game Created");

      _Config = minigameConfig as CodeBreakerGameConfig;

      // INGO Potentially we'll need to clamp the #cubes?
      // TODO: Potentially show a slide for showing Cozmo a # of cubes; need a consistent solution across games
      InitialCubesState initCubeState = new InitialCubesState(new HowToPlayCodebreakerState(), _Config.NumCubesInCode);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(-1.0f);
    }

    protected override void CleanUpOnDestroy() {
      // Don't need to destroy slides because the SharedMinigameView will handle it
    }

    #region UI

    public void ShowReadySlide(string readySlideTextLocKey, string buttonTextLocKey, ReadyButtonClickedHandler readyButtonCallback) {
      GameObject howToPlaySlide = ShowFullScreenSlide(kHowToPlaySlideName);
      _ReadySlide = howToPlaySlide.GetComponent<CodeBreakerReadySlide>();
      if (_ReadySlide != null) {
        _ReadySlide.SetSlideText(Localization.Get(readySlideTextLocKey));
        _ReadySlide.SetButtonText(Localization.Get(buttonTextLocKey));
        _ReadySlide.EnableButton(true);
        if (readyButtonCallback != null) {
          _ReadySlide.OnReadyButtonClicked += readyButtonCallback;
        }
      }
      else {
        DAS.Error(this, "The " + kHowToPlaySlideName + " slide in " + typeof(CodeBreakerGame).ToString()
        + " needs a " + typeof(CodeBreakerReadySlide).ToString() + " component attached to it!");
      }
    }

    public void ShowGamePanel(SubmitButtonClickedHandler submitButtonCallback) {
      GameObject gamePanelObject = ShowFullScreenSlide(kGamePanelSlideName);
      _GamePanelSlide = gamePanelObject.GetComponent<CodeBreakerPanel>();
      if (_GamePanelSlide != null) {
        _GamePanelSlide.EnableButton = true;
        if (submitButtonCallback != null) {
          _GamePanelSlide.OnSubmitButtonClicked += submitButtonCallback;
        }
        else {
          DAS.Warn(this, "Tried to attach a null callback to the 'HowToPlay' slide!");
        }
      }
      else {
        DAS.Error(this, "The " + kGamePanelSlideName + " slide in " + typeof(CodeBreakerGame).ToString()
        + " needs a " + typeof(CodeBreakerPanel).ToString() + " component attached to it!");
      }
    }

    public void RemoveSubmitButtonListener(SubmitButtonClickedHandler submitButtonCallback) {
      if (_GamePanelSlide != null) {
        if (submitButtonCallback != null) {
          _GamePanelSlide.OnSubmitButtonClicked -= submitButtonCallback;
        }
      }
    }

    public void DisableReadyButton() {
      if (_ReadySlide != null) {
        _ReadySlide.EnableButton(false);
      }
    }

    public bool EnableSubmitButton {
      get { 
        if (_GamePanelSlide != null) {
          return _GamePanelSlide.EnableButton; 
        }
        return false;
      }
      set {
        if (_GamePanelSlide != null) {
          _GamePanelSlide.EnableButton = value; 
        }
      }
    }

    #endregion

    #region Data

    public CubeCode GetRandomCode() {
      CubeCode code = new CubeCode();
      code.cubeColorIndex = new int[_Config.NumCubesInCode];
      for (int i = 0; i < code.cubeColorIndex.Length; i++) {
        code.cubeColorIndex[i] = GetRandomColorIndex();
      }
      return code;
    }

    private int GetRandomColorIndex() {
      int maxColorIndex;
      if (_Config.NumCodeColors <= _ValidCodeColors.Length) {
        maxColorIndex = _Config.NumCodeColors - 1;
      }
      else {
        DAS.Error(this, _Config.name + " is trying to use more code colors than are available! Check the number of " +
        "valid colors on the " + typeof(CodeBreakerGame).ToString() + " prefab!");
        maxColorIndex = _ValidCodeColors.Length - 1;
      }
      int colorIndex = Random.Range(0, maxColorIndex);
      return colorIndex;
    }

    public void UseGuess() {
      _NumGuessesLeft--;
      UpdateGuessesUI();
    }

    public bool AnyGuessesLeft() {
      return _NumGuessesLeft > 0;
    }

    public void ResetGuesses() {
      _NumGuessesLeft = _Config.NumGuesses;
      UpdateGuessesUI();
    }

    public void UpdateGuessesUI() {
      if (_GamePanelSlide != null) {
        _GamePanelSlide.SetGuessesLeft(_NumGuessesLeft);
      }
    }

    #endregion
  }

  // Represents a code picked by Cozmo
  public class CubeCode {
    /// <summary>
    /// Colors the cubes should be from left to right according to 
    /// Cozmo. (Index 0 indicates the leftmost cube from Cozmo)
    /// </summary>
    public int[] cubeColorIndex;

    public int NumCubes {
      get { return cubeColorIndex.Length; } 
      set { }
    }

    public override string ToString() {
      string str = "CubeCode: ";
      for (int i = 0; i < cubeColorIndex.Length; i++) {
        str += i + "(" + cubeColorIndex[i] + ") ";
      }
      return str;
    }
  }

  public class CubeState {
    public LightCube cube;
    public int currentColorIndex;
  }
}
