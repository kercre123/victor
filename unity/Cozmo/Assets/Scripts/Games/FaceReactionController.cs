using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class FaceReactionController : GameController {

  private bool gameOver = false;
  private bool gameReady = true;

  private FaceReactionState playState = FaceReactionState.NONE;
  private FaceReactionState nextState = FaceReactionState.SPIN_RANDOM;
  private float playStateTimer = 0.0f;
  private float randomSpinDuration = 0.0f;
  private bool alignToFaceWaiting = true;
  private bool animationPlaying = false;

  public enum FaceReactionState {
    NONE,
    SPIN_RANDOM,
    SEARCH_FOR_FACE,
    ALIGN_TO_FACE,
    MOVE_TOWARD_FACE,
    REACT_TO_FACE
  }

  protected override void OnEnable() {
    base.OnEnable();
    robot.VisionWhileMoving(true);
    robot.SetHeadAngle(0.5f);
    robot.SetLiftHeight(0.0f);
    robot.StartFaceAwareness();
    RobotEngineManager.instance.SuccessOrFailure += RobotEngineMessages;
  }

  protected override void OnDisable() {
    base.OnDisable();
    RobotEngineManager.instance.SuccessOrFailure -= RobotEngineMessages;
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
  }

  protected override void Exit_BUILDING() {
    // disables add/remove
    //base.Exit_BUILDING();

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
    robot.SetObjectAdditionAndDeletion(true, true);
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
    case FaceReactionState.SPIN_RANDOM:
      Enter_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Enter_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Enter_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Enter_REACT_TO_FACE();
      break;
    case FaceReactionState.ALIGN_TO_FACE:
      Enter_ALIGN_TO_FACE();
      break;
    }
  }

  private void UpdatePlayState() {
    playStateTimer += Time.deltaTime;
    switch (playState) {
    case FaceReactionState.SPIN_RANDOM:
      Update_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Update_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Update_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Update_REACT_TO_FACE();
      break;
    case FaceReactionState.ALIGN_TO_FACE:
      Update_ALIGN_TO_FACE();
      break;
    }
  }

  private void ExitPlayState() {
    switch (playState) {
    case FaceReactionState.SPIN_RANDOM:
      Exit_SPIN_RANDOM();
      break;
    case FaceReactionState.SEARCH_FOR_FACE:
      Exit_SEARCH_FOR_FACE();
      break;
    case FaceReactionState.MOVE_TOWARD_FACE:
      Exit_MOVE_TOWARD_FACE();
      break;
    case FaceReactionState.REACT_TO_FACE:
      Exit_REACT_TO_FACE();
      break;
    case FaceReactionState.ALIGN_TO_FACE:
      Exit_ALIGN_TO_FACE();
      break;
    }
  }

  private void Enter_SPIN_RANDOM() {
    robot.SetHeadAngle(-0.8f);
    randomSpinDuration = Random.Range(3.0f, 5.5f);
    DriveLeftOrRight(30.0f);
    Debug.Log("setting wheels spin");
  }

  private void Update_SPIN_RANDOM() {
    Debug.Log("spinning..." + randomSpinDuration);
    Debug.Log(playStateTimer);
    if (playStateTimer > randomSpinDuration) {
      nextState = FaceReactionState.SEARCH_FOR_FACE;
      Debug.Log("setting search for face stage..");
    }
  }

  private void Exit_SPIN_RANDOM() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_SEARCH_FOR_FACE() {
    robot.SetLiftHeight(0.0f);
    robot.SetHeadAngle(0.0f);
    DriveLeftOrRight(10.0f);
  }

  private void Update_SEARCH_FOR_FACE() {
    if (robot.markersVisibleObjects.Count > 0) {
      nextState = FaceReactionState.ALIGN_TO_FACE;
      robot.FaceObject(robot.markersVisibleObjects[0]);
    }
  }

  private void Exit_SEARCH_FOR_FACE() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_ALIGN_TO_FACE() {
    alignToFaceWaiting = true;
  }

  private void Update_ALIGN_TO_FACE() {
    if (playStateTimer > 3.0f) {
      nextState = FaceReactionState.SEARCH_FOR_FACE;
    }
    if (!alignToFaceWaiting) {
      nextState = FaceReactionState.MOVE_TOWARD_FACE;
    }
  }

  private void Exit_ALIGN_TO_FACE() {
    
  }

  private void Enter_MOVE_TOWARD_FACE() {
    robot.DriveWheels(15.0f, 15.0f);
  }

  private void Update_MOVE_TOWARD_FACE() {
    if (playStateTimer > 1.5f) {
      nextState = FaceReactionState.REACT_TO_FACE;
    }
  }

  private void Exit_MOVE_TOWARD_FACE() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_REACT_TO_FACE() {
    int index = Random.Range(0, RobotEngineManager.instance.robotAnimationNames.Count);
    SendAnimation(RobotEngineManager.instance.robotAnimationNames[index]);
    animationPlaying = true;
  }

  private void Update_REACT_TO_FACE() {
    if (!animationPlaying) {
      nextState = FaceReactionState.SPIN_RANDOM;
    }
  }

  private void Exit_REACT_TO_FACE() {
    
  }

  private void DriveLeftOrRight(float speed) {
    float leftRightDecision = Random.Range(0.0f, 1.0f);
    if (leftRightDecision < 0.5f) {
      robot.DriveWheels(-speed, speed);
    }
    else {
      robot.DriveWheels(speed, -speed);
    }
  }

  private void RobotEngineMessages(bool success, RobotActionType action_type) {
    if (success && action_type == RobotActionType.FACE_OBJECT) {
      alignToFaceWaiting = false;
    }
    if (success && action_type == RobotActionType.PLAY_ANIMATION) {
      animationPlaying = false;
    }
  }

  private void SendAnimation(string animName) {
    CozmoAnimation newAnimation = new CozmoAnimation();
    newAnimation.animName = animName;
    newAnimation.numLoops = 1;
    CozmoEmotionManager.instance.SendAnimation(newAnimation);
  }

}
