using System;
using UnityEngine;

public class SpeedTapStatePlayNewHand : State {

  private SpeedTapGame speedTapGame_ = null;
  private float startTimeMs_ = 0;
  private float onDelayTimeMs_ = 2000.0f;
  private float offDelayTimeMs_ = 2000.0f;
  private float cozmoTapDelayTimeMs_ = 100.0f;
  private float matchProbability_ = 0.35f;

  private bool lightsOn_ = false;
  private bool gotMatch_ = false;
  private bool cozmoTapping_ = false;

  enum WinState {
    Neutral,
    CozmoWins,
    PlayerWins
  }

  WinState curWinState_ = WinState.Neutral;

  Color[] colors = { Color.white, Color.red, Color.green, Color.blue, Color.yellow, Color.magenta };

  public override void Enter() {
    base.Enter();
    speedTapGame_ = stateMachine_.GetGame() as SpeedTapGame;
    startTimeMs_ = Time.time * 1000.0f;
    speedTapGame_.cozmoBlock_.SetLEDs(0, 0, 0xFF);
    speedTapGame_.playerBlock_.SetLEDs(0, 0, 0xFF);
    lightsOn_ = false;

    speedTapGame_.PlayerTappedBlockEvent += PlayerDidTap;
  }

  public override void Update() {
    base.Update();

    float curTimeMillis = Time.time * 1000.0f;

    if (lightsOn_) {
      if (gotMatch_) {
        if (!cozmoTapping_) {
          if ((curTimeMillis - startTimeMs_) >= cozmoTapDelayTimeMs_) {
            DAS.Info("SpeedTap.CozmoTapping", "" + speedTapGame_.cozmoScore_);
            RobotEngineManager.instance.RobotCompletedAnimation += RobotCompletedTapAnimation;
            robot.SendAnimation("TAP_ONE");
            cozmoTapping_ = true;
          }
        }
      }
      if ((curTimeMillis - startTimeMs_) >= onDelayTimeMs_) {
        speedTapGame_.cozmoBlock_.SetLEDs(0, 0, 0xFF);
        speedTapGame_.playerBlock_.SetLEDs(0, 0, 0xFF);
        lightsOn_ = false;
        startTimeMs_ = curTimeMillis;
      }
    }
    else {
      if ((curTimeMillis - startTimeMs_) >= offDelayTimeMs_) {
        curWinState_ = WinState.Neutral;
        RollForLights();
        lightsOn_ = true;
        startTimeMs_ = curTimeMillis;
      }
    }
  }

  public override void Exit() {
    base.Exit();

    RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
    speedTapGame_.PlayerTappedBlockEvent -= PlayerDidTap;
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
      gotMatch_ = false;
      cozmoTapping_ = false;
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
      break;
    case "finishTapCubeWin":
      DAS.Info("SpeedTapStatePlayNewHand.tap_win", "");
      // check for player tapped first here.
      gotMatch_ = false;
      cozmoTapping_ = false;
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedTapAnimation;
      break;
    }

  }

  void CozmoDidTap() {
    DAS.Info("SpeedTapStatePlayNewHand.cozmo_tap", "");
    if (curWinState_ == WinState.Neutral) {
      curWinState_ = WinState.CozmoWins;
      speedTapGame_.cozmoScore_++;
      speedTapGame_.UpdateUI();
      // play sound, do dance
      robot.SendAnimation("TAP_TWO");
    }
    // otherwise cozmo is too late!
    else {
      robot.SendAnimation("TAP_THREE");
    }
  }

  void BlockTapped(int blockID, int numTaps) {
    if (blockID == speedTapGame_.playerBlock_.ID) {
      PlayerDidTap();
    }   
  }

  void PlayerDidTap() {
    DAS.Info("SpeedTapStatePlayNewHand.player_tap", "");
    if (gotMatch_) {
      if (curWinState_ == WinState.Neutral) {
        curWinState_ = WinState.PlayerWins;
        speedTapGame_.playerScore_++;
        speedTapGame_.UpdateUI();
      }
    }
    else if (curWinState_ == WinState.Neutral) {
      curWinState_ = WinState.CozmoWins;
      speedTapGame_.playerScore_--;
      speedTapGame_.UpdateUI();
    }
  }

  private void RollForLights() {
    speedTapGame_.RollingBlocks();

    float matchExperiment = UnityEngine.Random.value;
    if (matchExperiment <= matchProbability_) {
      // Do match
      gotMatch_ = true;
      int randColor = ((int)(matchExperiment * 1000f)) % colors.Length;
      Color matchColor = colors[randColor];
      speedTapGame_.cozmoBlock_.SetLEDs(CozmoPalette.ColorToUInt(matchColor), 0, 0xFF);
      speedTapGame_.playerBlock_.SetLEDs(CozmoPalette.ColorToUInt(matchColor), 0, 0xFF);
    }
    else {
      // Do non-match
      gotMatch_ = false;
      int playerColorIdx = ((int)(matchExperiment * 1000f)) % colors.Length;
      int cozmoColorIdx = ((int)(matchExperiment * 3333f)) % colors.Length;
      if (cozmoColorIdx == playerColorIdx) {
        cozmoColorIdx = (cozmoColorIdx + 1) % colors.Length;
      }
      Color playerColor = colors[playerColorIdx];
      Color cozmoColor = colors[cozmoColorIdx];
      speedTapGame_.playerBlock_.SetLEDs(CozmoPalette.ColorToUInt(playerColor), 0, 0xFF);
      speedTapGame_.cozmoBlock_.SetLEDs(CozmoPalette.ColorToUInt(cozmoColor), 0, 0xFF);
    }
  }

}


