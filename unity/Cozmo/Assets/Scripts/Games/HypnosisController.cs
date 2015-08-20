using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

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
  private HypnosisState playState = HypnosisState.AWAKE;
  private HypnosisState nextState = HypnosisState.AWAKE;
  private float playStateTimer = 0.0f;
  private int mostRecentTappedID = -1;
  private ObservedObject tranceTarget = null;
  private bool searchTurnRight = false;

  protected override void OnEnable() {
    base.OnEnable();
    ActiveBlock.TappedAction += BlockTapped;
    robot.VisionWhileMoving(true);
    RobotEngineManager.instance.SuccessOrFailure += RobotEngineMessages;
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
    robot.DriveWheels(15.0f, -10.0f);

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
    if (playState != nextState) {
      ExitPlayState();
      playState = nextState;
      EnterPlayState();
    }
    else {
      UpdatePlayState();
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

  private void EnterPlayState() {
    playStateTimer = 0.0f;
    switch (playState) {
    case HypnosisState.SLEEP:
      Enter_ROBOT_SLEEP();
      break;
    case HypnosisState.AWAKE:
      Enter_ROBOT_AWAKE();
      break;
    case HypnosisState.SEARCH:
      Enter_ROBOT_SEARCH();
      break;
    case HypnosisState.TRANCE:
      Enter_ROBOT_TRANCE();
      break;
    }
  }

  private void UpdatePlayState() {
    playStateTimer += Time.deltaTime;
    switch (playState) {
    case HypnosisState.SLEEP:
      Update_ROBOT_SLEEP();
      break;
    case HypnosisState.AWAKE:
      Update_ROBOT_AWAKE();
      break;
    case HypnosisState.SEARCH:
      Update_ROBOT_SEARCH();
      break;
    case HypnosisState.TRANCE:
      Update_ROBOT_TRANCE();
      break;
    }
  }

  private void ExitPlayState() {
    switch (playState) {
    case HypnosisState.SLEEP:
      Exit_ROBOT_SLEEP();
      break;
    case HypnosisState.AWAKE:
      Exit_ROBOT_AWAKE();
      break;
    case HypnosisState.SEARCH:
      Exit_ROBOT_SEARCH();
      break;
    case HypnosisState.TRANCE:
      Exit_ROBOT_TRANCE();
      break;
    }
  }

  private void RobotEngineMessages(bool success, RobotActionType action_type) {
    if (success && action_type == RobotActionType.PLAY_ANIMATION) {
      robot.SetHeadAngle(-0.5f);
    }
  }

  private void SendAnimation(string animName) {
    CozmoAnimation newAnimation = new CozmoAnimation();
    newAnimation.animName = animName;
    newAnimation.numLoops = 1;
    CozmoEmotionManager.instance.SendAnimation(newAnimation);
  }

  private void BlockTapped(int blockID, int numTapped) {
    // only care about taps in awake mode.
    if (playState == HypnosisState.AWAKE) {
      mostRecentTappedID = blockID;
    }
  }

  private void Enter_ROBOT_SLEEP() {
    robot.DriveWheels(0.0f, 0.0f);
    mostRecentTappedID = -1;
    tranceTarget = null;
  }

  private void Update_ROBOT_SLEEP() {
    if (playStateTimer > 1.0f) {
      nextState = HypnosisState.AWAKE;
    }

    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.white);
      }
    }
  }

  private void Exit_ROBOT_SLEEP() {
    
  }

  private void Enter_ROBOT_AWAKE() {
    
  }

  private void Update_ROBOT_AWAKE() {
    if (mostRecentTappedID != -1) {
      nextState = HypnosisState.SEARCH;
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

  private void Exit_ROBOT_AWAKE() {
    
  }

  private void Enter_ROBOT_SEARCH() {
    foreach (KeyValuePair<int, ActiveBlock> activeBlock in robot.activeBlocks) {
      for (int j = 0; j < activeBlock.Value.lights.Length; ++j) {
        if (activeBlock.Value.ID == mostRecentTappedID) {
          activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(new Color(1.0f, 0.0f, 1.0f, 1.0f));
        }
        else {
          activeBlock.Value.lights[j].onColor = CozmoPalette.ColorToUInt(Color.green);
        }
      }
    }
  }

  private void Update_ROBOT_SEARCH() {
    if (searchTurnRight) {
      robot.DriveWheels(15.0f, -10.0f);
    }
    else {
      robot.DriveWheels(-10.0f, 15.0f);
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
      nextState = HypnosisState.TRANCE;
    }

    if (playStateTimer > 10.0f) {
      // searching for too long. give up.
      nextState = HypnosisState.SLEEP;
    }
  }

  private void Exit_ROBOT_SEARCH() {
    
  }

  private void Enter_ROBOT_TRANCE() {

  }

  private void Update_ROBOT_TRANCE() {

    if (Time.time - robot.activeBlocks[mostRecentTappedID].TimeLastSeen > 2.0f) {
      // we haven't seen the block we're looking for in a while... let's look for it.
      nextState = HypnosisState.SEARCH;
    }

    float angle = Vector2.Angle(robot.Forward, robot.activeBlocks[mostRecentTappedID].WorldPosition - robot.WorldPosition);
    if (robot.activeBlocks[mostRecentTappedID].ID == mostRecentTappedID && angle < 10.0f) {
      // we are reasonably aligned to it so move forward
      float distance = Vector3.Distance(robot.WorldPosition, robot.activeBlocks[mostRecentTappedID].WorldPosition);
      if (distance > 100.0f) {
        robot.DriveWheels(15.0f, 15.0f);
      }
      else {
        robot.DriveWheels(0.0f, 0.0f);
      }
    }
    else {
      // we need to turn to face it
      ComputeTurnDirection();
      if (searchTurnRight) {
        robot.DriveWheels(15.0f, -10.0f);
      }
      else {
        robot.DriveWheels(-10.0f, 15.0f);
      }
    }

    if (playStateTimer > 15.0f) {
      nextState = HypnosisState.SLEEP;
    }
  }

  private void Exit_ROBOT_TRANCE() {

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
