using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class ShakeShakeController : GameController {

  private bool gameOver = false;
  private bool gameReady = false;
  private int currentScore = 0;

  private class BlockData {
    public float lastColorChangeTime;
    public Color currentColor;

    public void SetNewColor(Color color) {
      currentColor = color;
      lastColorChangeTime = Time.time;
    }
  }

  private Dictionary<int, BlockData> blockData = new Dictionary<int, BlockData>();

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    ActiveBlock.TappedAction += BlockTapped;
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.TappedAction -= BlockTapped;
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
    // spawn UI
    // create block game states
    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      blockData.Add(activeBlock.Key, new BlockData());
    }
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();
    // update UI

    // update block game states
    int greenBlockCount = 0;
    int blueBlockCount = 0;

    // update block colors
    foreach (KeyValuePair<int, BlockData> bData in blockData) {
      for (int j = 0; j < robot.activeBlocks[bData.Key].lights.Length; ++j) {
        robot.activeBlocks[bData.Key].lights[j].onColor = CozmoPalette.ColorToUInt(bData.Value.currentColor);
      }
    }
    DAS.Debug("ShakeShakeController", "current score: " + currentScore);
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
    BlockData bData = null;
    if (blockData.TryGetValue(blockID, out bData)) {
      if (bData.currentColor == Color.green) {
        currentScore++;
      }
      else if (bData.currentColor == Color.red) {
        currentScore = currentScore - 10;
        if (currentScore < 0) {
          currentScore = 0;
        }
      }
    }
  }

}
