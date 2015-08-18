using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HypnosisController : GameController {

  bool gameOver = false;
  bool gameReady = false;
  bool waitForLayoutTracker = true;
  int buildingCubesSpotted = 0;

  public enum HypnosisState {
    SLEEP,
    AWAKE,
    DECIDE,
    SEARCH,
    TRANCE
  }

  private HypnosisState currentState = HypnosisState.AWAKE;
  private int mostRecentTappedID = -1;
  private float lastInTranceTime = 0.0f;
  private ObservedObject tranceTarget = null;

  protected override void OnEnable() {
    base.OnEnable();
    ActiveBlock.TappedAction += BlockTapped;
    robot.VisionWhileMoving(true);
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
    robot.DriveWheels(35.0f, 0.0f);
    if (robot.activeBlocks.Count >= 2) {
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

    switch (currentState) {
    case HypnosisState.SLEEP:
      break;
    case HypnosisState.AWAKE:
      RobotAwake();
      break;
    case HypnosisState.SEARCH:
      RobotSearch();
      break;
    case HypnosisState.TRANCE:
      RobotTrance();
      break;
    }
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

    // only care about taps in awake mode.
    if (currentState == HypnosisState.AWAKE) {
      mostRecentTappedID = blockID;
      // set all to green first.
      foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
        for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
          activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
        }
      }

      for (int i = 0; i < robot.activeBlocks[blockID].lights.Length; ++i) {
        robot.activeBlocks[blockID].lights[i].onColor = CozmoPalette.ColorToUInt(Color.blue);
      }
    }
   
  }

  private void RobotAwake() {
    if (mostRecentTappedID != -1) {
      currentState = HypnosisState.SEARCH;
    }
    // turns all blocks it has seen to green.
    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
      }
    }
  }

  private void RobotSearch() {
    robot.DriveWheels(35.0f, 0.0f);
    bool tappedBlockFound = false;
    Debug.Log("robot searching");

    for (int i = 0; i < robot.observedObjects.Count; ++i) {
      if (robot.observedObjects[i].ID == mostRecentTappedID) {
        tappedBlockFound = true;
        tranceTarget = robot.observedObjects[i];
        for (int j = 0; j < robot.activeBlocks[mostRecentTappedID].lights.Length; ++j) {
          robot.activeBlocks[mostRecentTappedID].lights[j].onColor = CozmoPalette.ColorToUInt(Color.red);
        }
        break;
      }
    }

    if (tappedBlockFound) {
      lastInTranceTime = Time.time;
      currentState = HypnosisState.TRANCE;
      robot.GotoObject(tranceTarget, 80.0f);
    }
  }

  private void RobotTrance() {
    if (Time.time - lastInTranceTime > 10.0f) {
      Debug.Log("Canceling Drive to Object action");
      robot.CancelAction(Anki.Cozmo.RobotActionType.DRIVE_TO_OBJECT);
      currentState = HypnosisState.AWAKE;
      mostRecentTappedID = -1;
      tranceTarget = null;
    }
  }
}
