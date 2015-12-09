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
  }

  public delegate void MiniGameWinHandler();

  public event MiniGameWinHandler OnMiniGameWin;

  public void RaiseMiniGameWin() {
    if (OnMiniGameWin != null) {
      OnMiniGameWin();
    }
  }

  public delegate void MiniGameLoseHandler();

  public event MiniGameWinHandler OnMiniGameLose;

  public void RaiseMiniGameLose() {
    if (OnMiniGameLose != null) {
      OnMiniGameLose();
    }
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private SharedMinigameView _SharedMinigameViewInstance;

  public void InitializeMinigame(ChallengeData challengeData) {
    GameObject minigameViewObj = UIManager.CreateUIElement(UIPrefabHolder.Instance.SharedMinigameViewPrefab.gameObject);
    _SharedMinigameViewInstance = minigameViewObj.GetComponent<SharedMinigameView>();

    // For all challenges, set the title text and add a quit button
    TitleText = Localization.Get(challengeData.ChallengeTitleLocKey);
    CreateDefaultQuitButton();

    Initialize(challengeData.MinigameConfig);

    // Populate the view before opening it so that animations play correctly
    _SharedMinigameViewInstance.OpenView();
  }

  protected abstract void Initialize(MinigameConfigBase minigameConfigData);

  public void OnDestroy() {
    CleanUpOnDestroy();
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
    _SharedMinigameViewInstance.EnableInteractivity();
  }

  public void CloseMinigame() {
    _SharedMinigameViewInstance.ViewCloseAnimationFinished += HandleMinigameViewCloseFinished;
    _SharedMinigameViewInstance.CloseView();
    _SharedMinigameViewInstance = null;
  }

  public void CloseMinigameImmediately() {
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    HandleMinigameViewCloseFinished();
  }

  private void HandleMinigameViewCloseFinished() {
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
}