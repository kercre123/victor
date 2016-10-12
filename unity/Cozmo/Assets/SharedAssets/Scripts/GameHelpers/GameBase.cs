using UnityEngine;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Cozmo.MinigameWidgets;
using Cozmo;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Anki.Assets;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {

  private const float kWaitForPickupOrPlaceTimeout_sec = 30f;
  private const float kChallengeCompleteScoreboardDelay = 5f;
  private float _AutoAdvanceTimestamp = -1f;

  private System.Guid? _GameUUID;

  public delegate void MiniGameQuitHandler();

  public event MiniGameQuitHandler OnMiniGameQuit;

  public delegate void MiniGameWinHandler();

  public event MiniGameWinHandler OnMiniGameWin;

  public delegate void MiniGameLoseHandler();

  public event MiniGameWinHandler OnMiniGameLose;

  public delegate void EndGameDialogHandler();

  public event EndGameDialogHandler OnShowEndGameDialog;

  public delegate void SharedMinigameViewHandler(SharedMinigameView newView);

  public event SharedMinigameViewHandler OnSharedMinigameViewInitialized;

  public IRobot CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  private AlertView _InterruptedAlertView = null;

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

  // Can add here if we need to display multiplayer scores too.
  public enum EndState {
    PlayerWin,
    CozmoWin,
    Tie,
    Quit,
  }
  private EndState _EndState;
  protected bool _ShowScoreboardOnComplete = true;

  protected List<Anki.Cozmo.BehaviorType> _DisabledReactionaryBehaviors = new List<BehaviorType>();

  private List<DifficultySelectOptionData> _DifficultyOptions;

  public List<DifficultySelectOptionData> DifficultyOptions {
    get {
      return _DifficultyOptions;
    }
  }

  public string ChallengeID {
    get {
      return _ChallengeData.ChallengeID;
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

  private bool _ResultsViewReached = false;

  #region Initialization

  protected const string _kReactionaryBehaviorOwnerId = "unity_game";

  // called when the game starts to disable reactionary behaviors, then again when the game exits to re-enable them
  private void InitializeReactionaryBehaviorsForGameStart() {
    AddDisabledReactionaryBehaviors();
    foreach (Anki.Cozmo.BehaviorType reactionaryBehavior in _DisabledReactionaryBehaviors) {
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, reactionaryBehavior, false);
    }
  }

  protected virtual void AddDisabledReactionaryBehaviors() {
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToCubeMoved);
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.AcknowledgeObject);
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.AcknowledgeFace);
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToFrustration);

    // for some reason this became a reactionary behavior but isn't named like one...
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.KnockOverCubes);
    _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.CantHandleTallStack);
  }

  private void ResetReactionaryBehaviorsForGameEnd() {
    foreach (Anki.Cozmo.BehaviorType reactionaryBehavior in _DisabledReactionaryBehaviors) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, reactionaryBehavior, true);
      }
    }
  }

  // the playGameSpecificMusic flag is mostly used for the press demo / face enrollment if we want the freeplay
  // music to continue playing without using an activity specific track.
  public void InitializeMinigame(ChallengeData challengeData, bool playGameSpecificMusic = true) {
    _GameStartTime = Time.time;
    _StateMachine.SetGameRef(this);

    _ChallengeData = challengeData;
    _DifficultyOptions = _ChallengeData.DifficultyOptions;
    _EndState = EndState.CozmoWin;
    _ResultsViewReached = false;
    if (CurrentRobot != null) {

      // this is done to prevent entering a game while robot is trying to finish get out seqeuences as part
      // of RobotResetState.
      CurrentRobot.CancelAllCallbacks();
      CurrentRobot.CancelAction(RobotActionType.UNKNOWN);

      CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      if ((CurrentRobot.RobotStatus & RobotStatusFlag.IS_CARRYING_BLOCK) != 0) {
        CurrentRobot.PlaceObjectOnGroundHere();
      }
      CurrentRobot.SetEnableFreeplayLightStates(false);
      CurrentRobot.SendAnimationTrigger(_ChallengeData.GetInAnimTrigger.Value);
    }

    if (playGameSpecificMusic) {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(GetDefaultMusicState());
    }

    _CubeCycleTimers = new Dictionary<int, CycleData>();
    _BlinkCubeTimers = new Dictionary<int, BlinkData>();

    SkillSystem.Instance.StartGame(_ChallengeData);
    // Clear Pending Rewards and Unlocks so ChallengeEndedDialog only displays things earned during this game
    RewardedActionManager.Instance.SendPendingRewardsToInventory();

    InitializeReactionaryBehaviorsForGameStart();

    RegisterRobotReactionaryBehaviorEvents();

    LoadMinigameUIAssetBundle();
    DAS.Event("game.launch", GetDasGameName());
  }

  private void LoadMinigameUIAssetBundle() {
    string minigameAssetBundleName = AssetBundleNames.minigame_ui_prefabs.ToString();
    AssetBundleManager.Instance.LoadAssetBundleAsync(
      minigameAssetBundleName, (bool success) => {
        if (success) {
          LoadSharedMinigameView(minigameAssetBundleName);
        }
        else {
          DAS.Error("GameBase.LoadMinigameUIAssetBundle", "Failed to load asset bundle " + minigameAssetBundleName);
        }
      });
  }

  private void LoadSharedMinigameView(string minigameAssetBundleName) {
    MinigameUIPrefabHolder.LoadSharedMinigameViewPrefab(minigameAssetBundleName, (GameObject viewPrefab) => {
      if (viewPrefab != null) {
        SharedMinigameView prefabScript = viewPrefab.GetComponent<SharedMinigameView>();
        _SharedMinigameViewInstance = UIManager.OpenView(prefabScript, newView => {
          newView.Initialize(_ChallengeData);
          InitializeMinigameView(newView, _ChallengeData);

          if (OnSharedMinigameViewInitialized != null) {
            OnSharedMinigameViewInitialized(newView);
          }
        });

        bool videoPlayedAlready = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameInstructionalVideoPlayed.ContainsKey(_ChallengeData.ChallengeID);
        bool noInstructionVideo = string.IsNullOrEmpty(_ChallengeData.InstructionVideoPath);

        if (videoPlayedAlready || noInstructionVideo) {
          FinishedInstructionalVideo();
        }
        else {
          SharedMinigameView.PlayVideo(_ChallengeData.InstructionVideoPath, FinishedInstructionalVideo);
        }
      }
      else {
        DAS.Error("GameBase.LoadSharedMinigameView", "Failed to shared minigame view");
      }
    });
  }

  private void FinishedInstructionalVideo() {
    bool videoPlayedAlready = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameInstructionalVideoPlayed.ContainsKey(_ChallengeData.ChallengeID);
    if (!videoPlayedAlready) {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameInstructionalVideoPlayed.Add(_ChallengeData.ChallengeID, true);
      DataPersistence.DataPersistenceManager.Instance.Save();
    }
    PrepRobotForGame();
  }

  private void InitializeMinigameView(SharedMinigameView newView, ChallengeData data) {
    // For all challenges, set the title text and add a quit button by default
    ChallengeTitleWidget titleWidget = newView.TitleWidget;
    titleWidget.Text = Localization.Get(data.ChallengeTitleLocKey);
    titleWidget.SubtitleText = null;
    newView.ShowQuitButton();

    if (data.IsMinigame) {
      newView.InitializeColor(UIColorPalette.GameBackgroundColor);
    }
    else {
      newView.InitializeColor(UIColorPalette.ActivityBackgroundColor);
    }

    newView.ShowWideSlideWithText(LocalizationKeys.kMinigameLabelCozmoPrep, null);
    newView.ShowShelf();
    newView.ShowSpinnerWidget();
    newView.QuitMiniGameConfirmed += HandleQuitConfirmed;
    ContextManager.Instance.OnAppHoldStart += HandleAppHoldStart;
    ContextManager.Instance.OnAppHoldEnd += HandleAppHoldEnd;
    PauseManager.Instance.OnPauseDialogOpen += HandlePauseManagerDialogOpened;
  }

  private void PrepRobotForGame() {
    IRobot currentRobot = RobotEngineManager.Instance.CurrentRobot;
    if (currentRobot == null) {
      return;
    }
    if (currentRobot.RobotStatus == RobotStatusFlag.IS_PICKING_OR_PLACING) {
      StartCoroutine(WaitForPickingUpOrPlacingFinish(currentRobot, Time.time));
    }
    else {
      CheckForCarryingBlock(currentRobot);
    }

    CurrentRobot.OnLightCubeRemoved += HandleLightCubeRemoved;
  }

  private IEnumerator WaitForPickingUpOrPlacingFinish(IRobot robot, float startTimestamp) {
    while (robot.RobotStatus == RobotStatusFlag.IS_PICKING_OR_PLACING
           && (Time.time - startTimestamp) < kWaitForPickupOrPlaceTimeout_sec) {
      yield return new WaitForEndOfFrame();
    }

    CheckForCarryingBlock(robot);
  }

  private void CheckForCarryingBlock(IRobot robot) {
    if (robot.RobotStatus == RobotStatusFlag.IS_CARRYING_BLOCK) {
      robot.PlaceObjectOnGroundHere(FinishPlaceObjectOnGround);
    }
    else {
      robot.TurnTowardsLastFacePose(Mathf.PI, callback: FinishTurnToFace);
    }
  }

  private void FinishPlaceObjectOnGround(bool success) {
    RobotEngineManager.Instance.CurrentRobot.TurnTowardsLastFacePose(Mathf.PI, callback: FinishTurnToFace);
  }

  private void FinishTurnToFace(bool success) {
    _SharedMinigameViewInstance.ShowMiddleBackground();
    _SharedMinigameViewInstance.HideSpinnerWidget();

    DAS.SetGlobal(DASConstants.Game.kGlobal, GetGameUUID());
    DAS.Event(DASConstants.Game.kStart, GetGameUUID());
    DAS.Event(DASConstants.Game.kType, GetDasGameName());
    DasTracker.Instance.TrackGameStarted();

    InitializeGame(_ChallengeData.MinigameConfig);
    if (!string.IsNullOrEmpty(_ChallengeData.InstructionVideoPath)) {
      _SharedMinigameViewInstance.ShowInstructionsVideoButton();
    }

    SetupViewAfterCozmoReady(_SharedMinigameViewInstance, _ChallengeData);

  }

  /// <summary>
  /// Called after Cozmo is "Ready". At this point the SharedMinigameView has already been instantiated.
  /// Use this method to initialize the state machine.
  /// </summary>
  /// <param name="minigameConfigData">Minigame config data.</param>
  protected abstract void InitializeGame(MinigameConfigBase minigameConfigData);

  /// <summary>
  /// Called after Cozmo is "Ready". At this point the SharedMinigameView has already been instantiated.
  /// Use this method to initialize the SharedMinigameView.
  /// </summary>
  /// <param name="minigameConfigData">Minigame config data.</param>
  protected virtual void SetupViewAfterCozmoReady(SharedMinigameView newView, ChallengeData data) {
  }

  #endregion

  // end Initialization

  #region Update

  protected virtual void Update() {
    UpdateCubeCycleLights();
    UpdateBlinkLights();
    UpdateStateMachine();
    AutoAdvanceCheck();
  }

  private void UpdateStateMachine() {
    _StateMachine.UpdateStateMachine();
  }

  // Use these if we need special cube behavior on app hold or whatever for specific minigames
  protected virtual void HandleAppHoldStart() {
    DAS.Info("GameBase.HandleAppHoldStart", "StartAppHold");

  }

  protected virtual void HandleAppHoldEnd() {
    DAS.Info("GameBase.HandleAppHoldEnd", "EndAppHold");

  }

  public void PauseStateMachine(State.PauseReason reason, BehaviorType reactionaryBehavior) {
    _StateMachine.Pause(reason, reactionaryBehavior);
  }

  public void ResumeStateMachine(State.PauseReason reason, BehaviorType reactionaryBehavior) {
    _StateMachine.Resume(reason, reactionaryBehavior);
  }

  #endregion

  // end Update

  #region Scoring

  // Score in the Current Round
  [HideInInspector]
  public int CozmoScore;

  [HideInInspector]
  public int PlayerScore;

  // Number of Rounds Won this Game
  [HideInInspector]
  public int PlayerScoreTotal;

  [HideInInspector]
  public int CozmoScoreTotal;

  // Number of Errors made, Error being a player action
  // that causes failure (not cozmo scoring a point)
  private int _PlayerMistakeCount;
  public int PlayerMistakeCount {
    get {
      return _PlayerMistakeCount;
    }
  }

  // Number of Errors made by cozmo, error being a cozmo
  // mistake that gives the player points.
  private int _CozmoMistakeCount;
  public int CozmoMistakeCount {
    get {
      return _CozmoMistakeCount;
    }
  }

  // The accuracy of the player's attempts to score points
  public float PlayerAccuracy {
    get {
      // If the player didn't score any points without cozmo making a mistake
      // then they have 0 % accuracy.
      if (PlayerScoreTotal - _CozmoMistakeCount <= 0) {
        return 0.0f;
      }
      // If the player has scored points intentionally, then their accuracy is
      // equal to their total points scored out of their attempts to score
      // points.
      float acc = ((float)PlayerScoreTotal / (float)(PlayerScoreTotal + _PlayerMistakeCount));
      return acc;
    }
  }

  // Points needed to win Round
  [HideInInspector]
  public int MaxScorePerRound;

  public virtual void PlayerMistake() {
    _PlayerMistakeCount++;
  }

  public virtual void CozmoMistake() {
    _CozmoMistakeCount++;
  }

  public void ResetScore() {
    CozmoScore = 0;
    PlayerScore = 0;
    UpdateUI();
  }

  public virtual void AddPoint(bool playerScored) {
    if (playerScored) {
      PlayerScore++;
      PlayerScoreTotal++;
    }
    else {
      CozmoScore++;
      CozmoScoreTotal++;
    }
    GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengePointScored, _ChallengeData.ChallengeID, _CurrentDifficulty, playerScored, PlayerScore, CozmoScore, IsHighIntensityRound()));
  }

  // Number of Rounds Won this Game
  [HideInInspector]
  public int PlayerRoundsWon;

  [HideInInspector]
  public int CozmoRoundsWon;

  // Total number of Rounds in this Game
  [HideInInspector]
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
  // can be overriden if you have special challenge specific logic for determining this
  public virtual bool IsHighIntensityRound() {
    int oneThirdRoundsTotal = TotalRounds / 3;
    return (PlayerRoundsWon + CozmoRoundsWon) > oneThirdRoundsTotal;
  }

  /// <summary>
  /// Ends the current round, whoever has the higher current score wins.
  /// </summary>
  public void EndCurrentRound() {
    bool playerWon = false;
    if (PlayerScore > CozmoScore) {
      PlayerRoundsWon++;
      playerWon = true;
    }
    else {
      CozmoRoundsWon++;
    }

    GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeRoundEnd, _ChallengeData.ChallengeID, _CurrentDifficulty, playerWon, PlayerScore, CozmoScore, IsHighIntensityRound()));

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
  public virtual void StartRoundBasedGameEnd() {
    // Fire OnGameComplete, passing in ChallengeID, CurrentDifficulty, and if Playerwon
    bool playerWon = PlayerRoundsWon > CozmoRoundsWon;
    StartBaseGameEnd(playerWon);
  }

  // now supports ties
  public virtual void StartBaseGameEnd(EndState endState) {
    GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeComplete, _ChallengeData.ChallengeID, _CurrentDifficulty, endState == EndState.PlayerWin, PlayerScore, CozmoScore, IsHighIntensityRound()));

    if (endState == EndState.PlayerWin) {
      HandleUnlockRewards();
      RaiseMiniGameWin();
    }
    else {
      if (endState == EndState.CozmoWin) {
        RaiseMiniGameLose();
      }
      else {
        RaiseMiniGameTie();
      }
    }
  }

  public virtual void StartBaseGameEnd(bool playerWon) {
    StartBaseGameEnd(playerWon ? EndState.PlayerWin : EndState.CozmoWin);
  }

  private void HandleUnlockRewards() {
    DataPersistence.PlayerProfile playerProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
    int currentDifficultyUnlocked = 0;
    if (playerProfile.GameDifficulty.ContainsKey(_ChallengeData.ChallengeID)) {
      currentDifficultyUnlocked = playerProfile.GameDifficulty[_ChallengeData.ChallengeID];
    }
    // Set NewDifficultyUnlock to -1 so it will be ignored by ChallengeEndedDialog
    RewardedActionManager.Instance.NewDifficultyUnlock = -1;
    int newDifficultyUnlocked = CurrentDifficulty + 1;
    bool higherLevelUnlocked = currentDifficultyUnlocked < newDifficultyUnlocked;
    bool highestOptionLevel = !(_ChallengeData.DifficultyOptions != null && newDifficultyUnlocked < _ChallengeData.DifficultyOptions.Count);

    if (higherLevelUnlocked && !highestOptionLevel) {
      playerProfile.GameDifficulty[_ChallengeData.ChallengeID] = newDifficultyUnlocked;
      DataPersistence.DataPersistenceManager.Instance.Save();
      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeDifficultyUnlock, _ChallengeData.ChallengeID, newDifficultyUnlocked));
      RewardedActionManager.Instance.NewDifficultyUnlock = newDifficultyUnlocked;
      DasTracker.Instance.TrackDifficultyUnlocked(_ChallengeData.ChallengeID, newDifficultyUnlocked);
    }
  }

  /// <summary>
  /// Returns Highest Unlocked Difficulty.
  /// </summary>
  /// <returns>Int representing the highest difficulty unlocked for this game's ChallengeID.</returns>
  public int HighestLevelCompleted() {
    int difficulty = 0;
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameDifficulty.TryGetValue(_ChallengeData.ChallengeID, out difficulty)) {
      // someone has reduced the number of difficulties saved, should only happen in development
      if (difficulty >= _ChallengeData.DifficultyOptions.Count) {
        return _ChallengeData.DifficultyOptions.Count - 1;
      }
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
    ContextManager.Instance.AppHoldEnd();
    DAS.Event(DASConstants.Game.kEnd, GetGameTimeElapsedAsStr());
    DAS.SetGlobal(DASConstants.Game.kGlobal, null);
    DasTracker.Instance.TrackGameEnded();
    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.CloseViewImmediately();
      _SharedMinigameViewInstance = null;
    }
    DAS.Info(this, "Finished GameBase On Destroy");
    SkillSystem.Instance.EndGame();

    DeregisterRobotReactionaryBehaviorEvents();

    ResetReactionaryBehaviorsForGameEnd();

    if (CurrentRobot != null) {
      CurrentRobot.SetEnableFreeplayLightStates(true);
    }

    AssetBundleManager.Instance.UnloadAssetBundle(AssetBundleNames.minigame_ui_prefabs.ToString());
  }

  private void QuitMinigame() {
    try {
      _SharedMinigameViewInstance.ViewCloseAnimationFinished += QuitMinigameAnimationFinished;
      _SharedMinigameViewInstance.CloseView();

      // cancels any queued up actions or co-routines
      if (CurrentRobot != null) {
        CurrentRobot.CancelAction(RobotActionType.UNKNOWN);
      }
      StopAllCoroutines();
    }
    catch (System.Exception e) {
      // This is happening sometimes when disconnecting from robot when a dialog is up.
      DAS.Info("StopAllCoroutines null ref internally sometimes", e.StackTrace);
    }
  }

  private void QuitMinigameAnimationFinished() {
    _SharedMinigameViewInstance.ViewCloseAnimationFinished -= QuitMinigameAnimationFinished;

    if (OnMiniGameQuit != null) {
      OnMiniGameQuit();
    }

    CleanUp();
  }

  private void CloseMinigame() {
    _SharedMinigameViewInstance.ViewCloseAnimationFinished += CleanUp;
    _SharedMinigameViewInstance.CloseView();
  }

  public void CloseMinigameImmediately() {
    DAS.Info(this, "Close Minigame Immediately");
    // If quitting early from a minigame, clear the pending action rewards, they should not be rewarded
    if (!_ResultsViewReached && _ChallengeData.IsMinigame) {
      RewardedActionManager.Instance.ResetPendingRewards();
    }
    _StateMachine.Stop();
    _SharedMinigameViewInstance.CloseViewImmediately();
    CleanUp();
  }

  public void CleanUp() {

    DAS.Debug("GameBase.CleanUp", "cleanup called");

    _SharedMinigameViewInstance.ViewCloseAnimationFinished -= CleanUp;

    ContextManager.Instance.OnAppHoldStart -= HandleAppHoldStart;
    ContextManager.Instance.OnAppHoldEnd -= HandleAppHoldEnd;
    PauseManager.Instance.OnPauseDialogOpen -= HandlePauseManagerDialogOpened;

    if (ContextManager.Instance.ManagerBusy) {
      ContextManager.Instance.OnAppHoldEnd();
    }

    if (CurrentRobot != null) {
      if (_StateMachine.GetReactionThatPausedGame() == Anki.Cozmo.BehaviorType.NoneBehavior) {
        // clears the action queue before quitting the game.
        CurrentRobot.CancelAction(RobotActionType.UNKNOWN);
      }

      CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;
    }

    if (_InterruptedAlertView != null) {
      _InterruptedAlertView.CloseViewImmediately();
    }

    // Some CleanUpOnDestroy overrides send a robot animation as well
    CleanUpOnDestroy();
    Destroy(gameObject);
  }

  public void SoftEndGameRobotReset() {
    DeregisterRobotReactionaryBehaviorEvents();
    AddToTotalGamesPlayed();

    DAS.Debug("GameBase.SoftEndGameRobotReset", "soft end game reset called");

    RobotEngineManager.Instance.RequestGameManager.DisableRequestGameBehaviorGroups();

    if (OnShowEndGameDialog != null) {
      OnShowEndGameDialog();
    }
  }

  private void AddToTotalGamesPlayed() {
    // Increment the amount that this challenge has been played by 1, only count fully completed playthroughs of challenges
    if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.ContainsKey(_ChallengeData.ChallengeID)) {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed[_ChallengeData.ChallengeID]++;
    }
    else {
      DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.Add(_ChallengeData.ChallengeID, 1);
    }
    DataPersistence.DataPersistenceManager.Instance.Save();
  }

  #endregion

  // end Clean Up


  #region Minigame Exit

  public void RaiseMiniGameQuit() {
    _StateMachine.Stop();
    DAS.Event(DASConstants.Game.kQuit, null);
    SendCustomEndGameDasEvents();

    // Track # times played for activities that have no win or lose state
    // Also fire OnChallengeComplete here for those
    if (!_ChallengeData.IsMinigame) {
      AddToTotalGamesPlayed();
      GameEventManager.Instance.FireGameEvent(GameEventWrapperFactory.Create(GameEvent.OnChallengeComplete, _ChallengeData.ChallengeID, _CurrentDifficulty, true, PlayerScore, CozmoScore, IsHighIntensityRound()));
    }
    // If quitting early from a minigame, clear the pending action rewards, they should not be rewarded
    if (!_ResultsViewReached && _ChallengeData.IsMinigame) {
      RewardedActionManager.Instance.ResetPendingRewards();
    }
    QuitMinigame();
  }

  protected virtual void SendCustomEndGameDasEvents() {
    // Implement in subclasses if desired
  }

  private void RaiseMiniGameWin() {
    _StateMachine.Stop();
    _EndState = EndState.PlayerWin;
    if (DataPersistence.DataPersistenceManager.Instance.CurrentSession.TotalWins.ContainsKey(_ChallengeData.ChallengeID)) {
      DataPersistence.DataPersistenceManager.Instance.CurrentSession.TotalWins[_ChallengeData.ChallengeID]++;
    }
    else {
      DataPersistence.DataPersistenceManager.Instance.CurrentSession.TotalWins.Add(_ChallengeData.ChallengeID, 1);
    }
    if (_ShowScoreboardOnComplete) {
      UpdateScoreboard(didPlayerWin: _EndState == EndState.PlayerWin);
    }
    ShowWinnerState();
  }

  private void RaiseMiniGameLose() {
    _StateMachine.Stop();
    _EndState = EndState.CozmoWin;

    if (_ShowScoreboardOnComplete) {
      UpdateScoreboard(didPlayerWin: _EndState == EndState.PlayerWin);
    }

    ShowWinnerState();
  }

  private void RaiseMiniGameTie() {
    _StateMachine.Stop();
    _EndState = EndState.Tie;
    ShowWinnerState();
  }

  private void UpdateScoreboard(bool didPlayerWin) {
    ScoreWidget cozmoScoreboard = _SharedMinigameViewInstance.CozmoScoreboard;
    cozmoScoreboard.Dim = false;
    cozmoScoreboard.IsWinner = !didPlayerWin;
    ScoreWidget playerScoreboard = _SharedMinigameViewInstance.PlayerScoreboard;
    playerScoreboard.Dim = false;
    playerScoreboard.IsWinner = didPlayerWin;
  }

  protected virtual void ShowWinnerState() {
    SoftEndGameRobotReset();
    _ResultsViewReached = true;
    ContextManager.Instance.AppFlash(playChime: true);
    string winnerText = "";
    if (_EndState == EndState.PlayerWin) {
      winnerText = Localization.GetWithArgs(LocalizationKeys.kMinigameTextPlayerWins, new object[] { DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName });
    }
    else if (_EndState == EndState.CozmoWin) {
      winnerText = Localization.Get(LocalizationKeys.kMinigameTextCozmoWins);
    }
    else if (_EndState == EndState.Tie) {
      winnerText = Localization.Get(LocalizationKeys.kMinigameTextTie);
    }
    SharedMinigameView.InfoTitleText = winnerText;
    _AutoAdvanceTimestamp = Time.time;
  }

  public void AutoAdvanceCheck() {
    if (_AutoAdvanceTimestamp != -1 && (Time.time - _AutoAdvanceTimestamp > kChallengeCompleteScoreboardDelay)) {
      HandleChallengeResultAdvance();
    }
  }

  // Update the Challenge Ended Dialog to show all pending rewards and unlocks,
  // if there are none, close the view.
  private void HandleChallengeResultAdvance() {
    _AutoAdvanceTimestamp = -1;
    if (RewardedActionManager.Instance.RewardPending || RewardedActionManager.Instance.NewDifficultyPending) {
      SharedMinigameView.HidePlayerScoreboard();
      SharedMinigameView.HideCozmoScoreboard();
      RewardedActionManager.Instance.PendingActionRewards = RewardedActionManager.Instance.ResolveTagRewardCollisions(RewardedActionManager.Instance.PendingActionRewards);
      SharedMinigameView.ShowContinueButtonOffset(HandleChallengeResultViewClosed,
        Localization.GetWithArgs(LocalizationKeys.kRewardCollectCollectEnergy, RewardedActionManager.Instance.TotalPendingEnergy),
        Localization.Get(LocalizationKeys.kRewardCollectInstruction),
        UIColorPalette.EnergyTextColor,
        "game_results_continue_button");
      _ChallengeEndViewInstance = _SharedMinigameViewInstance.ShowChallengeEndedSlide(_ChallengeData);
      _ChallengeEndViewInstance.DisplayRewards();
    }
    else {
      HandleChallengeResultViewClosed();
    }
  }

  private void HandleChallengeResultViewClosed() {
    // Turn off lights on robot
    if (CurrentRobot != null) {
      CurrentRobot.TurnOffAllLights();
    }

    // Close minigame UI
    CloseMinigame();

    if (_EndState == EndState.PlayerWin) {
      DAS.Event(DASConstants.Game.kEndWithRank, DASConstants.Game.kRankPlayerWon);
      if (OnMiniGameWin != null) {
        OnMiniGameWin();
      }
    }
    else {
      DAS.Event(DASConstants.Game.kEndWithRank, DASConstants.Game.kRankPlayerLose);
      if (OnMiniGameLose != null) {
        OnMiniGameLose();
      }
    }
    DAS.Event("game.end.score", PlayerRoundsWon.ToString(), DASUtil.FormatExtraData(CozmoRoundsWon.ToString()));

    Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Game_End);

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

  #endregion

  // end Difficulty Select

  #region LightCubes

  public void SetLEDs(int id, Color[] color) {
    if (CurrentRobot != null) {
      LightCube cube = null;
      CurrentRobot.LightCubes.TryGetValue(id, out cube);
      if (cube != null) {
        cube.SetLEDs(color);
      }
    }
  }

  public void SetLEDs(int id, Color color) {
    if (CurrentRobot != null) {
      LightCube cube = null;
      CurrentRobot.LightCubes.TryGetValue(id, out cube);
      if (cube != null) {
        cube.SetLEDs(color);
      }
    }
  }

  public float GetCycleDurationSeconds(int numRotations, float cycleIntervalSeconds) {
    return (numRotations * cycleIntervalSeconds * 4);
  }

  public void StartCycleCube(int cubeID, Color[] lightColorsCounterclockwise, float cycleIntervalSeconds) {
    StartCycleCubeInternal(cubeID, lightColorsCounterclockwise, cycleIntervalSeconds, false);
  }

  public void StartCycleCubeSingleColor(int cubeID, Color[] lightColors, float cycleIntervalSeconds, Color rotateColor) {
    StartCycleCubeInternal(cubeID, lightColors, cycleIntervalSeconds, true);
    if (_CubeCycleTimers.ContainsKey(cubeID)) {
      _CubeCycleTimers[cubeID].cycleSingleColorOnly = true;
      _CubeCycleTimers[cubeID].singleColor = rotateColor.ToUInt();
    }
  }

  private void StartCycleCubeInternal(int cubeID, Color[] lightColorsCounterclockwise, float cycleIntervalSeconds, bool isSingleColor) {
    // Remove from blink lights if it exists there
    StopBlinkLight(cubeID);

    // Set colors
    LightCube cube;

    if (CurrentRobot == null || !CurrentRobot.LightCubes.TryGetValue(cubeID, out cube)) {
      DAS.Warn("GameBase.StartCycleCubeInternal", "No lightcube with ID " + cubeID + " cube probably disconnected");
      return;
    }

    cube.SetLEDs(lightColorsCounterclockwise);
    if (isSingleColor) {
      cube.rotationPeriodMs = 0f;
    }
    else {
      cube.rotationPeriodMs = cycleIntervalSeconds * 1000f;
    }

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

  public void StopCycleCube(int cubeID) {
    if (_CubeCycleTimers.ContainsKey(cubeID)) {
      _CubeCycleTimers.Remove(cubeID);
    }
  }

  private void UpdateCubeCycleLights() {
    foreach (KeyValuePair<int, CycleData> kvp in _CubeCycleTimers) {
      kvp.Value.timeElaspedSeconds += Time.deltaTime;

      if (kvp.Value.timeElaspedSeconds > kvp.Value.cycleIntervalSeconds) {
        if (kvp.Value.cycleSingleColorOnly) {
          CycleLightsSingleColor(kvp.Value);
        }
        else {
          // This is now handled engine-side
          // CycleLightsClockwise(kvp.Value);
        }
        kvp.Value.timeElaspedSeconds %= kvp.Value.cycleIntervalSeconds;
      }
    }
  }

  private void CycleLightsSingleColor(CycleData data) {
    if (CurrentRobot == null) {
      return;
    }

    LightCube cube = null;
    CurrentRobot.LightCubes.TryGetValue(data.cubeID, out cube);
    if (cube == null) {
      return;
    }

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

  public void BlinkLight(int cubeId, float duration, Color blinkColor, Color originalColor) {
    BlinkLight(cubeId, duration, new Color[] { blinkColor }, new Color[] { originalColor });
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
    LightCube cube = null;
    CurrentRobot.LightCubes.TryGetValue(cubeId, out cube);
    if (cube == null) {
      return;
    }

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
      LightCube cube = null;
      CurrentRobot.LightCubes.TryGetValue(cubeId, out cube);
      if (cube == null) {
        return;
      }
      cube.SetLEDs(_BlinkCubeTimers[cubeId].originalColor);
      _BlinkCubeTimers.Remove(cubeId);
    }
  }

  private void UpdateBlinkLights() {
    List<int> cubesToStopBlinking = new List<int>();
    foreach (KeyValuePair<int, BlinkData> cubeIdToTimer in _BlinkCubeTimers) {
      cubeIdToTimer.Value.timeElaspedSeconds += Time.deltaTime;
      if (cubeIdToTimer.Value.timeElaspedSeconds > cubeIdToTimer.Value.blinkDuration) {
        cubesToStopBlinking.Add(cubeIdToTimer.Key);
      }
    }

    foreach (int cubeId in cubesToStopBlinking) {
      StopBlinkLight(cubeId);
    }
  }

  public void HandleLightCubeRemoved(LightCube cube) {
    // If this game was using a cube that was removed, quit the game and
    // for all other cubes the game is using make sure to turn their lights off
    foreach (int id in CubeIdsForGame) {
      if (id == cube.ID) {
        ShowInterruptionQuitGameView(LocalizationKeys.kMinigameLostTrackOfBlockTitle,
                                     LocalizationKeys.kMinigameLostTrackOfBlockDescription);

        // Remove this handler in the case multiple lightcubes are removed at the
        // same time we don't show multiple QuitGameViews
        if (CurrentRobot != null) {
          CurrentRobot.OnLightCubeRemoved -= HandleLightCubeRemoved;
        }
      }
      else {
        if (CurrentRobot != null) {
          LightCube lightCube;
          CurrentRobot.LightCubes.TryGetValue(id, out lightCube);
          if (lightCube != null) {
            lightCube.SetLEDsOff();
          }
        }
      }
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

  public float GetGameTimeElapsedInSeconds() {
    return Time.time - _GameStartTime;
  }

  #endregion

  // end DAS

  #region Cliff or Pickup handling

  protected void RegisterRobotReactionaryBehaviorEvents() {
    DeregisterRobotReactionaryBehaviorEvents();
    RobotEngineManager.Instance.AddCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
  }

  protected void DeregisterRobotReactionaryBehaviorEvents() {
    RobotEngineManager.Instance.RemoveCallback<ReactionaryBehaviorTransition>(HandleRobotReactionaryBehavior);
  }

  protected void HandleRobotReactionaryBehavior(object messageObject) {
    ReactionaryBehaviorTransition behaviorTransition = messageObject as ReactionaryBehaviorTransition;

    // log das event
    string currentStateString = "";
    State currState = _StateMachine.GetCurrState();
    if (currState != null) {
      currentStateString = currState.GetType().ToString();
    }
    DAS.Event("robot.interrupt", currentStateString, DASUtil.FormatExtraData(behaviorTransition.reactionaryBehaviorType.ToString()));

    if (behaviorTransition.behaviorStarted) {
      PauseStateMachine(State.PauseReason.ENGINE_MESSAGE, behaviorTransition.reactionaryBehaviorType);
    }
    else {
      ResumeStateMachine(State.PauseReason.ENGINE_MESSAGE, behaviorTransition.reactionaryBehaviorType);
    }
  }

  public void ShowDontMoveCozmoAlertView() {
    ShowInterruptionQuitGameView(LocalizationKeys.kMinigameDontMoveCozmoTitle, LocalizationKeys.kMinigameDontMoveCozmoDescription);
  }

  public void ShowInterruptionQuitGameView(string titleKey, string descriptionKey) {
    _StateMachine.Stop();

    SoftEndGameRobotReset();

    if (_SharedMinigameViewInstance != null) {
      _SharedMinigameViewInstance.HideQuitButton();
    }
    // Don't set everything on fire if its already on fire, that's a waste of perfectly good fire
    if (_InterruptedAlertView == null && PauseManager.Instance.IsAnyDialogOpen == false) {
      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonQuitGame, null);
      alertView.ViewCloseAnimationFinished += HandleInterruptionQuitGameViewClosed;
      alertView.TitleLocKey = titleKey;
      alertView.DescriptionLocKey = descriptionKey;
      _InterruptedAlertView = alertView;
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Game_End);
    }
  }

  private void HandlePauseManagerDialogOpened() {
    if (_InterruptedAlertView != null) {
      _InterruptedAlertView.CloseViewImmediately();
    }
  }

  private void HandleInterruptionQuitGameViewClosed() {
    RaiseMiniGameQuit();
  }

  #endregion
}
