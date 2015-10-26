using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class SpeedTapController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
  }

  private void BlockTapped(int blockID, int tappedTimes) {
    Debug.Log("Ima tapped:" + blockID);
  }

  protected override void OnDisable() {
    base.OnDisable();
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }

    if (robot.activeBlocks.Count >= 2) {
      gameReady = true;
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

}
