using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo;
using Anki.Cozmo.Audio;
using Cozmo.Util;
using System.Collections.Generic;

namespace SpeedTap {

  public class SpeedTapPlayerInfo : PlayerInfo {
    public int CubeID { get; set; }
    public float LastTapTimeStamp { get; set; }
    public readonly Color[] PlayerWinColors = new Color[4];
    public Cozmo.MinigameWidgets.ScoreWidget scoreWidget;
    public int MistakeTapsTotal { get; set; }
    public int CorrectTapsTotal { get; set; }
    public SpeedTapPlayerInfo(PlayerType playerType, string name) : base(playerType, name) {
      CubeID = -1;
      LastTapTimeStamp = -1;
      MistakeTapsTotal = 0;
      CorrectTapsTotal = 0;
    }
    public Color PlayerWinColor {
      get {
        return PlayerWinColors[0];
      }
      set {
        PlayerWinColors.Fill(value);
      }
    }
  }

  public class SpeedTapGame : GameBase {

    private const float _kTapAdjustRange = 5.0f;

    private const string _kWrongTapChance = "WrongTapChance";
    private const string _kTapDelayMin = "TapDelayMin";
    private const string _kTapDelayMax = "TapDelayMax";

    public void ResetTapTimestamps() {
      for (int i = 0; i < _PlayerInfo.Count; ++i) {
        SpeedTapPlayerInfo player = (SpeedTapPlayerInfo)_PlayerInfo[i];
        player.LastTapTimeStamp = -1.0f;
      }
    }

    #region Config Values

    public float BaseMatchChance { get; private set; }

    public float MatchChanceIncrease { get; private set; }

    public float CurrentMatchChance { get; set; }

    public float MinIdleInterval_percent { get; private set; }

    public float MaxIdleInterval_percent { get; private set; }

    public float MinTapDelay_percent { get; private set; }

    public float MaxTapDelay_percent { get; private set; }

    public float CozmoMistakeChance { get; private set; }

    public float CozmoFakeoutChance { get; private set; }

    public float TapResolutionDelay { get; private set; }

    public float MPTimeBetweenRoundsSec { get { return _GameConfig.MPTimeBetweenRoundsSec; } }

    private SpeedTapGameConfig _GameConfig;

    #endregion

    private Vector3 _CozmoPos;
    private Quaternion _CozmoRot;

    public bool RedMatch = false;

    private List<SpeedTapDifficultyData> _AllDifficultySettings;
    private SpeedTapDifficultyData _CurrentDifficultySettings;

    public SpeedTapDifficultyData CurrentDifficultySettings {
      get {
        return _CurrentDifficultySettings;
      }
    }


    private MusicStateWrapper _BetweenRoundsMusic;

    public MusicStateWrapper BetweenRoundsMusic { get { return _BetweenRoundsMusic; } }

    public DefaultSpeedTapRules Rules;

    [SerializeField]
    private GameObject _PlayerTapSlidePrefab;

    [SerializeField]
    private GameObject _PlayerTapRoundBeginSlidePrefab;

    [SerializeField]
    private GameObject _WaitForCozmoSlidePrefab;

    [SerializeField]
    private SpeedTapNumPlayerTypeSelectSlide _NumPlayersSelectSlidePrefab;
    public SpeedTapNumPlayerTypeSelectSlide SpeedTapNumPlayersSelectSlidePrefab {
      get { return _NumPlayersSelectSlidePrefab; }
    }

    [SerializeField]
    private SpeedTapRoundBeginSlide _SpeedTapRoundBeginSlidePrefab;

    public SpeedTapRoundBeginSlide SpeedTapRoundBeginSlidePrefab {
      get { return _SpeedTapRoundBeginSlidePrefab; }
    }

    [SerializeField]
    private SpeedTapRoundEndSlide _SpeedTapRoundEndSlidePrefab;

    public SpeedTapRoundEndSlide SpeedTapRoundEndSlidePrefab {
      get { return _SpeedTapRoundEndSlidePrefab; }
    }
#if ANKI_DEV_CHEATS
    public static bool sWantsSuddenDeathHumanHuman = false;
    public static bool sWantsSuddenDeathHumanCozmo = false;
#endif
    public override PlayerInfo AddPlayer(PlayerType playerType, string playerName) {
      SpeedTapPlayerInfo info = new SpeedTapPlayerInfo(playerType, playerName);
      _PlayerInfo.Add(info);
      // As a special case, Cozmo always grabs the first cube because it was shown first
      if (playerType == PlayerType.Cozmo && CubeIdsForGame.Count >= 1) {
        info.CubeID = CubeIdsForGame[0];
      }
      return info;
    }
    public override void InitializeAllPlayers() {
      if (_PlayerInfo.Count > 2) {
        SkillSystem.Instance.LevelingEnabled = false;
      }
    }

    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
      SpeedTapGameConfig speedTapConfig = minigameConfigData as SpeedTapGameConfig;
      // Set all Config based values
      TotalRounds = DebugMenuManager.Instance.DemoMode ? 1 : speedTapConfig.Rounds;
      MaxScorePerRound = speedTapConfig.MaxScorePerRound;
      _AllDifficultySettings = speedTapConfig.DifficultySettings;
      _BetweenRoundsMusic = speedTapConfig.BetweenRoundMusic;
      BaseMatchChance = speedTapConfig.BaseMatchChance;
      CurrentMatchChance = BaseMatchChance;
      MatchChanceIncrease = speedTapConfig.MatchChanceIncrease;
      MinIdleInterval_percent = speedTapConfig.MinIdleInterval_percent;
      MaxIdleInterval_percent = speedTapConfig.MaxIdleInterval_percent;
      CozmoFakeoutChance = speedTapConfig.CozmoFakeoutChance;
      TapResolutionDelay = speedTapConfig.TapResolutionDelay;
      _GameConfig = speedTapConfig;

      CozmoMistakeChance = SkillSystem.Instance.GetSkillVal(_kWrongTapChance);
      MinTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMin);
      MaxTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMax);
      // End config based values
      InitializeMinigameObjects(1);
#if ANKI_DEV_CHEATS
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("ShowdownCozmoHuman", "SpeedTap",
                            (string str) => { sWantsSuddenDeathHumanCozmo = !sWantsSuddenDeathHumanCozmo; });
      Anki.Debug.DebugConsoleData.Instance.AddConsoleFunction("ShowdownHumanHuman", "SpeedTap",
                                  (string str) => { sWantsSuddenDeathHumanHuman = !sWantsSuddenDeathHumanHuman; });
#endif
    }

    // Use this for initialization
    protected void InitializeMinigameObjects(int cubesRequired) {
      State lastState = null;
      if (FeatureGate.Instance.IsFeatureEnabled(FeatureType.SpeedTapMultiPlayer)) {
        lastState = new SpeedTapSelectNumPlayersState();
      }
      else {
        lastState = new SpeedTapCubeSelectionState();
      }
      InitialCubesState initCubeState = new InitialCubesState(
                                          new SelectDifficultyState(
                                            lastState,
                                            DifficultyOptions,
                                            HighestLevelCompleted()
                                          ),
                                          cubesRequired);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      CurrentRobot.OnLightCubeRemoved += LightCubeRemoved;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingPets, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingOverheadEdges, false);
    }

    // Set up Difficulty Settings that are not round specific
    protected override void OnDifficultySet(int difficulty) {
      Rules = GetRules(difficulty + 1);
      _CurrentDifficultySettings = null;
      for (int i = 0; i < _AllDifficultySettings.Count; i++) {
        if (_AllDifficultySettings[i].DifficultyID == difficulty) {
          _CurrentDifficultySettings = _AllDifficultySettings[i];
          break;
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
      // In demo mode, skills could have been overwritten
      if (DebugMenuManager.Instance.DemoMode) {
        CozmoMistakeChance = SkillSystem.Instance.GetSkillVal(_kWrongTapChance);
        MinTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMin);
        MaxTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMax);
      }
    }

    protected override void CleanUpOnDestroy() {
      if (CurrentRobot != null) {
        CurrentRobot.OnLightCubeRemoved -= LightCubeRemoved;
      }

      // turn back on the skills leveling system in case MP disabled it.
      SkillSystem.Instance.LevelingEnabled = true;

      Anki.Debug.DebugConsoleData.Instance.RemoveConsoleData("SpeedTap");
    }

    protected override void UpdateScoreboard(bool didPlayerWin) {
      PlayerInfo winner = GetPlayerMostRoundsWon();
      int playerCount = GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)GetPlayerByIndex(i);
        playerInfo.scoreWidget.Dim = false;
        playerInfo.scoreWidget.IsWinner = playerInfo == winner;
      }
    }

    protected override void ShowWinnerState(int currentEndIndex, string overrideWinnerText = null, string footerText = "", bool showWinnerTextInShelf = false) {
      base.ShowWinnerState(currentEndIndex, overrideWinnerText, footerText, GetPlayerCount() > 2);
    }

    public override void UpdateUI() {
      base.UpdateUI();
      int halfTotalRounds = (TotalRounds + 1) / 2;

      int roundsPlayedTotal = 0;
      int numHumans = 0;
      int playerCount = GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)GetPlayerByIndex(i);
        // Because creation involves an animation, can't create these on player creation
        if (playerInfo.scoreWidget == null) {
          if (playerInfo.playerType != PlayerType.Cozmo) {
            numHumans++;
          }
          if (playerInfo.playerType == PlayerType.Cozmo) {
            playerInfo.scoreWidget = SharedMinigameView.CozmoScoreboard;
          }
          else if (numHumans >= 2) {
            playerInfo.scoreWidget = SharedMinigameView.Player2Scoreboard;
            playerInfo.scoreWidget.PortraitColor = _GameConfig.Player2Tint;
            playerInfo.scoreWidget.SetNameLabelText(Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextMPScorePlayer, playerInfo.name));
          }
          else {
            playerInfo.scoreWidget = SharedMinigameView.PlayerScoreboard;
            if (playerCount >= 3) {
              playerInfo.scoreWidget.PortraitColor = _GameConfig.Player1Tint;
              playerInfo.scoreWidget.SetNameLabelText(Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextMPScorePlayer, playerInfo.name));
            }
          }
        }
        playerInfo.scoreWidget.Score = playerInfo.playerScoreRound;
        playerInfo.scoreWidget.MaxRounds = halfTotalRounds;
        playerInfo.scoreWidget.RoundsWon = playerInfo.playerRoundsWon;
        roundsPlayedTotal += playerInfo.playerRoundsWon;
      }
      if (GetPlayerCount() > 2) {
        // Display the current round
        if (IsOvertime()) {
          SharedMinigameView.ShelfWidget.SetWidgetText(Localization.Get(LocalizationKeys.kSpeedTapMultiplayerOverTime));
        }
        else {
          SharedMinigameView.ShelfWidget.SetWidgetText(Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, roundsPlayedTotal + 1));
        }
      }
      else {
        SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, roundsPlayedTotal + 1);
      }
    }

    private List<PlayerInfo> GetPlayersMostPointsWon() {
      List<PlayerInfo> maxScorers = new List<PlayerInfo>();
      int mostPoints = 0;
      for (int i = 0; i < _PlayerInfo.Count; ++i) {
        if (_PlayerInfo[i].playerScoreRound > mostPoints) {
          maxScorers.Clear();
          maxScorers.Add(_PlayerInfo[i]);
          mostPoints = _PlayerInfo[i].playerScoreRound;
        }
        else if (_PlayerInfo[i].playerScoreRound == mostPoints) {
          maxScorers.Add(_PlayerInfo[i]);
        }
      }
      return maxScorers;
    }

    private bool IsOvertime() {
      if (base.IsRoundComplete()) {
        List<PlayerInfo> maxScorers = GetPlayersMostPointsWon();
        // If multiple people have a high score, we need to keep playing until there is only one winner.
        if (maxScorers.Count > 1) {
          return true;
        }
      }
      return false;
    }

    public override bool IsRoundComplete() {
      // Some extra logic to account for overtime when multiple people have top score.
      return base.IsRoundComplete() && !IsOvertime();
    }

    public override void UpdateUIForGameEnd() {
      base.UpdateUIForGameEnd();
      // Hide Current Round at end
      SharedMinigameView.InfoTitleText = string.Empty;
    }

    private void LightCubeRemoved(LightCube cube) {
      // Invalidate the player/cozmo block id if it was removed
      int playerCount = GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)GetPlayerByIndex(i);
        if (playerInfo.CubeID == cube.ID) {
          playerInfo.CubeID = -1;
        }
      }
    }

    private static DefaultSpeedTapRules GetRules(int numLights = 1) {
      return new DefaultSpeedTapRules(numLights);
    }

    public void ShowPlayerTapConfirmSlide(int playerIndex) {
      SpeedTapPlayerTapConfirmSlide slide = SharedMinigameView.ShowWideGameStateSlide(_PlayerTapSlidePrefab, "PlayerTapConfirmSlide").GetComponent<SpeedTapPlayerTapConfirmSlide>();
      if (slide != null) {
        slide.Init(_PlayerInfo.Count, playerIndex);
      }
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

    // TODO: Reset _CozmoPos and _CozmoRot whenever we receive a deloc message to prevent COZMO-829
    public void CheckForAdjust(RobotCallback adjustCallback = null) {
      float dist = 0.0f;
      dist = (CurrentRobot.WorldPosition - _CozmoPos).magnitude;
      if (dist > _kTapAdjustRange) {
        Debug.LogWarning(string.Format("ADJUST : From Pos {0} - Rot {1} : To Pos {2} - Rot {3}", CurrentRobot.WorldPosition, CurrentRobot.Rotation, _CozmoPos, _CozmoRot));
        CurrentRobot.GotoPose(_CozmoPos, _CozmoRot, false, false, adjustCallback);
      }
      else {
        if (adjustCallback != null) {
          adjustCallback.Invoke(false);
        }
      }
    }

    protected override void SendCustomEndGameDasEvents() {
      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, HumanScore.ToString(), quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, HumanRoundsWon.ToString(), quitGameRoundsWonKeyValues);
    }

    public SpeedTapRoundData GetCurrentRoundData() {
      return CurrentDifficultySettings.SpeedTapRoundSettings[Math.Min(RoundsPlayed, CurrentDifficultySettings.SpeedTapRoundSettings.Count - 1)];
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
    public void StartEndMusic() {
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Quick_Tap_Win);
    }

    public void ClearWinningLightPatterns() {

      int playerCount = GetPlayerCount();
      for (int i = 0; i < playerCount; ++i) {
        SpeedTapPlayerInfo playerInfo = (SpeedTapPlayerInfo)GetPlayerByIndex(i);
        if (playerInfo.CubeID != -1) {
          if (CurrentRobot != null) {
            CurrentRobot.LightCubes[playerInfo.CubeID].SetLEDsOff();
          }
        }
      }
    }

    public virtual void AddPoint(List<PlayerInfo> playersScored, bool wasMistakeMade) {
      base.AddPoint(playersScored);
      // extra bookkeeping for if a mistake was made accuracy goal
      for (int i = 0; i < _PlayerInfo.Count; ++i) {
        SpeedTapPlayerInfo speedTapPlayer = (SpeedTapPlayerInfo)_PlayerInfo[i];
        if (wasMistakeMade && !playersScored.Contains(_PlayerInfo[i])) {
          speedTapPlayer.MistakeTapsTotal = speedTapPlayer.MistakeTapsTotal + 1;
        }
        else if (!wasMistakeMade && playersScored.Contains(_PlayerInfo[i])) {
          speedTapPlayer.CorrectTapsTotal = speedTapPlayer.CorrectTapsTotal + 1;
        }
      }
    }

    // event values to add on game completion
    protected override Dictionary<string, float> GetGameSpecificEventValues() {
      Dictionary<string, float> ret = new Dictionary<string, float>();
      float minAccuracy = 1.0f;
      // CorrectScoreTotalCount / TotalScore;
      for (int i = 0; i < _PlayerInfo.Count; ++i) {
        SpeedTapPlayerInfo speedTapPlayer = (SpeedTapPlayerInfo)_PlayerInfo[i];
        if (speedTapPlayer.playerType != PlayerType.Cozmo) {
          float total_taps = (float)(speedTapPlayer.CorrectTapsTotal + speedTapPlayer.MistakeTapsTotal);
          float accuracy = 0.0f;
          // never tapping counts as 0
          if (total_taps > 0) {
            accuracy = ((float)speedTapPlayer.CorrectTapsTotal) / total_taps;
          }
          minAccuracy = Mathf.Min(minAccuracy, accuracy);
        }
      }
      // in MP with take the lowest human score
      ret.Add(ChallengeAccuracyCondition.kConditionKey, minAccuracy);
      return ret;
    }

  }
}
