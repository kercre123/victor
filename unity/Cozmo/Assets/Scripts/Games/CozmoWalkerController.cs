using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class CozmoWalkerController : GameController {

  private bool gameReady = false;
  private bool gameOver = false;

  protected override void OnEnable() {
    base.OnEnable();
  }

  protected override void OnDisable() {
    base.OnDisable();
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
    gameReady = true;
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
    ProcessMovement();
    ProcessForkLift();
    ProcessHeadAngle();
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

  private void ProcessMovement() {
    float cozmo_angle = (robot.Rotation.eulerAngles.z + 90.0f) % 360.0f; // normalize to [0, 360)
    float phone_angle = (Input.gyro.attitude.eulerAngles.z);
    float delta = phone_angle - cozmo_angle;

    // prioritize rotation of cozmo over going forward.

    float leftSpeed = 0.0f;
    float rightSpeed = 0.0f;

    if (Mathf.Abs(delta) > 15.0f) {
      if (RotateClockWise(cozmo_angle, phone_angle)) {
        leftSpeed += 45.0f;
        rightSpeed += -45.0f;
      }
      else {
        leftSpeed += -45.0f;
        rightSpeed += 45.0f;
      }
    }

    if (Input.touchCount > 0 || Input.GetMouseButton(0)) {
      leftSpeed += 140.0f;
      rightSpeed += 140.0f;
    }
    robot.DriveWheels(leftSpeed, rightSpeed);
  }

  private void ProcessForkLift() {
    
  }

  private void ProcessHeadAngle() {
    
  }

  // curRot and newRot should be [0, 360.0f)
  private bool RotateClockWise(float curRot, float newRot) {
    float r1 = curRot - newRot;
    float r2 = newRot - curRot;

    // normalize to [0, 360.0f)
    if (r1 < 0.0f) {
      r1 += 360.0f;
    }
    if (r2 < 0.0f) {
      r2 += 360.0f;
    }
    return r2 > r1;
  }
}
