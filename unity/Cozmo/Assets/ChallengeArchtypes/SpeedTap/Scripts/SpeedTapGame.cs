using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo.Audio;
using Cozmo.Util;
using System.Collections.Generic;

namespace SpeedTap {

  public class SpeedTapGame : GameBase {

    private const float _kTapAdjustRange = 5.0f;

    private const string _kWrongTapChance = "WrongTapChance";
    private const string _kTapDelayMin = "TapDelayMin";
    private const string _kTapDelayMax = "TapDelayMax";

    #region Config Values

    public float BaseMatchChance { get; private set; }

    public float MatchChanceIncrease { get; private set; }

    public float CurrentMatchChance { get; set; }

    public float MinIdleIntervalMs { get; private set; }

    public float MaxIdleIntervalMs { get; private set; }

    public float MinTapDelayMs { get; private set; }

    public float MaxTapDelayMs { get; private set; }

    public float CozmoMistakeChance { get; private set; }

    public float CozmoFakeoutChance { get; private set; }

    #endregion

    private Vector3 _CozmoPos;
    private Quaternion _CozmoRot;

    public LightCube CozmoBlock;
    public LightCube PlayerBlock;

    public readonly Color[] PlayerWinColors = new Color[4];
    public readonly Color[] CozmoWinColors = new Color[4];

    public Color PlayerWinColor { 
      get { 
        return PlayerWinColors[0]; 
      } 
      set {
        PlayerWinColors.Fill(value);
      }
    }

    public Color CozmoWinColor { 
      get { 
        return CozmoWinColors[0]; 
      } 
      set {
        CozmoWinColors.Fill(value);
      }
    }

    private List<SpeedTapDifficultyData> _AllDifficultySettings;
    private SpeedTapDifficultyData _CurrentDifficultySettings;

    public SpeedTapDifficultyData CurrentDifficultySettings {
      get {
        return _CurrentDifficultySettings;
      }
    }


    private MusicStateWrapper _BetweenRoundsMusic;

    public MusicStateWrapper BetweenRoundsMusic { get { return _BetweenRoundsMusic; } }

    public SpeedTapRulesBase Rules;

    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _Rounds;

    public int CurrentRound {
      get {
        return _PlayerRoundsWon + _CozmoRoundsWon;
      }
    }

    private int _MaxScorePerRound;

    public event Action PlayerTappedBlockEvent;
    public event Action CozmoTappedBlockEvent;

    [SerializeField]
    private GameObject _PlayerTapSlidePrefab;

    [SerializeField]
    private GameObject _PlayerTapRoundBeginSlidePrefab;

    [SerializeField]
    private GameObject _WaitForCozmoSlidePrefab;

    public void ResetScore() {
      _CozmoScore = 0;
      _PlayerScore = 0;
      UpdateUI();
    }

    public void AddCozmoPoint() {
      _CozmoScore++;
    }

    public void AddPlayerPoint() {
      _PlayerScore++;
    }

    public bool IsRoundComplete() {
      return (_CozmoScore >= _MaxScorePerRound || _PlayerScore >= _MaxScorePerRound);
    }

    public void UpdateRoundScore() {
      if (_PlayerScore > _CozmoScore) {
        _PlayerRoundsWon++;
      }
      else {
        _CozmoRoundsWon++;
      }
    }

    public bool IsHighIntensityRound() {
      int oneThirdRoundsTotal = _Rounds / 3;
      return (_PlayerRoundsWon + _CozmoRoundsWon) > oneThirdRoundsTotal;
    }

    public bool IsGameComplete() {
      int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
      int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
      int roundsLeft = _Rounds - losingScore - winningScore;
      return (winningScore > losingScore + roundsLeft);
    }

    public bool IsHighIntensityGame() {
      int twoThirdsRoundsTotal = _Rounds / 3 * 2;
      return (_PlayerRoundsWon + _CozmoRoundsWon) > twoThirdsRoundsTotal;
    }

    public void HandleGameEnd() {
      if (_PlayerRoundsWon > _CozmoRoundsWon) {
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

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      SpeedTapGameConfig speedTapConfig = minigameConfig as SpeedTapGameConfig;
      // Set all Config based values
      _Rounds = speedTapConfig.Rounds;
      _MaxScorePerRound = speedTapConfig.MaxScorePerRound;
      _AllDifficultySettings = speedTapConfig.DifficultySettings;
      _BetweenRoundsMusic = speedTapConfig.BetweenRoundMusic;
      BaseMatchChance = speedTapConfig.BaseMatchChance;
      CurrentMatchChance = BaseMatchChance;
      MatchChanceIncrease = speedTapConfig.MatchChanceIncrease;
      MinIdleIntervalMs = speedTapConfig.MinIdleIntervalMs;
      MaxIdleIntervalMs = speedTapConfig.MaxIdleIntervalMs;
      CozmoFakeoutChance = speedTapConfig.CozmoFakeoutChance;

      CozmoMistakeChance = SkillSystem.Instance.GetSkillVal(_kWrongTapChance);
      MinTapDelayMs = SkillSystem.Instance.GetSkillVal(_kTapDelayMin);
      MaxTapDelayMs = SkillSystem.Instance.GetSkillVal(_kTapDelayMax);

      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapStarted);
      // End config based values
      InitializeMinigameObjects(1);
    }

    private int HighestLevelCompleted() {
      int difficulty = 0;
      if (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GameDifficulty.TryGetValue(_ChallengeData.ChallengeID, out difficulty)) {
        return difficulty;
      }
      return 0;
    }

    // Use this for initialization
    protected void InitializeMinigameObjects(int cubesRequired) { 

      InitialCubesState initCubeState = new InitialCubesState(
                                          new SelectDifficultyState(
                                            new HowToPlayState(new SpeedTapCozmoDriveToCube(true)),
                                            DifficultyOptions,
                                            HighestLevelCompleted()
                                          ), 
                                          cubesRequired);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
    }

    // Set up Difficulty Settings that are not round specific
    protected override void OnDifficultySet(int difficulty) {
      Rules = GetRules((SpeedTapRuleSet)difficulty);
      _CurrentDifficultySettings = null;
      for (int i = 0; i < _AllDifficultySettings.Count; i++) {
        if (_AllDifficultySettings[i].DifficultyID == difficulty) {
          _CurrentDifficultySettings = _AllDifficultySettings[i];
        }
      }
      if (_CurrentDifficultySettings == null) {
        DAS.Warn("SpeedTapGame.OnDifficultySet.NoValidSettingFound", string.Empty);
        _CurrentDifficultySettings = _AllDifficultySettings[0];
      }
      else {
        if (_CurrentDifficultySettings.Colors != null && _CurrentDifficultySettings.Colors.Length > 0) {
          Rules.SetUsableColors(_CurrentDifficultySettings.Colors);
        }
      }
    }

    protected override void CleanUpOnDestroy() {
      LightCube.TappedAction -= BlockTapped;
      RobotEngineManager.Instance.OnRobotPickedUp -= HandleCozmoPickedUpDisruption;
      LightCube.OnMovedAction -= HandleCozmoCubeMovedDisruption;
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapGetOut);
    }

    public void InitialCubesDone() {
      CozmoBlock = CurrentRobot.LightCubes[CubeIdsForGame[0]];
    }

    public void SetPlayerCube(LightCube cube) {
      PlayerBlock = cube;
      CubeIdsForGame.Add(cube);
    }

    public void UpdateUI() {
      int halfTotalRounds = (_Rounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = _CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = _CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = _PlayerScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = _PlayerRoundsWon;

      // Display the current round
      SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, _CozmoRoundsWon + _PlayerRoundsWon + 1);
    }

    public void UpdateUIForGameEnd() {
      // Hide Current Round at end
      SharedMinigameView.InfoTitleText = string.Empty;
    }

    private void BlockTapped(int blockID, int tappedTimes) {
      if (PlayerBlock != null && PlayerBlock.ID == blockID) {
        if (PlayerTappedBlockEvent != null) {
          PlayerTappedBlockEvent();
        }
      }
      else if (CozmoBlock != null && CozmoBlock.ID == blockID) {
        if (CozmoTappedBlockEvent != null) {
          CozmoTappedBlockEvent();
        }
      }
    }

    private static SpeedTapRulesBase GetRules(SpeedTapRuleSet ruleSet) {
      switch (ruleSet) {
      case SpeedTapRuleSet.NoRed:
        return new NoRedSpeedTapRules();
      case SpeedTapRuleSet.TwoColor:
        return new TwoColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountMultiColor:
        return new LightCountMultiColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountTwoColor:
        return new LightCountTwoColorSpeedTapRules();
      default:
        return new NoRedSpeedTapRules();
      }
    }

    public void ShowPlayerTapConfirmSlide() {
      SharedMinigameView.ShowWideGameStateSlide(_PlayerTapSlidePrefab, "PlayerTapConfirmSlide");
    }

    public void ShowPlayerTapNewRoundSlide() {
      SharedMinigameView.ShowWideGameStateSlide(_PlayerTapRoundBeginSlidePrefab, "PlayerTapNewRoundSlide");
    }

    public void ShowWaitForCozmoSlide() {
      SharedMinigameView.ShowWideGameStateSlide(_WaitForCozmoSlidePrefab, "WaitForCozmoSlide");
    }

    public void SetCozmoOrigPos() {
      _CozmoPos = CurrentRobot.WorldPosition;
      _CozmoRot = CurrentRobot.Rotation;
    }

    // TODO: Reset _CozmoPos and _CozmoRot whenever we receive a deloc message to prevent COZMO-
    public void CheckForAdjust(RobotCallback adjustCallback = null) {
      float dist = 0.0f;
      dist = (CurrentRobot.WorldPosition - _CozmoPos).magnitude;
      if (dist > _kTapAdjustRange) {
        CurrentRobot.GotoPose(_CozmoPos, _CozmoRot, false, false, adjustCallback);
      }
      else {
        if (adjustCallback != null) {
          adjustCallback.Invoke(false);
        }
      }
    }

    protected override void RaiseMiniGameQuit() {
      base.RaiseMiniGameQuit();
      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", _CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", _CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, _PlayerScore.ToString(), null, quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, _PlayerRoundsWon.ToString(), null, quitGameRoundsWonKeyValues);
    }

    public SpeedTapRoundData GetCurrentRoundData() {
      return CurrentDifficultySettings.SpeedTapRoundSettings[CurrentRound];
    }

    public float GetLightsOffDurationSec() {
      SpeedTapRoundData currRoundData = GetCurrentRoundData();
      return currRoundData.SecondsBetweenHands;
    }

    public float GetLightsOnDurationSec() {
      SpeedTapRoundData currRoundData = GetCurrentRoundData();
      return currRoundData.SecondsHandDisplayed;
    }

    public void StartRoundMusic() {
      SpeedTapRoundData currRoundData = GetCurrentRoundData();
      if (currRoundData.MidRoundMusic != Anki.Cozmo.Audio.GameState.Music.Invalid) {
        GameAudioClient.SetMusicState(currRoundData.MidRoundMusic);
      }
      else {
        GameAudioClient.SetMusicState(GetDefaultMusicState());
      }
    }

    // TODO: Promote to GameBase?
    public void StartCozmoPickedUpDisruptionDetection() {
      RobotEngineManager.Instance.OnRobotPickedUp -= HandleCozmoPickedUpDisruption;
      RobotEngineManager.Instance.OnRobotPickedUp += HandleCozmoPickedUpDisruption;
    }

    // TODO: Promote to GameBase?
    public void EndCozmoPickedUpDisruptionDetection() {
      RobotEngineManager.Instance.OnRobotPickedUp -= HandleCozmoPickedUpDisruption;
    }

    // TODO: Promote to GameBase?
    private void HandleCozmoPickedUpDisruption() {
      RobotEngineManager.Instance.OnRobotPickedUp -= HandleCozmoPickedUpDisruption;
      ShowPleaseDontCheatAlertView(LocalizationKeys.kSpeedTapDontMoveCozmoTitle, LocalizationKeys.kSpeedTapDontMoveCozmoDescription);
    }

    public void StartCozmoCubeMovedDisruptionDetection() {
      // INGO: Temporarily disabling broken code to unblock QA
      // LightCube.OnMovedAction -= HandleCozmoCubeMovedDisruption;
      // LightCube.OnMovedAction += HandleCozmoCubeMovedDisruption;
    }

    public void EndCozmoCubeMovedDisruptionDetection() {
      // INGO: Temporarily disabling broken code to unblock QA
      // LightCube.OnMovedAction -= HandleCozmoCubeMovedDisruption;
    }

    private void HandleCozmoCubeMovedDisruption(int cubeId, float xAccel, float yAccel, float zAccel) {
      if (cubeId == CozmoBlock.ID) {
        LightCube.OnMovedAction -= HandleCozmoCubeMovedDisruption;
        ShowPleaseDontCheatAlertView(LocalizationKeys.kSpeedTapDontMoveBlockTitle, LocalizationKeys.kSpeedTapDontMoveBlockDescription);
      }
    }

    private void ShowPleaseDontCheatAlertView(string titleKey, string descriptionKey) {
      LightCube.OnMovedAction -= HandleCozmoCubeMovedDisruption;
      RobotEngineManager.Instance.OnRobotPickedUp -= HandleCozmoPickedUpDisruption;
      EndGameRobotReset();
      PauseGame();

      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: false);
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonQuitGame, HandlePleaseDontCheatAlertViewClosed);
      alertView.ViewClosed += HandlePleaseDontCheatAlertViewClosed;
      alertView.TitleLocKey = titleKey;
      alertView.DescriptionLocKey = descriptionKey;
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
    }

    private void HandlePleaseDontCheatAlertViewClosed() {
      RaiseMiniGameQuit();
    }
  }
}
