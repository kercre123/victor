using UnityEngine;
using System.Collections;
using System;
using Anki.Cozmo.Audio;

namespace SpeedTap {

  public class SpeedTapGame : GameBase {

    public LightCube CozmoBlock;
    public LightCube PlayerBlock;
    public Color MatchColor;

    public ISpeedTapRules Rules;

    private int _CozmoScore;
    private int _PlayerScore;
    private int _PlayerRoundsWon;
    private int _CozmoRoundsWon;
    private int _Rounds;
    private int _MaxScorePerRound;

    public event Action PlayerTappedBlockEvent;

    [SerializeField]
    private SpeedTapPanel _GamePanelPrefab;
    private SpeedTapPanel _GamePanel;

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
      _PlayerScore = Mathf.Max(0, _PlayerScore - 1);
      UpdateUI();
    }

    private void CheckRounds() {
      if (_CozmoScore >= _MaxScorePerRound || _PlayerScore >= _MaxScorePerRound) {

        if (_PlayerScore > _CozmoScore) {
          _PlayerRoundsWon++;
        }
        else {
          _CozmoRoundsWon++;
        }

        int losingScore = Mathf.Min(_PlayerRoundsWon, _CozmoRoundsWon);
        int winningScore = Mathf.Max(_PlayerRoundsWon, _CozmoRoundsWon);
        int roundsLeft = _Rounds - losingScore - winningScore;
        if (winningScore > losingScore + roundsLeft) {
          if (_PlayerRoundsWon > _CozmoRoundsWon) {
            RaiseMiniGameWin();
          }
          else {
            RaiseMiniGameLose();
          }
        }
        ResetScore();
      }
    }

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      InitializeMinigameObjects();
      SpeedTapGameConfig speedTapConfig = minigameConfig as SpeedTapGameConfig;
      _Rounds = speedTapConfig.Rounds;
      _MaxScorePerRound = speedTapConfig.MaxScorePerRound;
      Rules = GetRules(speedTapConfig.RuleSet);
    }

    // Use this for initialization
    protected void InitializeMinigameObjects() { 
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new SpeedTapWaitForCubePlace(), 2, false, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);

      CurrentRobot.VisionWhileMoving(true);
      LightCube.TappedAction += BlockTapped;
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetBehaviorSystem(false);
      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<SpeedTapPanel>();
      _GamePanel.TapButtonPressed += UIButtonTapped;
      UpdateUI();

      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(-1.0f);
    }

    protected override void CleanUpOnDestroy() {
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }

      LightCube.TappedAction -= BlockTapped;
      GameAudioClient.SetMusicState(MusicGroupStates.SILENCE);
    }

    void InitialCubesDone() {
      CozmoBlock = GetClosestAvailableBlock();
      PlayerBlock = GetFarthestAvailableBlock();
    }

    public void UpdateUI() {
      _GamePanel.SetScoreText(_CozmoScore, _PlayerScore, _CozmoRoundsWon, _PlayerRoundsWon, _Rounds);
    }

    public void RollingBlocks() {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.EventType.PLAY_SFX_UI_CLICK_GENERAL);
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

    private static ISpeedTapRules GetRules(SpeedTap.SpeedTapRuleSet ruleSet) {
      switch (ruleSet) {
      default:
        return new DefaultSpeedTapRules();
      }
    }
  }
}
