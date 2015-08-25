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
  private float lastTurnSearchAngle = 0.0f;
  private bool turningLeft = false;
  private bool lookingUpToSearch = false;
  private float faceSeenTime = 0.0f;

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
    SetBackpackColors(Color.white);
    robot.SetHeadAngle(1f);
    randomSpinDuration = Random.Range(3.0f, 5.5f);
    DecideLeftOrRight();
    RotateCozmo(30.0f);
  }

  private void Update_SPIN_RANDOM() {
    if (playStateTimer > randomSpinDuration) {
      nextState = FaceReactionState.SEARCH_FOR_FACE;
    }
  }

  private void Exit_SPIN_RANDOM() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_SEARCH_FOR_FACE() {
    SetBackpackColors(Color.green);
    robot.SetLiftHeight(0.0f);
    robot.SetHeadAngle(-1f);
    DecideLeftOrRight();
    RotateCozmo(10.0f);
    lastTurnSearchAngle = robot.Rotation.eulerAngles.y;
    faceSeenTime = 0.0f;
  }

  private void Update_SEARCH_FOR_FACE() {
    Debug.Log(faceSeenTime);
    // HACK: shouldn't have to use accumulators when the face detection
    // is more reliable (not as many false positives)
    if (robot.markersVisibleObjects.Count > 0 && robot.markersVisibleObjects[0].isFace) {
      if (faceSeenTime > 1.5f) {
        Debug.Log("we found a face!");
        nextState = FaceReactionState.ALIGN_TO_FACE;
      }
      faceSeenTime += Time.deltaTime;
    }
    else {
      faceSeenTime = Mathf.Max(0.0f, faceSeenTime - Time.deltaTime * 1.5f);
    }

    if (faceSeenTime > 0.0f)
      return;

    if (robot.IsHeadAngleRequestUnderway())
      return;

    // turn, look up and down.
    // when done looking up and down, continue turning.

    if (Mathf.Abs(robot.Rotation.eulerAngles.z - lastTurnSearchAngle) < 40.0f) {
      RotateCozmo(10.0f);
    }
    else if (lookingUpToSearch == false) {
      RotateCozmo(0.0f);
      robot.SetHeadAngle(0.5f);
      lookingUpToSearch = true;
    }
    else {
      lookingUpToSearch = false;
      robot.SetHeadAngle(-1f);
      lastTurnSearchAngle = robot.Rotation.eulerAngles.z;
    }

  }

  private void Exit_SEARCH_FOR_FACE() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_ALIGN_TO_FACE() {
    SetBackpackColors(new Color(1.0f, 1.0f, 0.0f));
    alignToFaceWaiting = true;
    robot.FaceObject(robot.markersVisibleObjects[0]);
  }

  private void Update_ALIGN_TO_FACE() {
    if (playStateTimer > 3.0f) {
      nextState = FaceReactionState.SEARCH_FOR_FACE;
      Debug.Log("align to face took too long... going back to search");
      return;
    }
    if (!alignToFaceWaiting) {
      nextState = FaceReactionState.MOVE_TOWARD_FACE;
    }
  }

  private void Exit_ALIGN_TO_FACE() {
    
  }

  private void Enter_MOVE_TOWARD_FACE() {
    SetBackpackColors(new Color(1.0f, 0.0f, 0.0f));
    Debug.Log("moving toward face");
    robot.DriveWheels(20.0f, 20.0f);
  }

  private void Update_MOVE_TOWARD_FACE() {
    if (playStateTimer > 2.0f) {
      nextState = FaceReactionState.REACT_TO_FACE;
    }
  }

  private void Exit_MOVE_TOWARD_FACE() {
    robot.DriveWheels(0.0f, 0.0f);
  }

  private void Enter_REACT_TO_FACE() {
    int index = Random.Range(0, RobotEngineManager.instance.robotAnimationNames.Count);
    //SendAnimation(RobotEngineManager.instance.robotAnimationNames[index]);
    int fixedIndex = Random.Range(0, 5);
    switch (fixedIndex) {
    case 0:
      SendAnimation("majorWinBeatBox");
      break;
    case 1:
      SendAnimation("MinorIrritation");
      break;
    case 2:
      SendAnimation("Satisfaction");
      break;
    case 3:
      SendAnimation("winMatch");
      break;
    case 4:
      SendAnimation("shocked");
      break;
    }

    animationPlaying = true;
  }

  private void Update_REACT_TO_FACE() {
    if (!animationPlaying) {
      nextState = FaceReactionState.SPIN_RANDOM;
    }
  }

  private void Exit_REACT_TO_FACE() {
    
  }

  private void DecideLeftOrRight() {
    float leftRightDecision = Random.Range(0.0f, 1.0f);
    turningLeft = leftRightDecision < 0.5f;
  }

  private void RotateCozmo(float speed) {
    if (turningLeft) {
      robot.DriveWheels(speed, -speed);
    }
    else {
      robot.DriveWheels(-speed, speed);
    }
  }

  private void RobotEngineMessages(bool success, RobotActionType action_type) {
    if (success && action_type == RobotActionType.FACE_OBJECT) {
      alignToFaceWaiting = false;
      Debug.Log("Align to face successful!");
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

  private void SetBackpackColors(Color c) {
    robot.SetBackpackLEDs(CozmoPalette.ColorToUInt(c));
  }

}
