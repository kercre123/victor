using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class HypnosisController : GameController {

  public enum HypnosisState {
    SLEEP,
    AWAKE,
    DECIDE,
    SEARCH,
    TRANCE
  }

  private bool gameOver = false;
  private bool gameReady = false;
  private bool waitForLayoutTracker = true;
  private int buildingCubesSpotted = 0;
  private HypnosisState currentState = HypnosisState.AWAKE;
  private int mostRecentTappedID = -1;
  private float lastInTranceTime = 0.0f;
  private float startSleepTime = 0.0f;
  private ObservedObject tranceTarget = null;
  private bool searchTurnRight = false;

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
    robot.DriveWheels(35.0f, -20.0f);

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(new Color(1.0f, 1.0f, 1.0f, 1.0f));
      }
    }

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
      RobotSleep();
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

      foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
        for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
          if (activeBlock.Value.ID == blockID) {
            activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(new Color(1.0f, 1.0f, 0.0f, 1.0f));
          }
          else {
            activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
          }
        }
      }
    }
   
  }

  private void RobotSleep() {
    if (Time.time - startSleepTime > 3.0f) {
      currentState = HypnosisState.AWAKE;
    }

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.white);
      }
    }
  }

  private void RobotAwake() {
    if (mostRecentTappedID != -1) {
      currentState = HypnosisState.SEARCH;
      ComputeTurnDirection();
    }
    else {
      // turns all blocks it has seen to green.
      foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
        for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
          activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
        }
      }
    }
  }

  private void RobotSearch() {
    if (searchTurnRight) {
      robot.DriveWheels(35.0f, -20.0f);
    }
    else {
      robot.DriveWheels(-20.0f, 35.0f);
    }

    bool tappedBlockFound = false;
 
    for (int i = 0; i < robot.observedObjects.Count; ++i) {
      float angle = Vector2.Angle(robot.Forward, robot.activeBlocks[mostRecentTappedID].WorldPosition - robot.WorldPosition);
      if (robot.observedObjects[i].ID == mostRecentTappedID && angle < 10.0f) {
        tappedBlockFound = true;
        tranceTarget = robot.observedObjects[i];
        for (int j = 0; j < robot.activeBlocks[mostRecentTappedID].lights.Length; ++j) {
          robot.activeBlocks[mostRecentTappedID].lights[j].onColor = CozmoPalette.ColorToUInt(Color.red);
        }
        break;
      }
    }

    if (tappedBlockFound) {
      robot.DriveWheels(0.0f, 0.0f);
      lastInTranceTime = Time.time;
      currentState = HypnosisState.TRANCE;
    }
  }

  private void RobotTrance() {

    bool foundTranceTarget = false;

    for (int i = 0; i < robot.observedObjects.Count; ++i) {
      if (robot.observedObjects[i].ID == mostRecentTappedID) {
        foundTranceTarget = true;
        float angle = Vector2.Angle(robot.Forward, robot.observedObjects[i].WorldPosition - robot.WorldPosition);
        if (robot.observedObjects[i].ID == mostRecentTappedID && angle < 10.0f) {
          // we are reasonably aligned to it
          float distance = Vector3.Distance(robot.WorldPosition, robot.observedObjects[i].WorldPosition);
          if (distance > 100.0f) {
            robot.DriveWheels(20.0f, 20.0f);
          }
          else {
            robot.DriveWheels(0.0f, 0.0f);
          }
        }
        else {
          // we need to turn to face it
          ComputeTurnDirection();
          if (searchTurnRight) {
            robot.DriveWheels(35.0f, -20.0f);
          }
          else {
            robot.DriveWheels(-20.0f, 35.0f);
          }
        }
      }
    }

    if (foundTranceTarget == false) {
      currentState = HypnosisState.SEARCH;
      ComputeTurnDirection();
    }

    if (Time.time - lastInTranceTime > 8.0f) {
      robot.DriveWheels(0.0f, 0.0f);
      currentState = HypnosisState.SLEEP;
      mostRecentTappedID = -1;
      tranceTarget = null;
      startSleepTime = Time.time;
    }
  }

  private void ComputeTurnDirection() {
    if (mostRecentTappedID == -1)
      return;

    float turnAngle = Vector3.Cross(robot.Forward, robot.activeBlocks[mostRecentTappedID].WorldPosition - robot.WorldPosition).z;
    if (turnAngle < 0.0f)
      searchTurnRight = true;
    else
      searchTurnRight = false;
  }
}
