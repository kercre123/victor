using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.MinigameWidgets;
using DG.Tweening;
using Anki.Cozmo;
using System.Linq;
using Cozmo.Util;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  protected void RaiseMiniGameQuit() {
    _StateMachine.Stop();

    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }

    CloseMinigameImmediately();
  }

  public delegate void MiniGameWinHandler(StatContainer rewardedXp);

  public event MiniGameWinHandler OnMiniGameWin;

  public void RaiseMiniGameWin(string primaryTextOverride = "", string secondaryTextOverride = "") {
    _StateMachine.Stop();

    _WonChallenge = true;
    string primaryText = primaryTextOverride;
    if (string.IsNullOrEmpty(primaryText)) {
      primaryText = Localization.Get(LocalizationKeys.kMinigameTextPlayerWins);
    }
    _SharedMinigameViewInstance.ShowCozmoScoreWidget();
    _SharedMinigameViewInstance.ShowPlayerWinnerBanner();
    OpenChallengeEndedDialog(primaryText, secondaryTextOverride);
  }

  public delegate void MiniGameLoseHandler(StatContainer rewardedXp);

  public event MiniGameWinHandler OnMiniGameLose;

  public void RaiseMiniGameLose(string primaryTextOverride = "", string secondaryTextOverride = "") {
    _StateMachine.Stop();
    _WonChallenge = false;
    string primaryText = primaryTextOverride;
    if (string.IsNullOrEmpty(primaryText)) {
      primaryText = Localization.Get(LocalizationKeys.kMinigameTextCozmoWins);
    }
    _SharedMinigameViewInstance.ShowCozmoWinnerBanner();
    _SharedMinigameViewInstance.ShowPlayerScoreWidget();
    OpenChallengeEndedDialog(primaryText, secondaryTextOverride);
  }

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private SharedMinigameView _SharedMinigameViewInstance;

  protected Transform SharedMinigameViewInstanceParent { get { return _SharedMinigameViewInstance.transform; } }

  protected ChallengeData _ChallengeData;
  private ChallengeEndedDialog _ChallengeEndViewInstance;
  private bool _WonChallenge;

  protected StateMachine _StateMachine = new StateMachine();

  [SerializeField]
  protected GameStateSlide[] _GameStateSlides;

  private StatContainer _RewardedXp;

  private float _GameStartTime;

  public void InitializeMinigame(ChallengeData challengeData) {
    _GameStartTime = Time.time;
    _StateMachine.SetGameRef(this);
    _SharedMinigameViewInstance = UIManager.OpenView(
      UIPrefabHolder.Instance.SharedMinigameViewPrefab, 
      false) as SharedMinigameView;

    _ChallengeData = challengeData;
    _WonChallenge = false;
 
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MUSIC.PLAYFUL);
    Initialize(challengeData.MinigameConfig);

    // Populate the view before opening it so that animations play correctly
    InitializeView(challengeData);
    _SharedMinigameViewInstance.OpenView();
  }

  protected abstract void Initialize(MinigameConfigBase minigameConfigData);

  protected virtual void InitializeView(ChallengeData data) {
    // For all challenges, set the title text and add a quit button by default
    ShowTitleWidget(Localization.Get(data.ChallengeTitleLocKey), data.ChallengeIcon);
    CreateDefaultBackButton();
  }

  protected virtual int CalculateTimeStatRewards() {
    return Mathf.CeilToInt((Time.time - _GameStartTime) / 30.0f);
  }

  protected virtual int CalculateNoveltyStatRewards() {
    const int maxPoints = 5;

    // sessions are in chronological order, completed challenges are as well.
    // using Reversed gets them in reverse chronological order
    var completedChallenges = 
      DataPersistence.DataPersistenceManager.Instance.Data.Sessions
          .Reversed().SelectMany(x => x.CompletedChallenges.Reversed());

    int noveltyPoints = 0;
    bool found = false;
    foreach (var challenge in completedChallenges) {
      if (challenge.ChallengeId == this._ChallengeData.ChallengeID || noveltyPoints == maxPoints) {
        found = true;
        break;
      }
      noveltyPoints++;
    }
    return found ? noveltyPoints : maxPoints;
  }

  // should be override for each mini game that wants to grant excitement rewards.
  protected virtual int CalculateExcitementStatRewards() {
    return 0;
  }

  public void OnDestroy() {
    if (CurrentRobot != null) {
      CurrentRobot.ResetRobotState(() => {
        RobotEngineManager.Instance.CurrentRobot.SetIdleAnimation(AnimationName.kIdleBrickout);
      });
    }
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    if (_ChallengeEndViewInstance != null) {
      _ChallengeEndViewInstance.ViewCloseAnimationFinished -= HandleChallengeResultViewClosed;
      _ChallengeEndViewInstance.CloseViewImmediately();
      _ChallengeEndViewInstance = null;
    }
  }

  protected virtual void Update() {
    UpdateStateMachine();
  }

  protected virtual void UpdateStateMachine() {
    _StateMachine.UpdateStateMachine();
  }

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  public void CloseMinigameImmediately() {
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MUSIC.SILENCE);
    CleanUpOnDestroy();
    Destroy(gameObject);
  }

  private int ComputeXpForStat(Anki.Cozmo.ProgressionStatType statType) {
    switch (statType) {
    case Anki.Cozmo.ProgressionStatType.Time:
      return CalculateTimeStatRewards();
    case Anki.Cozmo.ProgressionStatType.Novelty:
      return CalculateNoveltyStatRewards();
    case Anki.Cozmo.ProgressionStatType.Excitement:
      return CalculateExcitementStatRewards();
    default: 
      return 0;
    }
  }

  private void OpenChallengeEndedDialog(string primaryText, string secondaryText) {
    // Open confirmation dialog instead
    _ChallengeEndViewInstance = UIManager.OpenView(UIPrefabHolder.Instance.ChallengeEndViewPrefab) as ChallengeEndedDialog;
    _ChallengeEndViewInstance.SetupDialog(Localization.Get(_ChallengeData.ChallengeTitleLocKey),
      _ChallengeData.ChallengeIcon, primaryText, secondaryText);
    // Listen for dialog close
    _ChallengeEndViewInstance.ViewCloseAnimationFinished += HandleChallengeResultViewClosed;

    _RewardedXp = new StatContainer();

    foreach (var statType in StatContainer.sKeys) {
      // Check that this is a goal xp
      if (DailyGoalManager.Instance.HasGoalForStat(statType)) {
        int grantedXp = ComputeXpForStat(statType);
        if (grantedXp != 0) {
          _RewardedXp[statType] = grantedXp;
          _ChallengeEndViewInstance.AddReward(statType, grantedXp);

          // Grant right away even if there are animations in the daily goal ui
          CurrentRobot.AddToProgressionStat(statType, grantedXp);
        }
      }
    }
  }

  private void HandleChallengeResultViewClosed() {
    if (_WonChallenge) {
      if (OnMiniGameWin != null) {
        OnMiniGameWin(_RewardedXp);
      }
    }
    else {
      if (OnMiniGameLose != null) {
        OnMiniGameLose(_RewardedXp);
      }
    }
    CloseMinigameImmediately();
  }

  #region Default Quit button

  protected void CreateDefaultBackButton() {
    _SharedMinigameViewInstance.CreateQuickQuitButton();
    _SharedMinigameViewInstance.QuitMiniGameConfirmed -= HandleQuitConfirmed;
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;
  }

  public void HideDefaultBackButton() {
    _SharedMinigameViewInstance.HideQuickQuitButton();
  }

  public void CreateDefaultQuitButton() {
    _SharedMinigameViewInstance.CreateQuitButton();
    _SharedMinigameViewInstance.QuitMiniGameConfirmed -= HandleQuitConfirmed;
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;
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
      if (value < 0 || value > 1) {
        DAS.Warn(this, "Tried to set progress to value=" + value + " which is not in the range of 0 to 1! Clamping.");
        _Progress = Mathf.Clamp(value, 0f, 1f);
      }
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

  protected void ShowTitleWidget(string titleText, Sprite titleIcon) {
    _SharedMinigameViewInstance.CreateTitleWidget(titleText, titleIcon);
  }

  #endregion

  #region How To Play Button

  public void OpenHowToPlayView() {
    _SharedMinigameViewInstance.CreateHowToPlayButton(_ChallengeData.HowToPlayDialogContentPrefab);
    _SharedMinigameViewInstance.OpenHowToPlayView();
  }

  public void CloseHowToPlayView() {
    _SharedMinigameViewInstance.CloseHowToPlayView();
  }

  #endregion

  #region Difficulty Select

  public void OpenDifficultySelectView(List<DifficultySelectOptionData> options, int highestDifficultyAvailable, System.Action<DifficultySelectOptionData> onSelect) {
    _SharedMinigameViewInstance.CreateDifficultySelectView(options, highestDifficultyAvailable, onSelect);
    _SharedMinigameViewInstance.OpenDifficultySelectView();
  }

  public void CloseDifficultySelectView() {
    _SharedMinigameViewInstance.CloseDifficultySelectView();
  }

  #endregion

  #region Game State Slides

  public ShowCozmoCubeSlide ShowShowCozmoCubesSlide(int numCubesRequired) {
    GameObject slideObject = _SharedMinigameViewInstance.ShowFullScreenSlide(UIPrefabHolder.Instance.InitialCubesSlide);
    ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
    cubeSlide.Initialize(numCubesRequired);
    return cubeSlide;
  }

  /// <summary>
  /// Returns an instance of the slide created. Null if the creation failed.
  /// </summary>]
  public GameObject ShowGameStateSlide(string slideName) {
    // Search through the array for a slide of the same name
    GameObject slideObject = null;
    GameStateSlide foundSlideData = null;
    foreach (var slide in _GameStateSlides) {
      if (slide != null && slide.slideName == slideName) {
        foundSlideData = slide;
        break;
      }
      else if (slide == null) {
        DAS.Warn(this, "Null slide found in slide prefabs! Check this object's" +
        " list of slides! gameObject.name=" + gameObject.name);
      }
    }

    // If found, show that slide.
    if (foundSlideData != null) {
      if (foundSlideData.slidePrefab != null) {
        slideObject = _SharedMinigameViewInstance.ShowGameStateSlide(foundSlideData);
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

    return slideObject;
  }

  public void HideGameStateSlide() {
    _SharedMinigameViewInstance.HideGameStateSlide();
  }

  #endregion

  #region ContinueGameShelfWidget

  public void ShowContinueButtonShelf() {
    _SharedMinigameViewInstance.ShowContinueButtonShelf();
  }

  public void HideContinueButtonShelf() {
    _SharedMinigameViewInstance.HideContinueButtonShelf();
  }

  public void SetContinueButtonShelfText(string text) {
    _SharedMinigameViewInstance.SetContinueButtonShelfText(text);
  }

  public void SetContinueButtonText(string text) {
    _SharedMinigameViewInstance.SetContinueButtonText(text);
  }

  public void SetContinueButtonListener(ContinueGameShelfWidget.ContinueButtonClickHandler buttonClickHandler) {
    _SharedMinigameViewInstance.SetContinueButtonListener(buttonClickHandler);
  }

  public void EnableContinueButton(bool enable) {
    _SharedMinigameViewInstance.EnableContinueButton(enable);
  }

  #endregion

  #region Info Title Text

  public string InfoTitleText {
    get { return _SharedMinigameViewInstance.InfoTitleText; }
    set { 
      _SharedMinigameViewInstance.InfoTitleText = value; 
    }
  }

  #endregion

  #region Score Widgets

  public int CozmoScore {
    set {
      _SharedMinigameViewInstance.CozmoScore = value;
    }
  }

  public int CozmoMaxRounds {
    set {
      _SharedMinigameViewInstance.CozmoMaxRounds = value;
    }
  }

  public int CozmoRoundsWon {
    set {
      _SharedMinigameViewInstance.CozmoRoundsWon = value;
    }
  }

  public int PlayerScore {
    set {
      _SharedMinigameViewInstance.PlayerScore = value;
    }
  }

  public int PlayerMaxRounds {
    set {
      _SharedMinigameViewInstance.PlayerMaxRounds = value;
    }
  }

  public int PlayerRoundsWon {
    set {
      _SharedMinigameViewInstance.PlayerRoundsWon = value;
    }
  }

  #endregion
}

[System.Serializable]
public class GameStateSlide {
  public string slideName;
  public CanvasGroup slidePrefab;
}