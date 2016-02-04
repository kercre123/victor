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

    private int _CurrentDifficulty;
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

    public void ResetScore() {
      _CozmoScore = 0;
      _PlayerScore = 0;
      UpdateUI();
    }

    public void CozmoWinsHand() {
      _CozmoScore++;
      CheckRounds();
      UpdateUI();
    }

    public void PlayerWinsHand() {
      _PlayerScore++;
      CheckRounds();
      UpdateUI();
    }

    public void PlayerLosesHand() {
      CozmoWinsHand();
    }

    private void HandleRoundAnimationDone(bool success) {
      _StateMachine.SetNextState(new SteerState(50.0f, 50.0f, 0.8f, new SpeedTapWaitForCubePlace()));
    }

    private void CheckRounds() {
      if (_CozmoScore >= _MaxScorePerRound || _PlayerScore >= _MaxScorePerRound) {

        if (_PlayerScore > _CozmoScore) {
          _PlayerRoundsWon++;

          if (_CurrentDifficulty > DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted) {
            DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted = _CurrentDifficulty;
            DataPersistence.DataPersistenceManager.Instance.Save();
          }          

          if (_DifficultyOptions.LastOrDefault().DifficultyId > _CurrentDifficulty) {
            SetDifficulty(_CurrentDifficulty + 1);
          }
            
          _StateMachine.SetNextState(new SteerState(-50.0f, -50.0f, 1.2f, new AnimationState(AnimationName.kMajorFail, HandleRoundAnimationDone)));
        }
        else {
          _CozmoRoundsWon++;
          _StateMachine.SetNextState(new SteerState(-50.0f, -50.0f, 1.2f, new AnimationGroupState(AnimationGroupName.kWin, HandleRoundAnimationDone)));
        }

        if (Mathf.Abs(_PlayerScore - _CozmoScore) < 2) {
          _CloseRoundCount++;
        }

        int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
        int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
        int roundsLeft = _Rounds - losingScore - winningScore;
        if (winningScore > losingScore + roundsLeft) {
          if (_PlayerRoundsWon > _CozmoRoundsWon) {
            RaiseMiniGameWin("WINNER: COZMO", "Cozmo " + _CozmoRoundsWon + ", " + "Player " + _PlayerRoundsWon);
          }
          else {
            RaiseMiniGameLose("WINNER: PLAYER", "Cozmo " + _CozmoRoundsWon + ", " + "Player " + _PlayerRoundsWon);
          }
        }
        ResetScore();
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
      _StateMachine.SetNextState(new SpeedTapSelectDifficulty(cubesRequired));

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetBehaviorSystem(false);

      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(-1.0f);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.MUSIC.SILENCE);
    }

    public void SetDifficulty(int difficulty) {
      _CurrentDifficulty = difficulty;
      Rules = GetRules((SpeedTapRuleSet)difficulty);
    }

    protected override void CleanUpOnDestroy() {

      LightCube.TappedAction -= BlockTapped;
      GameAudioClient.SetMusicState(MUSIC.SILENCE);
    }

    public void InitialCubesDone() {
      CozmoBlock = GetClosestAvailableBlock();
      PlayerBlock = GetFarthestAvailableBlock();
    }

    public void UpdateUI() {
      int halfTotalRounds = (_Rounds + 1) / 2;
      CozmoScore = _CozmoScore;
      CozmoMaxRounds = halfTotalRounds;
      CozmoRoundsWon = _CozmoRoundsWon;

      PlayerScore = _PlayerScore;
      PlayerMaxRounds = halfTotalRounds;
      PlayerRoundsWon = _PlayerRoundsWon;

      // Display the current round
      InfoTitleText = string.Format(Localization.GetCultureInfo(), 
        Localization.Get(LocalizationKeys.kSpeedTapRoundsText), _CozmoRoundsWon + _PlayerRoundsWon + 1);
    }

    public void RollingBlocks() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GenericEvent.PLAY_SFX_UI_CLICK_GENERAL);
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

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = CurrentRobot.SeenObjects[i];
          }
        }
      }
      return closest as LightCube;
    }

    private LightCube GetFarthestAvailableBlock() {
      float maxDist = 0;
      ObservedObject farthest = null;

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d >= maxDist) {
            maxDist = d;
            farthest = CurrentRobot.SeenObjects[i];
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
      case SpeedTapRuleSet.LightCountAnyColor:
        return new LightCountAnyColorSpeedTapRules();
      case SpeedTapRuleSet.LightCountSameColorNoTap:
        return new LightCountSameColorNoTap();
      case SpeedTapRuleSet.LightCountSameColorNoRed:
        return new LightCountSameColorNoRed();
      default:
        return new DefaultSpeedTapRules();
      }
    }

    protected override int CalculateExcitementStatRewards() {
      return 1 + _CloseRoundCount * 2;
    }
  }
}
