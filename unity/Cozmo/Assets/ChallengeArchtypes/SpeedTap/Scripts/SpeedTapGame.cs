using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo.Audio;
using Cozmo.Util;
using System.Collections.Generic;

namespace SpeedTap {

  public class SpeedTapGame : GameBase {

    public LightCube CozmoBlock;
    public LightCube PlayerBlock;
    public bool PlayerTap = false;
    public bool AllRoundsOver = false;
    // Used for matching animation values and fixing cozmo's position after animations play
    public float CozmoAdjustTime = 0.0f;
    public float CozmoAdjustSpeed = 0.0f;
    public float CozmoAdjustTimeLeft = 0.0f;

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

    public ISpeedTapRules Rules;

    private const float _kRetreatSpeed = -50.0f;
    private const float _kRetreatTime = 1.5f;

    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _Rounds;
    private int _MaxScorePerRound;
    // how many rounds were close in score? used to calculate
    // excitement score rewards.
    private int _CloseRoundCount = 0;

    private List<DifficultySelectOptionData> _DifficultyOptions;

    public event Action PlayerTappedBlockEvent;

    [SerializeField]
    private GameObject _PlayerTapSlidePrefab;

    public void ResetScore() {
      _CozmoScore = 0;
      _PlayerScore = 0;
      UpdateUI();
    }

    public void CozmoWinsHand() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
      _CozmoScore++;
      CheckRounds();
      UpdateUI();
    }

    public void PlayerWinsHand() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
      _PlayerScore++;
      CheckRounds();
      UpdateUI();
    }

    public void PlayerLosesHand() {
      CozmoWinsHand();
    }

    private void HandleRoundRetreatDone(bool success) {
      int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
      int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
      int roundsLeft = _Rounds - losingScore - winningScore;
      if (winningScore > losingScore + roundsLeft) {
        AllRoundsOver = true;
        if (_PlayerRoundsWon > _CozmoRoundsWon) {
          _StateMachine.SetNextState(new AnimationState(RandomLoseSession(), HandleSessionAnimDone));
        }
        else {
          _StateMachine.SetNextState(new AnimationState(RandomWinSession(), HandleSessionAnimDone));
        }
      }
      else {
        ResetScore();
        _StateMachine.SetNextState(new SpeedTapWaitForCubePlace(false));
      }
    }

    private void HandleSessionAnimDone(bool success) {
      if (_PlayerRoundsWon > _CozmoRoundsWon) {
        RaiseMiniGameWin();
      }
      else {
        RaiseMiniGameLose();
      }
    }

    private void CheckRounds() {
      if (_CozmoScore >= _MaxScorePerRound || _PlayerScore >= _MaxScorePerRound) {
        
        if (Mathf.Abs(_PlayerScore - _CozmoScore) < 2) {
          _CloseRoundCount++;
        }

        if (_PlayerScore > _CozmoScore) {
          _PlayerRoundsWon++;

          if (CurrentDifficulty > DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted) {
            DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted = CurrentDifficulty;
            DataPersistence.DataPersistenceManager.Instance.Save();
          }          

          if (_DifficultyOptions.LastOrDefault().DifficultyId > CurrentDifficulty) {
            CurrentDifficulty++;
          }
            
          _StateMachine.SetNextState(new SteerState(_kRetreatSpeed, _kRetreatSpeed, _kRetreatTime, new AnimationState(RandomLoseRound(), HandleRoundRetreatDone)));
        }
        else {
          _CozmoRoundsWon++;
          _StateMachine.SetNextState(new SteerState(_kRetreatSpeed, _kRetreatSpeed, _kRetreatTime, new AnimationState(RandomWinRound(), HandleRoundRetreatDone)));
        }
      }
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      SpeedTapGameConfig speedTapConfig = minigameConfig as SpeedTapGameConfig;
      _Rounds = speedTapConfig.Rounds;
      _MaxScorePerRound = speedTapConfig.MaxScorePerRound;
      _DifficultyOptions = speedTapConfig.DifficultyOptions;
      InitializeMinigameObjects(speedTapConfig.NumCubesRequired());
    }

    // Use this for initialization
    protected void InitializeMinigameObjects(int cubesRequired) { 

      InitialCubesState initCubeState = new InitialCubesState(
                                          new SelectDifficultyState(
                                            new SpeedTapWaitForCubePlace(true),
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

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
    }

    protected override void OnDifficultySet(int difficulty) {
      Rules = GetRules((SpeedTapRuleSet)difficulty);
    }

    protected override void CleanUpOnDestroy() {

      LightCube.TappedAction -= BlockTapped;
      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silence);
    }

    public void InitialCubesDone() {
      CozmoBlock = GetClosestAvailableBlock();
      PlayerBlock = GetFarthestAvailableBlock();
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

    public void RollingBlocks() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLightup);
    }

    private void UIButtonTapped() {
      if (PlayerTappedBlockEvent != null) {
        PlayerTappedBlockEvent();
      }
    }

    private void BlockTapped(int blockID, int tappedTimes) {
      if (PlayerBlock != null && PlayerBlock.ID == blockID) {
        if (PlayerTappedBlockEvent != null) {
          PlayerTappedBlockEvent();
        }
      }
    }

    private LightCube GetClosestAvailableBlock() {
      float minDist = float.MaxValue;
      ObservedObject closest = null;

      for (int i = 0; i < CubesForGame.Count; ++i) {
        if (CubesForGame[i] != PlayerBlock) {
          float d = Vector3.Distance(CubesForGame[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = CubesForGame[i];
          }
        }
      }
      return closest as LightCube;
    }

    private LightCube GetFarthestAvailableBlock() {
      float maxDist = 0;
      ObservedObject farthest = null;

      for (int i = 0; i < CubesForGame.Count; ++i) {
        if (CubesForGame[i] != CozmoBlock) {
          float d = Vector3.Distance(CubesForGame[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d >= maxDist) {
            maxDist = d;
            farthest = CubesForGame[i];
          }
        }
      }
      return farthest as LightCube;
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

    public void ShowPlayerTapSlide() {
      SharedMinigameView.ShowNarrowGameStateSlide(_PlayerTapSlidePrefab, "PlayerTapSlide");
    }

    #region RandomAnims

    // Temp Functions for random animation until anim groups are ready

    // Moves cozmo based on a certain speed and amount of time
    public void CozmoAdjust() {
      if ((CozmoAdjustTime != 0.0f) && (CozmoAdjustTimeLeft <= 0.0f)) {
        CozmoAdjustTimeLeft = CozmoAdjustTime;
        CozmoAdjustTime = 0.0f;
        CurrentRobot.DriveWheels(CozmoAdjustSpeed, CozmoAdjustSpeed);
      }
    }

    public string RandomWinHand() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_winHand_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_winHand_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_winHand_03;
        break;
      }
      return animName;
    }

    public string RandomLoseHand() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_loseHand_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_loseHand_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_loseHand_03;
        break;
      }
      return animName;
    }

    public string RandomWinRound() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_winRound_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_winRound_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_winRound_03;
        break;
      }
      return animName;
    }

    public string RandomLoseRound() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_loseRound_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_loseRound_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_loseRound_03;
        break;
      }
      return animName;
    }

    public string RandomWinSession() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_winSession_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_winSession_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_winSession_03;
        break;
      }
      return animName;
    }

    public string RandomLoseSession() {
      string animName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        animName = AnimationName.kSpeedTap_loseSession_01;
        break;
      case 1:
        animName = AnimationName.kSpeedTap_loseSession_02;
        break;
      default:
        animName = AnimationName.kSpeedTap_loseSession_03;
        break;
      }
      return animName;
    }

    public string RandomTap() {
      string tapName = "";
      CozmoAdjustTime = 0.0f;
      CozmoAdjustSpeed = 0.0f;
      int roll = UnityEngine.Random.Range(0, 4);
      switch (roll) {
      case 0:
        CozmoAdjustTime = 0.132f;
        CozmoAdjustSpeed = 51.0f;
        tapName = AnimationName.kSpeedTap_Tap_01;
        break;
      case 1:
        CozmoAdjustTime = 0.132f;
        CozmoAdjustSpeed = -51.0f;
        tapName = AnimationName.kSpeedTap_Tap_02;
        break;
      default:
        tapName = AnimationName.kSpeedTap_Tap_03;
        break;
      }
      return tapName;
    }

    #endregion
  }
}
