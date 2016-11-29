using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo;
using Anki.Cozmo.Audio;
using Cozmo.Util;
using System.Collections.Generic;

namespace SpeedTap {

  public enum FirstToTap {
    Cozmo,
    Player,
    NoTaps
  }

  public class SpeedTapGame : GameBase {

    private const float _kTapAdjustRange = 5.0f;

    private const string _kWrongTapChance = "WrongTapChance";
    private const string _kTapDelayMin = "TapDelayMin";
    private const string _kTapDelayMax = "TapDelayMax";

    public FirstToTap FirstTapper {
      get {
        // If neither timestamp has been set, no taps
        if (_LastCozmoTimeStamp == -1 && _LastPlayerTimeStamp == -1) {
          return FirstToTap.NoTaps;
        }
        // If one of the two timestamps hasn't been set, other one counts as first
        if ((_LastCozmoTimeStamp == -1) || (_LastPlayerTimeStamp == -1)) {
          if (_LastCozmoTimeStamp != -1) {
            return FirstToTap.Cozmo;
          }
          else if (_LastPlayerTimeStamp != -1) {
            return FirstToTap.Player;
          }
        }
        // If both have been set, most recent timestamp counts as first
        if (_LastCozmoTimeStamp < _LastPlayerTimeStamp) {
          return FirstToTap.Cozmo;
        }
        else {
          return FirstToTap.Player;
        }
      }
    }

    public void ResetTapTimestamps() {
      _LastCozmoTimeStamp = -1;
      _LastPlayerTimeStamp = -1;
    }

    private float _LastPlayerTimeStamp = -1;
    private float _LastCozmoTimeStamp = -1;
    public float TapResolutionDelay = 0.0f;

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

    #endregion

    private Vector3 _CozmoPos;
    private Quaternion _CozmoRot;

    public bool RedMatch = false;
    public int CozmoBlockID = -1;
    public int PlayerBlockID = -1;

    public readonly Color[] PlayerWinColors = new Color[4];
    public readonly Color[] CozmoWinColors = new Color[4];

    public LightCube GetCozmoBlock() {
      LightCube cube = null;
      if (CurrentRobot != null) {
        CurrentRobot.LightCubes.TryGetValue(CozmoBlockID, out cube);
      }
      return cube;
    }

    public LightCube GetPlayerBlock() {
      LightCube cube = null;
      if (CurrentRobot != null) {
        CurrentRobot.LightCubes.TryGetValue(PlayerBlockID, out cube);
      }
      return cube;
    }

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

    [SerializeField]
    private GameObject _PlayerTapSlidePrefab;

    [SerializeField]
    private GameObject _PlayerTapRoundBeginSlidePrefab;

    [SerializeField]
    private GameObject _WaitForCozmoSlidePrefab;

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

    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
      SpeedTapGameConfig speedTapConfig = minigameConfigData as SpeedTapGameConfig;
      // Set all Config based values
      TotalRounds = speedTapConfig.Rounds;
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

      CozmoMistakeChance = SkillSystem.Instance.GetSkillVal(_kWrongTapChance);
      MinTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMin);
      MaxTapDelay_percent = SkillSystem.Instance.GetSkillVal(_kTapDelayMax);
      // End config based values
      InitializeMinigameObjects(1);

      // Removes engine holding a delay to saveguard against "phantom taps"
      RobotEngineManager.Instance.Message.EnableBlockTapFilter = Singleton<Anki.Cozmo.ExternalInterface.EnableBlockTapFilter>.Instance.Initialize(false);
      RobotEngineManager.Instance.SendMessage();
    }

    // Use this for initialization
    protected void InitializeMinigameObjects(int cubesRequired) {
      InitialCubesState initCubeState = new InitialCubesState(
                                          new SelectDifficultyState(
                                            new SpeedTapCozmoDriveToCube(true),
                                            DifficultyOptions,
                                            HighestLevelCompleted()
                                          ),
                                          cubesRequired);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.OnLightCubeRemoved += LightCubeRemoved;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(OnRobotAnimationEvent);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingPets, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingOverheadEdges, false);
    }

    // Set up Difficulty Settings that are not round specific
    protected override void OnDifficultySet(int difficulty) {
      Rules = GetRules((SpeedTapRuleSet)difficulty);
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
    }

    protected override void CleanUpOnDestroy() {
      LightCube.TappedAction -= BlockTapped;
      if (CurrentRobot != null) {
        CurrentRobot.OnLightCubeRemoved -= LightCubeRemoved;
      }
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.AnimationEvent>(OnRobotAnimationEvent);

      // Adds back in engine holding a delay to saveguard against "phantom taps"
      RobotEngineManager.Instance.Message.EnableBlockTapFilter = Singleton<Anki.Cozmo.ExternalInterface.EnableBlockTapFilter>.Instance.Initialize(true);
      RobotEngineManager.Instance.SendMessage();
    }

    public void InitialCubesDone() {
      CozmoBlockID = CubeIdsForGame[0];
    }

    public void SetPlayerCube(LightCube cube) {
      PlayerBlockID = cube.ID;
      CubeIdsForGame.Add(cube);
    }

    public override void UpdateUI() {
      base.UpdateUI();
      int halfTotalRounds = (TotalRounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = PlayerScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = PlayerRoundsWon;

      // Display the current round
      SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, CozmoRoundsWon + PlayerRoundsWon + 1);
    }

    public override void UpdateUIForGameEnd() {
      base.UpdateUIForGameEnd();
      // Hide Current Round at end
      SharedMinigameView.InfoTitleText = string.Empty;
    }

    /// <summary>
    /// Sets player's last tapped timestamp based on light cube message.
    /// Cozmo uses AnimationEvents for maximum accuracy.
    /// </summary>
    /// <param name="blockID">Block ID.</param>
    /// <param name="tappedTimes">Tapped times.</param>
    /// <param name="timeStamp">Time stamp.</param>
    private void BlockTapped(int blockID, int tappedTimes, float timeStamp) {
      if (PlayerBlockID != -1 && PlayerBlockID == blockID) {
        if (_LastPlayerTimeStamp > timeStamp || _LastPlayerTimeStamp == -1) {
          _LastPlayerTimeStamp = timeStamp;
        }
      }
    }

    /// <summary>
    /// Sets cozmo's last tapped timestamp based on animation event message
    /// </summary>
    /// <param name="animEvent">Animation event.</param>
    private void OnRobotAnimationEvent(Anki.Cozmo.ExternalInterface.AnimationEvent animEvent) {
      if (animEvent.event_id == AnimEvent.TAPPED_BLOCK &&
          (_LastCozmoTimeStamp > animEvent.timestamp || _LastCozmoTimeStamp == -1)) {
        _LastCozmoTimeStamp = animEvent.timestamp;
      }
    }

    private void LightCubeRemoved(LightCube cube) {
      // Invalidate the player/cozmo block id if it was removed
      if (PlayerBlockID == cube.ID) {
        PlayerBlockID = -1;
      }
      if (CozmoBlockID == cube.ID) {
        CozmoBlockID = -1;
      }

    }

    private static SpeedTapRulesBase GetRules(SpeedTapRuleSet ruleSet) {
      switch (ruleSet) {
      case SpeedTapRuleSet.NoRed:
        return new NoRedSpeedTapRules();
      case SpeedTapRuleSet.TwoColor:
        return new TwoColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountThreeColor:
        return new LightCountThreeColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountMultiColor:
        return new LightCountMultiColorSpeedTapRules();
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

      DAS.Event(DASConstants.Game.kQuitGameScore, PlayerScore.ToString(), quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, PlayerRoundsWon.ToString(), quitGameRoundsWonKeyValues);
    }

    public SpeedTapRoundData GetCurrentRoundData() {
      return CurrentDifficultySettings.SpeedTapRoundSettings[RoundsPlayed];
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
      if (PlayerBlockID != -1) {
        StopCycleCube(PlayerBlockID);
        if (CurrentRobot != null) {
          CurrentRobot.LightCubes[PlayerBlockID].SetLEDsOff();
        }
      }

      if (CozmoBlockID != -1) {
        StopCycleCube(CozmoBlockID);
        if (CurrentRobot != null) {
          CurrentRobot.LightCubes[CozmoBlockID].SetLEDsOff();
        }
      }
    }
  }
}
