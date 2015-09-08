using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class PatternPlayController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
    RobotEngineManager.instance.SuccessOrFailure += RobotEngineMessages;
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.TappedAction -= BlockTapped;
    RobotEngineManager.instance.SuccessOrFailure -= RobotEngineMessages;
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
    robot.DriveWheels(15.0f, -15.0f);

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(new Color(1.0f, 1.0f, 1.0f, 1.0f));
      }
    }

    // makes sure cozmo sees all 4 blocks first.
    if (robot.activeBlocks.Count >= 4) {
      gameReady = true;
      robot.DriveWheels(0.0f, 0.0f);
    }
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Enter_PRE_GAME() {
    base.Enter_PRE_GAME();
  }

  protected override void Update_PRE_GAME() {
    base.Update_PRE_GAME();
  }

  protected override void Exit_PRE_GAME() {
    base.Exit_PRE_GAME();
  }

  protected override void Enter_PLAYING() {
    base.Enter_PLAYING();

  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();
  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();
  }

  protected override void Enter_RESULTS() {
    base.Enter_RESULTS();
  }

  protected override void Update_RESULTS() {
    base.Update_RESULTS();
  }

  protected override void Exit_RESULTS() {
    base.Exit_RESULTS();
  }

  protected override bool IsPreGameCompleted() {
    return true;
  }

  protected override bool IsGameReady() {
    return gameReady;
  }

  protected override bool IsGameOver() {
    return gameOver;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();
  }

  private void BlockTapped(int blockID, int numTapped) {

  }

  private void RobotEngineMessages(bool success, RobotActionType action_type) {

  }

}
