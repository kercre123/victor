using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HypnosisController : GameController {

  bool gameOver = false;

  public enum HypnosisState {
    WANDER,
    SEARCH,
    TRANCE
  }

  private HypnosisState currentState = HypnosisState.WANDER;
  private int mostRecentTappedID = -1;
  private float lastInTranceTime = 0.0f;
  private ObservedObject tranceTarget = null;

  protected override void OnEnable() {
    base.OnEnable();
    ActiveBlock.TappedAction += BlockTapped;
  }

  protected override void OnDisable() {
    base.OnDisable();
    ActiveBlock.TappedAction -= BlockTapped;
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
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
    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
      }
    }
    switch (currentState) {
    case HypnosisState.WANDER:
      RobotWander();
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
    return true;
  }

  protected override bool IsGameOver() {
    return gameOver;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();
  }

  private void BlockTapped(int blockID, int numTapped) {
    mostRecentTappedID = blockID;
  }

  private void RobotWander() {
    if (mostRecentTappedID != -1) {
      currentState = HypnosisState.SEARCH;
    }
  }

  private void RobotSearch() {
    robot.DriveWheels(15.0f, 0.0f);
    bool tappedBlockFound = false;

    for (int i = 0; i < robot.observedObjects.Count; ++i) {
      if (robot.observedObjects[i].ID == mostRecentTappedID) {
        tappedBlockFound = true;
        tranceTarget = robot.observedObjects[i];
      }
    }

    if (tappedBlockFound) {
      lastInTranceTime = Time.time;
      currentState = HypnosisState.TRANCE;
    }
  }

  private void RobotTrance() {

    // follow observed object
    if (tranceTarget != null) {
      robot.GotoObject(tranceTarget, 80.0f);
    }

    if (Time.time - lastInTranceTime > 10.0f) {
      currentState = HypnosisState.WANDER;
      mostRecentTappedID = -1;
      tranceTarget = null;
    }
  }

}
