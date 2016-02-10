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

  public delegate void MiniGameWinHandler(StatContainer rewardedXp);

  public event MiniGameWinHandler OnMiniGameWin;

  public delegate void MiniGameLoseHandler(StatContainer rewardedXp);

  public event MiniGameWinHandler OnMiniGameLose;

  public Robot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private SharedMinigameView _SharedMinigameViewInstance;

  public SharedMinigameView SharedMinigameView {
    get { return _SharedMinigameViewInstance; }
  }

  protected Transform SharedMinigameViewInstanceParent { get { return _SharedMinigameViewInstance.transform; } }

  protected ChallengeData _ChallengeData;
  private ChallengeEndedDialog _ChallengeEndViewInstance;
  private bool _WonChallenge;

  protected StateMachine _StateMachine = new StateMachine();

  // TODO: Delete after removing usage by Minesweeper, Codebreaker
  [SerializeField]
  protected GameStateSlide[] _GameStateSlides;

  private StatContainer _RewardedXp;

  private float _GameStartTime;

  #region Initialization

  public void InitializeMinigame(ChallengeData challengeData) {
    _GameStartTime = Time.time;
    _StateMachine.SetGameRef(this);
    _SharedMinigameViewInstance = UIManager.OpenView(
      UIPrefabHolder.Instance.SharedMinigameViewPrefab, 
      false) as SharedMinigameView;
    _SharedMinigameViewInstance.Initialize(_ChallengeData.HowToPlayDialogContentPrefab,
      _ChallengeData.HowToPlayDialogContentLocKey);
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;

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
    SharedMinigameView.CreateTitleWidget(Localization.Get(data.ChallengeTitleLocKey), data.ChallengeIcon);
    SharedMinigameView.CreateQuickQuitButton();
  }

  #endregion

  // end Initialization

  #region Update

  protected virtual void Update() {
    UpdateStateMachine();
  }

  protected virtual void UpdateStateMachine() {
    _StateMachine.UpdateStateMachine();
  }

  #endregion

  // end Update

  #region Clean Up

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

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
  }

  public void CloseMinigameImmediately() {
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MUSIC.SILENCE);
    CleanUpOnDestroy();
    Destroy(gameObject);
  }

  #endregion

  // end Clean Up

  #region Calculate Stats

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

  #endregion

  // end Calculate Stats

  #region Minigame Exit

  protected void RaiseMiniGameQuit() {
    _StateMachine.Stop();

    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }

    CloseMinigameImmediately();
  }

  public void RaiseMiniGameWin(string subtitleText = null) {
    _StateMachine.Stop();

    _WonChallenge = true;
    _SharedMinigameViewInstance.ShowCozmoScoreWidget();
    _SharedMinigameViewInstance.ShowPlayerWinnerBanner();
    OpenChallengeEndedDialog(subtitleText);
  }

  public void RaiseMiniGameLose(string subtitleText = null) {
    _StateMachine.Stop();
    _WonChallenge = false;
    _SharedMinigameViewInstance.ShowCozmoWinnerBanner();
    _SharedMinigameViewInstance.ShowPlayerScoreWidget();
    OpenChallengeEndedDialog(subtitleText);
  }

  private void OpenChallengeEndedDialog(string subtitleText = null) {
    // Open confirmation dialog instead
    GameObject challengeEndSlide = _SharedMinigameViewInstance.ShowCustomGameStateSlide(
                                     UIPrefabHolder.Instance.ChallengeEndViewPrefab.gameObject, 
                                     "ChallengeEndSlide");
    _ChallengeEndViewInstance = challengeEndSlide.GetComponent<ChallengeEndedDialog>();
    _ChallengeEndViewInstance.SetupDialog(subtitleText);

    // Listen for dialog close
    ShowContinueButtonShelf(centerShelf: true);
    SetContinueButtonText(Localization.Get(LocalizationKeys.kButtonContinue));
    SetContinueButtonListener(HandleChallengeResultViewClosed);

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

  private void HandleQuitConfirmed() {
    RaiseMiniGameQuit();
  }

  #endregion

  // end Minigame Exit handling

  #region Difficulty Select

  private int _CurrentDifficulty;

  public int CurrentDifficulty {
    get { return _CurrentDifficulty; }
    set { 
      _CurrentDifficulty = value;
      OnDifficultySet(value);
    }
  }

  protected virtual void OnDifficultySet(int difficulty) {
  }

  #endregion

  // end Difficulty Select

  #region Game State Slides

  // TODO: Delete after removing usage by CodeBreaker, Minesweeper
  public GameObject ShowFullScreenSlide(string slideName) {
    // If found, show that slide.
    GameObject slideObject = null;
    GameStateSlide foundSlideData = GetSlideByName(slideName);
    if (foundSlideData != null) {
      slideObject = _SharedMinigameViewInstance.ShowFullScreenSlide(foundSlideData);
    }
    else {
      DAS.Error(this, "Could not find slide with name '" + slideName + "' in slide prefabs! Check this object's" +
      " list of slides! gameObject.name=" + gameObject.name);
    }
    return slideObject;
  }

  // TODO: Delete after removing usage by CodeBreaker, Minesweeper
  private GameStateSlide GetSlideByName(string slideName) {
    // Search through the array for a slide of the same name
    GameStateSlide foundSlideData = null;
    foreach (var slide in _GameStateSlides) {
      if (slide != null && slide.slideName == slideName) {
        if (slide.slidePrefab != null) {
          foundSlideData = slide;
        }
        else {
          DAS.Error(this, "Null prefab for slide with name '" + slideName + "'! Check this object's" +
          " list of slides! gameObject.name=" + gameObject.name);
        }
        break;
      }
      else if (slide == null) {
        DAS.Warn(this, "Null slide found in slide prefabs! Check this object's" +
        " list of slides! gameObject.name=" + gameObject.name);
      }
    }
    return foundSlideData;
  }

  public ShowCozmoCubeSlide ShowShowCozmoCubesSlide(int numCubesRequired) {
    GameObject slideObject = _SharedMinigameViewInstance.ShowFullScreenSlide(UIPrefabHolder.Instance.InitialCubesSlide);
    ShowCozmoCubeSlide cubeSlide = slideObject.GetComponent<ShowCozmoCubeSlide>();
    cubeSlide.Initialize(numCubesRequired);
    return cubeSlide;
  }

  public GameObject ShowFullScreenSlide(GameObject slidePrefab) {
    return _SharedMinigameViewInstance.ShowFullScreenSlide(slidePrefab, slidePrefab.name);
  }

  public void ShowInfoTextSlide(string textToDisplay) {
    _SharedMinigameViewInstance.ShowInfoTextSlide(textToDisplay);
  }

  public void ShowInfoTextSlideWithKey(string localizationKey) {
    _SharedMinigameViewInstance.ShowInfoTextSlide(Localization.Get(localizationKey));
  }

  public void HideGameStateSlide() {
    _SharedMinigameViewInstance.HideGameStateSlide();
  }

  #endregion

  #region ContinueGameShelfWidget

  public void ShowContinueButtonShelf(bool centerShelf = false) {
    _SharedMinigameViewInstance.ShowContinueButtonShelf(centerShelf);
  }

  public void HideContinueButtonShelf() {
    _SharedMinigameViewInstance.HideContinueButtonShelf();
  }

  public void SetContinueButtonShelfText(string text, bool isComplete) {
    _SharedMinigameViewInstance.SetContinueButtonShelfText(text, isComplete);
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

  public bool CozmoDim {
    set {
      _SharedMinigameViewInstance.CozmoDim = value;
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

  public bool PlayerDim {
    set {
      _SharedMinigameViewInstance.PlayerDim = value;
    }
  }

  #endregion
}

[System.Serializable]
public class GameStateSlide {
  public string slideName;
  public CanvasGroup slidePrefab;
}