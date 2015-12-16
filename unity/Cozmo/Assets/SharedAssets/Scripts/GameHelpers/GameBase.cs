using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.MinigameWidgets;
using DG.Tweening;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  protected void RaiseMiniGameQuit() {
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }

    CloseMinigameImmediately();
  }

  public delegate void MiniGameWinHandler();

  public event MiniGameWinHandler OnMiniGameWin;

  public void RaiseMiniGameWin() {
    _WonChallenge = true;
    OpenChallengeEndedDialog();
  }

  public delegate void MiniGameLoseHandler();

  public event MiniGameWinHandler OnMiniGameLose;

  public void RaiseMiniGameLose() {
    _WonChallenge = false;
    OpenChallengeEndedDialog();
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private SharedMinigameView _SharedMinigameViewInstance;

  protected ChallengeData _ChallengeData;
  private AlertView _ChallengeEndViewInstance;
  private bool _WonChallenge;

  [SerializeField]
  protected HowToPlaySlide[] _HowToPlayPrefabs;

  public void InitializeMinigame(ChallengeData challengeData) {
    _SharedMinigameViewInstance = UIManager.OpenView(
      UIPrefabHolder.Instance.SharedMinigameViewPrefab, 
      false) as SharedMinigameView;

    _ChallengeData = challengeData;
    _WonChallenge = false;
    Initialize(challengeData.MinigameConfig);

    // Populate the view before opening it so that animations play correctly
    InitializeView(challengeData);
    _SharedMinigameViewInstance.OpenView();
  }

  protected abstract void Initialize(MinigameConfigBase minigameConfigData);

  protected virtual void InitializeView(ChallengeData data) {
    // For all challenges, set the title text and add a quit button by default
    TitleText = Localization.Get(data.ChallengeTitleLocKey);
    CreateDefaultQuitButton();
  }

  public void OnDestroy() {
    CleanUpOnDestroy();
    if (CurrentRobot != null) {
      CurrentRobot.ResetRobotState();
    }
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    if (_ChallengeEndViewInstance != null) {
      _ChallengeEndViewInstance.CloseViewImmediately();
      _ChallengeEndViewInstance = null;
    }
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  protected virtual void PauseGame() {
    _SharedMinigameViewInstance.DisableInteractivity();
  }

  protected virtual void ResumeGame() {
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.EnableInteractivity();
    }
  }

  public void CloseMinigameImmediately() {
    Destroy(gameObject);
  }

  private AlertView OpenChallengeEndedDialog() {
    // Open confirmation dialog instead
    AlertView alertView = UIManager.OpenView(UIPrefabHolder.Instance.ChallengeEndViewPrefab) as AlertView;
    // Hook up callbacks
    alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue, HandleChallengeResultViewClosed);
    alertView.TitleLocKey = _ChallengeData.ChallengeTitleLocKey;
    alertView.DescriptionLocKey = _WonChallenge ? LocalizationKeys.kLabelChallengeCompleted : LocalizationKeys.kLabelChallengeFailed;
    // Listen for dialog close
    alertView.ViewCloseAnimationFinished += HandleQuitViewClosed;
    return alertView;
  }

  private void HandleChallengeResultViewClosed() {
    if (_WonChallenge) {
      if (OnMiniGameWin != null) {
        OnMiniGameWin();
      }
    }
    else {
      if (OnMiniGameLose != null) {
        OnMiniGameLose();
      }
    }
    Destroy(gameObject);
  }

  #region Default Quit button

  protected void CreateDefaultQuitButton() {
    _SharedMinigameViewInstance.CreateQuitButton();
    _SharedMinigameViewInstance.QuitMiniGameViewOpened += HandleQuitViewOpened;
    _SharedMinigameViewInstance.QuitMiniGameViewClosed += HandleQuitViewClosed;
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;
  }

  private void HandleQuitViewOpened() {
    PauseGame();
  }

  private void HandleQuitViewClosed() {
    ResumeGame();
  }

  private void HandleQuitConfirmed() {
    RaiseMiniGameQuit();
  }

  #endregion

  #region Attempts Bar

  private int _MaxAttempts = -1;

  public int MaxAttempts {
    get { return _MaxAttempts; }
    set {
      if (value > 0) {
        _MaxAttempts = value;
        _SharedMinigameViewInstance.SetMaxCozmoAttempts(_MaxAttempts);
        AttemptsLeft = _MaxAttempts;
      }
      else {
        DAS.Error(this, "Tried to set MaxAttempts to a negative int! Aborting!");
      }
    }
  }

  private int _AttemptsLeft = -1;

  public int AttemptsLeft {
    get { return _AttemptsLeft; }
    set {
      _AttemptsLeft = Mathf.Clamp(value, 0, MaxAttempts);
      _AttemptsLeft = value;
      _SharedMinigameViewInstance.SetCozmoAttemptsLeft(_AttemptsLeft);
    }
  }

  public bool TryDecrementAttempts() {
    AttemptsLeft--;

    return (AttemptsLeft > 0);
  }

  #endregion

  #region Task Progress Bar

  // From 0 to 1
  private float _Progress = 0f;

  public float Progress {
    get { return _Progress; }
    set {
      _Progress = Mathf.Clamp(value, 0f, 1f);
      _Progress = value;
      _SharedMinigameViewInstance.SetProgress(_Progress);
    }
  }

  // By default says "Challenge Progress"
  protected string ProgressBarLabelText {
    get { 
      return _SharedMinigameViewInstance.ProgressBarLabelText; 
    }
    set {
      _SharedMinigameViewInstance.ProgressBarLabelText = value;
    }
  }

  // Add some decorative lines to the bar to demark the number
  // "segments" there are in the bar. Progress is still from 0 to 1 overall,
  // so if you want to fill the first segment of a 4 segment bar, set
  // Progress to 0.25.
  public int NumSegments {
    get {
      return _SharedMinigameViewInstance.NumSegments;
    }
    set {
      _SharedMinigameViewInstance.NumSegments = value;
    }
  }

  #endregion

  #region Title Widget

  protected string TitleText {
    get {
      return _SharedMinigameViewInstance.TitleText;
    }
    set {
      _SharedMinigameViewInstance.TitleText = value;
    }
  }

  #endregion

  #region How To Play Slides

  public void ShowHowToPlaySlide(string slideName) {
    // Search through the array for a slide of the same name
    HowToPlaySlide foundSlide = null;
    foreach (var slide in _HowToPlayPrefabs) {
      if (slide != null && slide.slideName == slideName) {
        foundSlide = slide;
        break;
      }
      else if (slide == null) {
        DAS.Warn(this, "Null slide found in slide prefabs! Check this object's" +
        " list of slides! gameObject.name=" + gameObject.name);
      }
    }

    // If found, show that slide.
    if (foundSlide != null) {
      if (foundSlide.slidePrefab != null) {
        _SharedMinigameViewInstance.ShowHowToPlaySlide(foundSlide);
      }
      else {
        DAS.Error(this, "Null prefab for slide with name '" + slideName + "'! Check this object's" +
        " list of slides! gameObject.name=" + gameObject.name);
      }
    }
    else {
      DAS.Error(this, "Could not find slide with name '" + slideName + "' in slide prefabs! Check this object's" +
      " list of slides! gameObject.name=" + gameObject.name);
    }
  }

  #endregion
}

[System.Serializable]
public class HowToPlaySlide {
  public string slideName;
  public CanvasGroup slidePrefab;
}