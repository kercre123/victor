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

  private System.Guid? _GameUUID;

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  public delegate void MiniGameWinHandler(Transform[] rewardIcons);

  public event MiniGameWinHandler OnMiniGameWin;

  public delegate void MiniGameLoseHandler(Transform[] rewardIcons);

  public event MiniGameWinHandler OnMiniGameLose;

  public delegate void EndGameDialogHandler();

  public event EndGameDialogHandler OnShowEndGameDialog;

  public IRobot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private SharedMinigameView _SharedMinigameViewInstance;

  public SharedMinigameView SharedMinigameView {
    get { return _SharedMinigameViewInstance; }
  }

  public Anki.Cozmo.Audio.GameState.Music GetDefaultMusicState() {
    return _ChallengeData.DefaultMusic;
  }

  protected Transform SharedMinigameViewInstanceParent { get { return _SharedMinigameViewInstance.transform; } }

  protected ChallengeData _ChallengeData;
  private ChallengeEndedDialog _ChallengeEndViewInstance;
  private bool _WonChallenge;

  private List<DifficultySelectOptionData> _DifficultyOptions;

  public List<DifficultySelectOptionData> DifficultyOptions {
    get {
      return _DifficultyOptions;
    }
  }


  protected StateMachine _StateMachine = new StateMachine();

  private float _GameStartTime;

  [HideInInspector]
  public List<int> CubeIdsForGame;

  private Dictionary<int, CycleData> _CubeCycleTimers;

  private class CycleData {
    public int cubeID;
    public float timeElaspedSeconds;
    public float cycleIntervalSeconds;
    public Color[] cycleColors;
    public int colorIndex;
    public bool cycleSingleColorOnly;
    public uint singleColor;
  }

  private Dictionary<int, BlinkData> _BlinkCubeTimers;

  private class BlinkData {
    public int cubeID;
    public float timeElaspedSeconds;
    public float blinkDuration;
    public Color[] originalColor;
  }

  public bool Paused {
    get {
      return _StateMachine.IsPaused;
    }
  }

  #region Initialization

  // the playGameSpecificMusic flag is mostly used for the press demo / face enrollment if we want the freeplay
  // music to continue playing without using an activity specific track.
  public void InitializeMinigame(ChallengeData challengeData, bool playGameSpecificMusic = true) {
    _GameStartTime = Time.time;
    _StateMachine.SetGameRef(this);

    _ChallengeData = challengeData;
    _DifficultyOptions = _ChallengeData.DifficultyOptions;
    _WonChallenge = false;

    if (playGameSpecificMusic) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(GetDefaultMusicState());
    }

    RobotEngineManager.Instance.CurrentRobot.TurnTowardsLastFacePose(Mathf.PI, callback: FinishTurnToFace);

    _CubeCycleTimers = new Dictionary<int, CycleData>();
    _BlinkCubeTimers = new Dictionary<int, BlinkData>();

    SkillSystem.Instance.StartGame(_ChallengeData);
    //SkillSystem.Instance.OnLevelUp += HandleCozmoSkillLevelUp;

    RegisterInterruptionStartedEvents();
  }

  private void FinishTurnToFace(bool success) {
    _SharedMinigameViewInstance = UIManager.OpenView(
      MinigameUIPrefabHolder.Instance.SharedMinigameViewPrefab, 
      newView => {
        newView.Initialize();
        InitializeView(newView, _ChallengeData);
      });

    Initialize(_ChallengeData.MinigameConfig);
    _SharedMinigameViewInstance.QuitMiniGameConfirmed += HandleQuitConfirmed;

    DAS.Event(DASConstants.Game.kStart, GetGameUUID());
    DAS.Event(DASConstants.Game.kType, GetDasGameName());

    DAS.SetGlobal(DASConstants.Game.kGlobal, GetDasGameName());
  }

  protected abstract void Initialize(MinigameConfigBase minigameConfigData);

  protected virtual void InitializeView(SharedMinigameView newView, ChallengeData data) {
    // For all challenges, set the title text and add a quit button by default
    ChallengeTitleWidget titleWidget = newView.TitleWidget;
    titleWidget.Text = Localization.Get(data.ChallengeTitleLocKey);
    titleWidget.Icon = data.ChallengeIconPlainStyle;
    newView.ShowQuitButton();

    // TODO use different color for activities vs games
    newView.InitializeColor(UIColorPalette.GameBackgroundColor);
  }

  #endregion

  // end Initialization

  #region Update

  protected virtual void Update() {
    UpdateCubeCycleLights();
    UpdateBlinkLights();
    UpdateStateMachine();
  }

  private void UpdateStateMachine() {
    _StateMachine.UpdateStateMachine();
  }

  public void PauseStateMachine() {
    _StateMachine.Pause();
  }

  public void ResumeStateMachine() {
    _StateMachine.Resume();
  }

  #endregion

  // end Update

  #region Scoring

  // Score in the Current Round
  public int CozmoScore;
  public int PlayerScore;
  // Points needed to win Round
  public int MaxScorePerRound;

  public void ResetScore() {
    CozmoScore = 0;
    PlayerScore = 0;
    UpdateUI();
  }

  // TODO: Add any effects for Scoring Points here to propogate to all minigames for consistency
  public virtual void AddCozmoPoint() {
    CozmoScore++;
  }

  public virtual void AddPlayerPoint() {
    PlayerScore++;
  }

  // Number of Rounds Won this Game
  public int PlayerRoundsWon;
  public int CozmoRoundsWon;

  // Total number of Rounds in this Game
  public int TotalRounds;
  // Number of Rounds Played this Game
  public int RoundsPlayed {
    get {
      return PlayerRoundsWon + CozmoRoundsWon;
    }
  }

  public int RoundsNeededToWin {
    get {
      return (TotalRounds / 2) + 1;
    }
  }

  public int CurrentRound {
    get {
      return RoundsPlayed + 1;
    }
  }

  public virtual void UpdateUI() {
    // TODO: Put any base logic for Updating UI Here.
  }

  public virtual void UpdateUIForGameEnd() {
    // TODO: Put any base logic for Updating UI for game end here.
  }

  public bool IsRoundComplete() {
    return (CozmoScore >= MaxScorePerRound || PlayerScore >= MaxScorePerRound);
  }

  // True if the Round is Close in terms of Points.
  public bool IsHighIntensityRound() {
    int oneThirdRoundsTotal = TotalRounds / 3;
    return (PlayerRoundsWon + CozmoRoundsWon) > oneThirdRoundsTotal;
  }

  /// <summary>
  /// Ends the current round, whoever has the higher current score wins.
  /// </summary>
  public void EndCurrentRound() {
    if (PlayerScore > CozmoScore) {
      PlayerRoundsWon++;
    }
    else {
      CozmoRoundsWon++;
    }
    UpdateUI();
  }

  // True if someone has enough rounds won to complete the Game.
  public bool IsGameComplete() {
    int losingScore = Mathf.Min(PlayerRoundsWon, CozmoRoundsWon);
    int winningScore = Mathf.Max(PlayerRoundsWon, CozmoRoundsWon);
    int roundsLeft = TotalRounds - losingScore - winningScore;
    return (winningScore > losingScore + roundsLeft);
  }

  // True if the Game is Close in terms of Rounds.
  public bool IsHighIntensityGame() {
    int twoThirdsRoundsTotal = TotalRounds / 3 * 2;
    return (PlayerRoundsWon + CozmoRoundsWon) > twoThirdsRoundsTotal;
  }

  // Handles the end of the game based on Rounds won, will attempt to progress difficulty as well
  public virtual void HandleGameEnd() {
    if (PlayerRoundsWon > CozmoRoundsWon) {
      PlayerProfile playerProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
      int currentDifficultyUnlocked = 0;
      if (playerProfile.GameDifficulty.ContainsKey(_ChallengeData.ChallengeID)) {
        currentDifficultyUnlocked = playerProfile.GameDifficulty[_ChallengeData.ChallengeID];
      }
      int newDifficultyUnlocked = CurrentDifficulty + 1;
      if (currentDifficultyUnlocked < newDifficultyUnlocked) {
        playerProfile.GameDifficulty[_ChallengeData.ChallengeID] = newDifficultyUnlocked;
        DataPersistence.DataPersistenceManager.Instance.Save();
      }
      RaiseMiniGameWin();
    }
    else {
      RaiseMiniGameLose();
    }
  }

  /// <summary>
  /// Returns Highest Unlocked Difficulty.
  /// </summary>
  /// <returns>Int representing the highest difficulty unlocked for this game's ChallengeID.</returns>
  public int HighestLevelCompleted() {
    int difficulty = 0;
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameDifficulty.TryGetValue(_ChallengeData.ChallengeID, out difficulty)) {
      return difficulty;
    }
    return 0;
  }

  #endregion

  // End Scoring

  #region Clean Up

  /// <summary>
  /// Clean up listeners and extra game objects. Called before the game is 
  /// destroyed when the player quits or the robot loses connection.
  /// </summary>
  protected abstract void CleanUpOnDestroy();

  public void OnDestroy() {
    DAS.Event(DASConstants.Game.kEnd, GetGameTimeElapsedAsStr());
    DAS.SetGlobal(DASConstants.Game.kGlobal, null);
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    DAS.Info(this, "Finished GameBase On Destroy");
    SkillSystem.Instance.EndGame();
    //SkillSystem.Instance.OnLevelUp -= HandleCozmoSkillLevelUp;

    DeregisterInterruptionStartedEvents();
    DeregisterInterruptionEndedEvents();
  }

  public void CloseMinigameImmediately() {
    DAS.Info(this, "Close Minigame Immediately");
    if (CurrentRobot != null) {
      CurrentRobot.ResetRobotState(EndGameRobotReset);
    }
    CleanUpOnDestroy();
    Destroy(gameObject);
  }

  public void SoftEndGameRobotReset() {
    DeregisterInterruptionStartedEvents();
    DeregisterInterruptionEndedEvents();

    if (CurrentRobot != null) {
      CurrentRobot.ResetRobotState(EndGameRobotReset);
      // Disable all Request game behavior groups so we don't request games at the 
      // end of game screen.
      RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
    }
  }

  public void EndGameRobotReset() {
    if (OnShowEndGameDialog != null) {
      OnShowEndGameDialog();
    }
  }

  #endregion

  // end Clean Up


  #region Minigame Exit

  protected virtual void RaiseMiniGameQuit() {
    _StateMachine.Stop();

    DAS.Event(DASConstants.Game.kQuit, null);
    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }

    CloseMinigameImmediately();
  }

  public void RaiseMiniGameWin(string subtitleText = null) {
    _StateMachine.Stop();
    _WonChallenge = true;

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedWin);

    UpdateScoreboard(didPlayerWin: _WonChallenge);

    if (string.IsNullOrEmpty(subtitleText)) {
      subtitleText = Localization.Get(LocalizationKeys.kMinigameTextPlayerWins);
    }
    OpenChallengeEndedDialog(subtitleText);
  }

  public void RaiseMiniGameLose(string subtitleText = null) {
    _StateMachine.Stop();
    _WonChallenge = false;

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedLose);

    UpdateScoreboard(didPlayerWin: _WonChallenge);

    if (string.IsNullOrEmpty(subtitleText)) {
      subtitleText = Localization.Get(LocalizationKeys.kMinigameTextCozmoWins);
    }
    OpenChallengeEndedDialog(subtitleText);
  }

  private void UpdateScoreboard(bool didPlayerWin) {
    ScoreWidget cozmoScoreboard = _SharedMinigameViewInstance.CozmoScoreboard;
    cozmoScoreboard.Dim = false;
    cozmoScoreboard.IsWinner = !didPlayerWin;
    ScoreWidget playerScoreboard = _SharedMinigameViewInstance.PlayerScoreboard;
    playerScoreboard.Dim = false;
    playerScoreboard.IsWinner = didPlayerWin;
  }

  private void OpenChallengeEndedDialog(string subtitleText = null) {

    // handles edge case of where the how to play is open.
    _SharedMinigameViewInstance.CloseHowToPlayView();

    GameObject challengeEndSlide = _SharedMinigameViewInstance.ShowNarrowGameStateSlide(
                                     MinigameUIPrefabHolder.Instance.ChallengeEndViewPrefab.gameObject, 
                                     "challenge_end_slide");
    _ChallengeEndViewInstance = challengeEndSlide.GetComponent<ChallengeEndedDialog>();
    _ChallengeEndViewInstance.SetupDialog(subtitleText);

    SoftEndGameRobotReset();

    // Listen for dialog close
    SharedMinigameView.ShowContinueButtonCentered(HandleChallengeResultViewClosed,
      Localization.Get(LocalizationKeys.kButtonContinue), "end_of_game_continue_button");
    SharedMinigameView.HideHowToPlayButton();
  }

  private void HandleChallengeResultViewClosed() {
    // Turn off lights on robot
    if (CurrentRobot != null) {
      CurrentRobot.TurnOffAllLights();
    }

    // Get unparented reward icons
    Transform[] rewardIconObjects = _ChallengeEndViewInstance.GetRewardIconsByStat();

    // Pass icons and xp to HomeHub
    if (_WonChallenge) {
      DAS.Event(DASConstants.Game.kEndWithRank, DASConstants.Game.kRankPlayerWon);
      if (OnMiniGameWin != null) {
        OnMiniGameWin(rewardIconObjects);
      } 
    }
    else {
      DAS.Event(DASConstants.Game.kEndWithRank, DASConstants.Game.kRankPlayerLose);
      if (OnMiniGameLose != null) {
        OnMiniGameLose(rewardIconObjects);
      }
    }

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameEnd);

    // Close minigame UI
    CloseMinigameImmediately();
    DAS.Info(this, "HandleChallengeResultViewClosed");
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

  /// <summary>
  /// TODO: Replace this with better handling for notifying Results view that a level up occoured during
  /// game instead of creating a popup. Create an appropriate results cell with the same info.
  /// </summary>
  /// <param name="newLevel">New level.</param>
  protected void HandleCozmoSkillLevelUp(int newLevel) {
    AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab);
    // Hook up callbacks
    alertView.SetCloseButtonEnabled(true);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue);
    alertView.TitleLocKey = LocalizationKeys.kSkillsLevelUpTitle;
    alertView.DescriptionLocKey = LocalizationKeys.kSkillsLevelUpDescription;
    alertView.SetMessageArgs(new object[] { newLevel, _ChallengeData.name });
  }

  #endregion

  // end Difficulty Select

  #region LightCubes

  public float GetCycleDurationSeconds(int numRotations, float cycleIntervalSeconds) {
    return (numRotations * cycleIntervalSeconds * 4);
  }

  public void StartCycleCube(int cubeID, Color[] lightColorsCounterclockwise, float cycleIntervalSeconds) {
    // Remove from blink lights if it exists there
    StopBlinkLight(cubeID);

    // Set colors
    LightCube cube = CurrentRobot.LightCubes[cubeID];
    cube.SetLEDs(lightColorsCounterclockwise);

    // Set up timing data
    CycleData data = new CycleData();
    data.cubeID = cubeID;
    data.cycleIntervalSeconds = cycleIntervalSeconds;
    data.timeElaspedSeconds = 0f;
    data.colorIndex = 0;
    data.cycleColors = lightColorsCounterclockwise;
    data.cycleSingleColorOnly = false;

    // Update data
    if (_CubeCycleTimers.ContainsKey(cubeID)) {
      _CubeCycleTimers[cubeID] = data;
    }
    else {
      _CubeCycleTimers.Add(cubeID, data);
    }
  }

  public void StartCycleCubeSingleColor(int cubeID, Color[] lightColors, float cycleIntervalSeconds, Color rotateColor) {
    StartCycleCube(cubeID, lightColors, cycleIntervalSeconds);
    if (_CubeCycleTimers.ContainsKey(cubeID)) {
      _CubeCycleTimers[cubeID].cycleSingleColorOnly = true;
      _CubeCycleTimers[cubeID].singleColor = rotateColor.ToUInt();
    }
  }

  public void StopCycleCube(int cubeID) {
    if (_CubeCycleTimers.ContainsKey(cubeID)) {
      _CubeCycleTimers.Remove(cubeID);
    }
  }

  private void UpdateCubeCycleLights() {
    foreach (KeyValuePair<int,CycleData> kvp in _CubeCycleTimers) {
      kvp.Value.timeElaspedSeconds += Time.deltaTime;

      if (kvp.Value.timeElaspedSeconds > kvp.Value.cycleIntervalSeconds) {
        if (kvp.Value.cycleSingleColorOnly) {
          CycleLightsSingleColor(kvp.Value);
        }
        else {
          CycleLightsClockwise(kvp.Value);
        }
        kvp.Value.timeElaspedSeconds %= kvp.Value.cycleIntervalSeconds;
      }
    }
  }

  private void CycleLightsSingleColor(CycleData data) {
    LightCube cube = CurrentRobot.LightCubes[data.cubeID];
    data.colorIndex++;
    data.colorIndex %= cube.Lights.Length;
    for (int i = 0; i < cube.Lights.Length; i++) {
      cube.Lights[i].OnColor = data.cycleColors[i % data.cycleColors.Length].ToUInt();
      if (i == data.colorIndex) {
        cube.Lights[i].OnColor = data.singleColor;
      }
      cube.Lights[i].OnPeriodMs = LightCube.Light.FOREVER;
    }
  }

  private void CycleLightsClockwise(CycleData data) {
    LightCube cube = CurrentRobot.LightCubes[data.cubeID];
    data.colorIndex++;
    data.colorIndex %= data.cycleColors.Length;
    int colorIndex = 0;
    for (int i = 0; i < cube.Lights.Length; i++) {
      colorIndex = (data.colorIndex + i) % data.cycleColors.Length;
      cube.Lights[i].OnColor = data.cycleColors[colorIndex].ToUInt();
      cube.Lights[i].OnPeriodMs = LightCube.Light.FOREVER;
    }
  }

  public void BlinkLight(int cubeId, float duration, Color blinkColor, Color originalColor) {
    BlinkLight(cubeId, duration, new Color[]{ blinkColor }, new Color[]{ originalColor });
  }

  public void BlinkLight(int cubeId, float duration, Color[] blinkColorsCounterclockwise, Color[] originalColors) {
    // Remove from cycle cubes if it's there
    StopCycleCube(cubeId);

    // Set up timing data
    BlinkData data = new BlinkData();
    data.cubeID = cubeId;
    data.blinkDuration = duration;
    data.timeElaspedSeconds = 0f;
    data.originalColor = originalColors;

    // Set colors
    LightCube cube = CurrentRobot.LightCubes[cubeId];
    cube.SetLEDs(blinkColorsCounterclockwise);

    // Update data
    if (_BlinkCubeTimers.ContainsKey(cubeId)) {
      _BlinkCubeTimers[cubeId] = data;
    }
    else {
      _BlinkCubeTimers.Add(cubeId, data);
    }
  }

  private void StopBlinkLight(int cubeId) {
    if (_BlinkCubeTimers.ContainsKey(cubeId)) {
      LightCube cube = CurrentRobot.LightCubes[cubeId];
      cube.SetLEDs(_BlinkCubeTimers[cubeId].originalColor);
      _BlinkCubeTimers.Remove(cubeId);
    }
  }

  private void UpdateBlinkLights() {
    List<int> cubesToStopBlinking = new List<int>();
    foreach (KeyValuePair<int,BlinkData> cubeIdToTimer in _BlinkCubeTimers) {
      cubeIdToTimer.Value.timeElaspedSeconds += Time.deltaTime;
      if (cubeIdToTimer.Value.timeElaspedSeconds > cubeIdToTimer.Value.blinkDuration) {
        cubesToStopBlinking.Add(cubeIdToTimer.Key);
      }
    }

    foreach (int cubeId in cubesToStopBlinking) {
      StopBlinkLight(cubeId);
    }
  }

  #endregion

  // end LightCubes

  #region DAS Events

  private string GetGameUUID() {
    // TODO: Does this need to be more complicated?
    if (!_GameUUID.HasValue) {
      _GameUUID = System.Guid.NewGuid();
    }
    return _GameUUID.Value.ToString();
  }

  private string GetDasGameName() {
    return _ChallengeData.ChallengeID.ToLower().Replace("game", "");
  }

  private string GetGameTimeElapsedAsStr() {
    return string.Format("{0}", Time.time - _GameStartTime);
  }

  #endregion

  // end DAS

  #region Cliff or Pickup handling

  protected void RegisterInterruptionStartedEvents() {
    DeregisterInterruptionStartedEvents();
    RobotEngineManager.Instance.OnCliffEvent += HandleRobotCliffEventStarted;
    RobotEngineManager.Instance.OnRobotPickedUp += HandleRobotPickedUpStarted;
    RobotEngineManager.Instance.OnRobotOnBack += HandleRobotOnBackEventStarted;
  }

  protected void DeregisterInterruptionStartedEvents() {
    RobotEngineManager.Instance.OnCliffEvent -= HandleRobotCliffEventStarted;
    RobotEngineManager.Instance.OnRobotPickedUp -= HandleRobotPickedUpStarted;
    RobotEngineManager.Instance.OnRobotOnBack -= HandleRobotOnBackEventStarted;
  }

  protected void RegisterInterruptionEndedEvents() {
    DeregisterInterruptionEndedEvents();
    RobotEngineManager.Instance.OnCliffEventFinished += HandleRobotInterruptionEventEnded;
    RobotEngineManager.Instance.OnRobotPutDown += HandleRobotInterruptionEventEnded;
    RobotEngineManager.Instance.OnRobotOnBackFinished += HandleRobotInterruptionEventEnded;
  }

  protected void DeregisterInterruptionEndedEvents() {
    RobotEngineManager.Instance.OnCliffEventFinished -= HandleRobotInterruptionEventEnded;
    RobotEngineManager.Instance.OnRobotPutDown -= HandleRobotInterruptionEventEnded;
    RobotEngineManager.Instance.OnRobotOnBackFinished -= HandleRobotInterruptionEventEnded;
  }

  private void HandleRobotPickedUpStarted() {
    HandleRobotInterruptionEventStarted();
  }

  private void HandleRobotCliffEventStarted(Anki.Cozmo.CliffEvent cliffEvent) {
    HandleRobotInterruptionEventStarted();
  }

  private void HandleRobotOnBackEventStarted(Anki.Cozmo.ExternalInterface.RobotOnBack robotOnBackMessage) {
    HandleRobotInterruptionEventStarted();
  }

  protected virtual void HandleRobotInterruptionEventStarted() {
    DeregisterInterruptionStartedEvents();
    RegisterInterruptionEndedEvents();

    PauseStateMachine();
  }

  protected virtual void HandleRobotInterruptionEventEnded() {
    DeregisterInterruptionEndedEvents();
    RegisterInterruptionStartedEvents();

    ResumeStateMachine();
  }

  public void ShowDontMoveCozmoAlertView() {
    ShowInterruptionQuitGameView(LocalizationKeys.kMinigameDontMoveCozmoTitle, LocalizationKeys.kMinigameDontMoveCozmoDescription);
  }

  protected void ShowInterruptionQuitGameView(string titleKey, string descriptionKey) {
    SoftEndGameRobotReset();

    Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
    alertView.SetCloseButtonEnabled(false);
    alertView.SetPrimaryButton(LocalizationKeys.kButtonQuitGame, null);
    alertView.ViewCloseAnimationFinished += HandleInterruptionQuitGameViewClosed;
    alertView.TitleLocKey = titleKey;
    alertView.DescriptionLocKey = descriptionKey;
    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
  }

  private void HandleInterruptionQuitGameViewClosed() {
    RaiseMiniGameQuit();
  }

  #endregion
}