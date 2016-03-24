using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo.Audio;
using Cozmo.Util;
using System.Collections.Generic;

namespace SpeedTap {

  public class SpeedTapGame : GameBase {

    private const float _kRetreatSpeed = -130.0f;
    private const float _kRetreatTime = 0.30f;
    private const float _kTapAdjustRange = 5.0f;

    private Vector3 _CozmoPos;
    private Quaternion _CozmoRot;

    public LightCube CozmoBlock;
    public LightCube PlayerBlock;
    public bool PlayerHitFirst = false;
    public bool AllRoundsOver = false;

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

    public List<DifficultySelectOptionData> DifficultyOptions {
      get {
        return _DifficultyOptions;
      }
    }

    private MusicStateWrapper _BetweenRoundsMusic;

    public ISpeedTapRules Rules;

    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _Rounds;
    private int _MaxScorePerRound;
    public int ConsecutiveMisses = 0;
    // how many rounds were close in score? used to calculate
    // excitement score rewards.
    private int _CloseRoundCount = 0;

    private List<DifficultySelectOptionData> _DifficultyOptions;

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

    // In Future, potentially play a different animation here
    public void PlayerHitWrong() {
      DAS.Info("SpeedTapGame.PlayerHitWrong", "");
      CozmoWinsHand();
    }

    public void CozmoWinsHand() {
      _CozmoScore++;
      UpdateUI();
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
      PlayerBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      for (int i = 0; i < 4; i++) {
        CozmoBlock.Lights[i].SetFlashingLED(CozmoWinColors[i], 100, 100, 0);
      }
      if (IsRoundComplete()) {
        HandleRoundEnd();
      }
      else {
        _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_WinHand, 
          HandleHandEndAnimDone));
      }
    }

    public void PlayerWinsHand() {
      _PlayerScore++;
      UpdateUI();
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
      CozmoBlock.SetFlashingLEDs(Color.red, 100, 100, 0);
      for (int i = 0; i < 4; i++) {
        PlayerBlock.Lights[i].SetFlashingLED(PlayerWinColors[i], 100, 100, 0);
      }
      if (IsRoundComplete()) {
        HandleRoundEnd();
      }
      else {
        _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseHand, 
          HandleHandEndAnimDone));
      }
    }

    public bool IsRoundComplete() {
      return (_CozmoScore >= _MaxScorePerRound || _PlayerScore >= _MaxScorePerRound);
    }

    public bool IsSessionComplete() {
      int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
      int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
      int roundsLeft = _Rounds - losingScore - winningScore;
      return (winningScore > losingScore + roundsLeft);
    }

    private void HandleRoundEnd() {
      GameAudioClient.SetMusicState(_BetweenRoundsMusic);
      ConsecutiveMisses = 0;
      if (Mathf.Abs(_PlayerScore - _CozmoScore) < 2) {
        _CloseRoundCount++;
      }

      if (_PlayerScore > _CozmoScore) {
        _PlayerRoundsWon++;
      }
      else {
        _CozmoRoundsWon++;
      }
      UpdateUI();
      if (IsSessionComplete()) {
        AllRoundsOver = true;
        if (_PlayerRoundsWon > _CozmoRoundsWon) {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseSession, HandleSessionAnimDone));
        }
        else {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_WinSession, HandleSessionAnimDone));
        }
      }
      else {
        if (_PlayerScore > _CozmoScore) {          
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_LoseRound, HandleRoundEndAnimDone));
        }
        else {
          _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kSpeedTap_WinRound, HandleRoundEndAnimDone));
        }
      }
    }

    private void HandleHandEndAnimDone(bool success) {
      _StateMachine.SetNextState(new SpeedTapStatePlayNewHand());
    }

    private void HandleRoundEndAnimDone(bool success) {
      ResetScore();
      _StateMachine.SetNextState(new SpeedTapWaitForCubePlace(false));
    }

    private void HandleSessionAnimDone(bool success) {
      if (_PlayerRoundsWon > _CozmoRoundsWon) {
        if (CurrentDifficulty >= DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted) {
          DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted = CurrentDifficulty + 1;
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
      _Rounds = speedTapConfig.Rounds;
      _MaxScorePerRound = speedTapConfig.MaxScorePerRound;
      _DifficultyOptions = speedTapConfig.DifficultyOptions;
      _BetweenRoundsMusic = speedTapConfig.BetweenRoundMusic;
      InitializeMinigameObjects(1);
    }

    // Use this for initialization
    protected void InitializeMinigameObjects(int cubesRequired) { 

      InitialCubesState initCubeState = new InitialCubesState(
                                          new SelectDifficultyState(
                                            new HowToPlayState(new SpeedTapWaitForCubePlace(true)),
                                            DifficultyOptions,
                                            Mathf.Max(DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted, 1)
                                          ), 
                                          cubesRequired);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetBehaviorSystem(false);
    }

    protected override void OnDifficultySet(int difficulty) {
      Rules = GetRules((SpeedTapRuleSet)difficulty);
    }

    protected override void CleanUpOnDestroy() {
      LightCube.TappedAction -= BlockTapped;
    }

    public void InitialCubesDone() {
      CozmoBlock = CubesForGame[0];
    }

    public void SetPlayerCube(LightCube cube) {
      PlayerBlock = cube;
      CubesForGame.Add(cube);
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

      if (AllRoundsOver) {
        // Hide Current Round at end
        SharedMinigameView.InfoTitleText = string.Empty;
      }
      else {
        // Display the current round
        SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, _CozmoRoundsWon + _PlayerRoundsWon + 1);
      }
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

    private static ISpeedTapRules GetRules(SpeedTapRuleSet ruleSet) {
      switch (ruleSet) {
      case SpeedTapRuleSet.NoRed:
        return new NoRedSpeedTapRules();
      case SpeedTapRuleSet.TwoColor:
        return new TwoColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountNoColor:
        return new LightCountNoColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountMultiColor:
        return new LightCountMultiColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountSameColorNoTap:
        return new LightCountSameColorNoTap();
      case SpeedTapRuleSet.LightCountTwoColor:
        return new LightCountTwoColorSpeedTapRules();
      default:
        return new DefaultSpeedTapRules();
      }
    }

    protected override int CalculateExcitementStatRewards() {
      return 1 + _CloseRoundCount * 2;
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


  }
}
