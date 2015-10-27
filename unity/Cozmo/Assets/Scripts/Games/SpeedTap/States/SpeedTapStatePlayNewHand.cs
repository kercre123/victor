using System;
using UnityEngine;

public class SpeedTapStatePlayNewHand : State {

  SpeedTapController speedTapController = null;
  float startTimeMillis = 0;
  float onDelayTimeMillis = 2000.0f;
  float offDelayTimeMillis = 2000.0f;
  float cozmoTapDelayTimeMillis = 100.0f;
  float matchProbability = 0.35f;


  bool lightsOn = false;
  bool gotMatch = false;
  bool cozmoTapping = false;

  enum WinState {
    Neutral,
    CozmoWins,
    PlayerWins
  };

  WinState curWinState = WinState.Neutral;

  Color[] colors = { Color.white, Color.red, Color.green, Color.blue, Color.yellow, Color.magenta };

  public override void Enter() {
    base.Enter();
    speedTapController = stateMachine.GetGameController() as SpeedTapController;
    startTimeMillis = Time.time * 1000.0f;
    speedTapController.cozmoBlock.SetLEDs(0, 0, 0xFF);
    speedTapController.playerBlock.SetLEDs(0, 0, 0xFF);
    lightsOn = false;

    //ActiveBlock.TappedAction += BlockTapped;
    speedTapController.PlayerTappedBlockEvent += PlayerDidTap;
  }

  public override void Update() {
    base.Update();

    float curTimeMillis = Time.time * 1000.0f;

    if (lightsOn) {
      if (gotMatch) {
        if(! cozmoTapping) {
          if ((curTimeMillis - startTimeMillis) >= cozmoTapDelayTimeMillis) {
            DAS.Info("SpeedTap.CozmoTapping", "" + speedTapController.cozmoScore);
            RobotEngineManager.instance.RobotCompletedAnimation += RobotCompletedTapAnimation;
            CozmoEmotionManager.SetEmotion("TAP_ONE", true);
            cozmoTapping = true;
          }
        }
      }
      if ((curTimeMillis - startTimeMillis) >= onDelayTimeMillis) {
        speedTapController.cozmoBlock.SetLEDs(0, 0, 0xFF);
        speedTapController.playerBlock.SetLEDs(0, 0, 0xFF);
        lightsOn = false;
        startTimeMillis = curTimeMillis;
      }
    }
    else {
      if ((curTimeMillis - startTimeMillis) >= offDelayTimeMillis) {
        curWinState = WinState.Neutral;
        RollForLights();
        lightsOn = true;
        startTimeMillis = curTimeMillis;
      }
    }
  }

  public override void Exit() {
    base.Exit();
    //ActiveBlock.TappedAction -= BlockTapped;
    RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
    speedTapController.PlayerTappedBlockEvent -= PlayerDidTap;
  }

  void RobotCompletedTapAnimation(bool success, string animName) {
    DAS.Info("SpeedTapStatePlayNewHand.RobotCompletedTapAnimation", animName + " success = " + success);
    switch (animName) {
    case "tapCube":
      DAS.Info("SpeedTapStatePlayNewHand.tap_complete", "");
      // check for player tapped first here.
      CozmoDidTap();
      break;
    case "finishTapCubeLose":
      DAS.Info("SpeedTapStatePlayNewHand.tap_win", "");
      // check for player tapped first here.
      gotMatch = false;
      cozmoTapping = false;
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
      break;
    case "finishTapCubeWin":
      DAS.Info("SpeedTapStatePlayNewHand.tap_win", "");
      // check for player tapped first here.
      gotMatch = false;
      cozmoTapping = false;
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
      break;
    }

  }

  void CozmoDidTap() {
    DAS.Info("SpeedTapStatePlayNewHand.cozmo_tap", "");
    if (curWinState == WinState.Neutral) {
      curWinState = WinState.CozmoWins;
      speedTapController.cozmoScore++;
      speedTapController.UpdateUI();
      // play sound, do dance
      CozmoEmotionManager.SetEmotion("TAP_TWO", true);
    }
    // otherwise cozmo is too late!
    else {
      CozmoEmotionManager.SetEmotion("TAP_THREE", true);
    }
  }

  void BlockTapped(int blockID, int numTaps) {
    if (blockID == speedTapController.playerBlock.ID) {
      PlayerDidTap();
    }   
  }

  void PlayerDidTap() {
    DAS.Info("SpeedTapStatePlayNewHand.player_tap", "");
    if (gotMatch) {
      if (curWinState == WinState.Neutral) {
        curWinState = WinState.PlayerWins;
        speedTapController.playerScore++;
        speedTapController.UpdateUI();
      }
    }
    else if (curWinState == WinState.Neutral) {
      curWinState = WinState.CozmoWins;
      speedTapController.playerScore--;
      speedTapController.UpdateUI();
    }
  }

  private void RollForLights() {
    float matchExperiment = UnityEngine.Random.value;
    if (matchExperiment <= matchProbability) {
      // Do match
      gotMatch = true;
      int randColor = ((int)(matchExperiment * 1000f)) % colors.Length;
      Color matchColor = colors[randColor];
      speedTapController.cozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(matchColor), 0, 0xFF);
      speedTapController.playerBlock.SetLEDs(CozmoPalette.ColorToUInt(matchColor), 0, 0xFF);
    }
    else {
      // Do non-match
      gotMatch = false;
      int playerColorIdx = ((int)(matchExperiment * 1000f)) % colors.Length;
      int cozmoColorIdx = ((int)(matchExperiment * 3333f)) % colors.Length;
      if (cozmoColorIdx == playerColorIdx) {
        cozmoColorIdx = (cozmoColorIdx + 1) % colors.Length;
      }
      Color playerColor = colors[playerColorIdx];
      Color cozmoColor = colors[cozmoColorIdx];
      speedTapController.playerBlock.SetLEDs(CozmoPalette.ColorToUInt(playerColor), 0, 0xFF);
      speedTapController.cozmoBlock.SetLEDs(CozmoPalette.ColorToUInt(cozmoColor), 0, 0xFF);
    }
  }

}


